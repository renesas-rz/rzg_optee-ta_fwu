/*
 * Copyright (c) 2021-2023, Renesas Electronics Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <flash_pta.h>
#include <tee_client_api.h>

typedef struct {
	TEEC_Context *ctx;
	TEEC_Session *sess;
	TEEC_SharedMemory file_sm;
	uint32_t offset;
	char *file;
} st_sample_ctx_t;

static void usage(char *program)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Writing to Flash Memory Sample\n");
	fprintf(stderr, "Usage: %s [-s] <OFFSET> <FILE> \n", program);
	fprintf(stderr, "\t-s               Wirting to SPI Flash (default)\n");
	fprintf(stderr, "\t<OFFSET>         Offset address to write region\n");
	fprintf(stderr, "\t<FILE>           The path of the file to write\n");
	fprintf(stderr, "\n");
	exit(1);
}

static void get_args(int argc, char *argv[], st_sample_ctx_t *ctx)
{
	int opt;
	char *ep;

	if (argc < 3)
		usage(argv[0]);

	while (-1 != (opt = getopt(argc, argv, "s"))) {
		switch (opt) {
		case 's':
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	if ((optind + 2) < argc) {
		fprintf(stderr, "Cannot find the <OFFSET> or <FILE> options.\n");
		usage(argv[0]);
	}

	ctx->offset = strtol(argv[optind], &ep, 0);
	if (*ep) {
		fprintf(stderr, "Cannot parse <OFFSET>: \"%s\"\n", argv[optind]);
		usage(argv[0]);
	}

	ctx->file = argv[optind + 1];
}

static int load_file(char *file, void *buf, size_t *buf_sz)
{
	int res = -1;
	FILE *fp = NULL;
	struct stat statbuf;

	if ((!file) || (!buf) || (!buf_sz)) {
		fprintf(stderr, "Bad arguments\n");
		goto err;
	}

	if (stat(file, &statbuf) != 0) {
		fprintf(stderr, "Failed to get size: \"%s\"\n", strerror(errno));
		goto err;
	}

	if (*buf_sz < statbuf.st_size) {
		fprintf(stderr, "Bad file size: \"%ld\"\n", *buf_sz);
		goto err;
	}

	if ((fp = fopen(file, "rb")) == NULL) {
		fprintf(stderr, "Failed to open: \"%s\"\n", file);
		goto err;
	}

	*buf_sz = statbuf.st_size;
	if (*buf_sz > fread(buf, 1, *buf_sz, fp)) {
		fprintf(stderr, "Failed to read: \"%s\"\n", strerror(errno));
		goto err;
	}

	res = 0;
err:
	if (fp)
		fclose(fp);

	return res;
}

static int prepare_tee_session(st_sample_ctx_t *ctx)
{
	static TEEC_Context context;
	static TEEC_Session session;

	TEEC_UUID uuid = FLASH_UUID;
	uint32_t origin;
	TEEC_Result res;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &context);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_InitializeContext failed with code 0x%x\n", res);
		return -1;
	}
	ctx->ctx = &context;

	/* Open a session with the TA */
	res = TEEC_OpenSession(&context, &session, &uuid,
				   TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_Opensession failed with code 0x%x origin 0x%x\n", res, origin);
		return -1;
	}
	ctx->sess = &session;

	return 0;
}

static void terminate_tee_session(st_sample_ctx_t *ctx)
{
	if (ctx->sess)
		TEEC_CloseSession(ctx->sess);

	if (ctx->ctx)
		TEEC_FinalizeContext(ctx->ctx);

	ctx->ctx = NULL;
	ctx->sess = NULL;
}

static int load_file_data(st_sample_ctx_t *ctx)
{
	TEEC_Result res;
	uintptr_t file_buff;
	size_t file_size;
	struct stat statbuf;

	if (stat(ctx->file, &statbuf) != 0) {
		fprintf(stderr, "Failed to get size: \"%s\"\n", strerror(errno));
		return -1;
	}

	(ctx->file_sm).size = statbuf.st_size;
	(ctx->file_sm).flags = TEEC_MEM_INPUT;

	res = TEEC_AllocateSharedMemory(ctx->ctx, &ctx->file_sm);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_AllocateSharedMemory failed with code 0x%x\n", res);
		return -1;
	}

	file_buff = (uintptr_t)(ctx->file_sm).buffer;
	file_size = (ctx->file_sm).size;

	res = load_file(ctx->file, (void *)file_buff, &file_size);
	if (res)
		return -1;

	return 0;
}

static int flash_write_session(st_sample_ctx_t *ctx)
{
	TEEC_Result res;
	TEEC_Operation op;
	uint32_t origin;

	op.paramTypes = (uint32_t)TEEC_PARAM_TYPES(
				 TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = ctx->offset;
	op.params[1].tmpref.buffer = (ctx->file_sm).buffer;
	op.params[1].tmpref.size = (ctx->file_sm).size;

	res = TEEC_InvokeCommand(ctx->sess, FLASH_CMD_WRITE_SPI, &op, &origin);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_InvokeCommand(WRITE_SPI) failed 0x%x origin 0x%x\n", res, origin);
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int res;
	st_sample_ctx_t ctx;

	memset(&ctx, 0, sizeof(ctx));

	/* Parse command line options */
	get_args(argc, argv, &ctx);

	printf("Prepare session with the TA\n");
	res = prepare_tee_session(&ctx);
	if (res)
		goto err;

	printf("Load file data\n");
	res = load_file_data(&ctx);
	if (res)
		goto err;

	printf("Write file data to flash memory\n");
	res = flash_write_session(&ctx);
	if (res)
		goto err;

err:
	if ((ctx.file_sm).buffer)
		TEEC_ReleaseSharedMemory(&ctx.file_sm);

	terminate_tee_session(&ctx);

	return 0;
}
