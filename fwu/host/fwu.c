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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <tsip_pta.h>
#include <tee_client_api.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
	bool skip;
	char *infile;
	char *outfile;
} st_fw_info_t;

typedef struct {
	TEEC_Context *ctx;
	TEEC_Session *sess;
	TEEC_SharedMemory input_sm;
	TEEC_SharedMemory output_sm;
	char *dir;
	bool fwu_mode;
} st_sample_ctx_t;

static char filepath[1024];

static st_fw_info_t key_info = {
#if defined(MACHINE_hihope_rzg2h)
	0, "Keyring-hihope-rzg2h_Enc.bin", "Keyring-hihope-rzg2h_fwu.bin"
#elif defined(MACHINE_hihope_rzg2m)
	0, "Keyring-hihope-rzg2m_Enc.bin", "Keyring-hihope-rzg2m_fwu.bin"
#elif defined(MACHINE_hihope_rzg2n)
	0, "Keyring-hihope-rzg2n_Enc.bin", "Keyring-hihope-rzg2n_fwu.bin"
#elif defined(MACHINE_ek874)
	0, "Keyring-ek874_Enc.bin", "Keyring-ek874_fwu.bin"
#else
#error "MACHINE not set."
#endif
};

static st_fw_info_t fwu_info[] = {
#if defined(MACHINE_hihope_rzg2h)
	{ 0, "bl31-hihope-rzg2h_Enc.bin", "bl31-hihope-rzg2h_fwu.bin" },
	{ 0, "tee-hihope-rzg2h_Enc.bin", "tee-hihope-rzg2h_fwu.bin" },
	{ 0, "u-boot-hihope-rzg2h_Enc.bin", "u-boot-hihope-rzg2h_fwu.bin" },
#elif defined(MACHINE_hihope_rzg2m)
	{ 0, "bl31-hihope-rzg2m_Enc.bin", "bl31-hihope-rzg2m_fwu.bin" },
	{ 0, "tee-hihope-rzg2m_Enc.bin", "tee-hihope-rzg2m_fwu.bin" },
	{ 0, "u-boot-hihope-rzg2m_Enc.bin", "u-boot-hihope-rzg2m_fwu.bin" },
#elif defined(MACHINE_hihope_rzg2n)
	{ 0, "bl31-hihope-rzg2n_Enc.bin", "bl31-hihope-rzg2n_fwu.bin" },
	{ 0, "tee-hihope-rzg2n_Enc.bin", "tee-hihope-rzg2n_fwu.bin" },
	{ 0, "u-boot-hihope-rzg2n_Enc.bin", "u-boot-hihope-rzg2n_fwu.bin" },
#elif defined(MACHINE_ek874)
	{ 0, "bl31-ek874_Enc.bin", "bl31-ek874_fwu.bin" },
	{ 0, "tee-ek874_Enc.bin", "tee-ek874_fwu.bin" },
	{ 0, "u-boot-ek874_Enc.bin", "u-boot-ek874_fwu.bin" },
#else
#error "MACHINE not set."
#endif
};

static void usage(char *program)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Firmware Update Sample\n");
	fprintf(stderr, "Usage: %s <-k|-f> <DIR> \n", program);
	fprintf(stderr, "\t-k               Keyring Update Session (default)\n");
	fprintf(stderr, "\t-f               Firmware Update Session\n");
	fprintf(stderr, "\t<DIR>            Directory where the firmware is stored\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "\tKeyring File:\n");
	fprintf(stderr, "\t\t%s => %s\n", key_info.infile, key_info.outfile);
	fprintf(stderr, "\tFirmware File:\n");
	for (int i = 0; i < ARRAY_SIZE(fwu_info); i++)
		fprintf(stderr, "\t\t%s => %s\n", fwu_info[i].infile, fwu_info[i].outfile);
	fprintf(stderr, "\n");
	exit(1);
}

static void get_args(int argc, char *argv[], st_sample_ctx_t *ctx)
{
	int opt;

	if (argc < 3)
		usage(argv[0]);

	ctx->fwu_mode = false;

	while (-1 != (opt = getopt(argc, argv, "kf"))) {
		switch (opt) {
		case 'k':
			/* Update Keyring */
			ctx->fwu_mode = false;
			break;
		case 'f':
			/* Update Firmware */
			ctx->fwu_mode = true;
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	if ((optind + 1) < argc) {
		fprintf(stderr, "Cannot find the <DIR> option.\n");
		usage(argv[0]);
	}

	ctx->dir = argv[optind];
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

static int save_file(char *file, void *buf, size_t buf_sz, int new)
{
	int res = -1;
	FILE *fp = NULL;
	char *mode = (new) ? "wb" : "ab";

	if ((!file) || (!buf)) {
		fprintf(stderr, "Bad arguments\n");
		goto err;
	}

	if ((fp = fopen(file, mode)) == NULL) {
		fprintf(stderr, "Failed to open: \"%s\"\n", file);
		goto err;
	}

	if (buf_sz > fwrite(buf, 1, buf_sz, fp)) {
		fprintf(stderr, "Failed to write: \"%s\"\n", strerror(errno));
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

	TEEC_UUID uuid = TSIP_UUID;
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

static int alloc_keyring_memory(st_sample_ctx_t *ctx)
{
	TEEC_Result res;
	uintptr_t fwu_base;
	size_t fwu_size;

	/* temporary encrypted keyring size */
	(ctx->input_sm).size = 688;
	(ctx->input_sm).flags = TEEC_MEM_INPUT;

	/* re-encrypted keyring size */
	(ctx->output_sm).size = 1296;
	(ctx->output_sm).flags = TEEC_MEM_OUTPUT;

	res = TEEC_AllocateSharedMemory(ctx->ctx, &ctx->input_sm);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_AllocateSharedMemory failed with code 0x%x\n", res);
		return -1;
	}

	res = TEEC_AllocateSharedMemory(ctx->ctx, &ctx->output_sm);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_AllocateSharedMemory failed with code 0x%x\n", res);
		return -1;
	}

	fwu_base = (uintptr_t)(ctx->input_sm).buffer;
	fwu_size = (ctx->input_sm).size;

	sprintf(filepath, "%s/%s", ctx->dir, key_info.infile);

	printf("\t%s\n", filepath);

	res = load_file(filepath, (void *)fwu_base, &fwu_size);
	if (res)
		return -1;

	return 0;
}

static int alloc_firmware_memory(st_sample_ctx_t *ctx, st_update_fw_t fwu_param[16])
{
	TEEC_Result res;
	uintptr_t fwu_base;
	size_t fwu_size;

	for (int i = 0; i < ARRAY_SIZE(fwu_info); i++) {

		struct stat statbuf;

		sprintf(filepath, "%s/%s", ctx->dir, fwu_info[i].infile);

		if (stat(filepath, &statbuf) != 0) {
			fwu_info[i].skip = 1;
			printf("\tSkip => %s not found\n", filepath);
			continue;
		}
		else {
			fwu_info[i].skip = 0;
			printf("\t%s\n", filepath);
		}

		(ctx->input_sm).size += statbuf.st_size;
		(ctx->output_sm).size += statbuf.st_size + 64;
	}

	(ctx->input_sm).flags = TEEC_MEM_INPUT;
	(ctx->output_sm).flags = TEEC_MEM_OUTPUT;

	res = TEEC_AllocateSharedMemory(ctx->ctx, &ctx->input_sm);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_AllocateSharedMemory failed with code 0x%x\n", res);
		return -1;
	}

	res = TEEC_AllocateSharedMemory(ctx->ctx, &ctx->output_sm);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_AllocateSharedMemory failed with code 0x%x\n", res);
		return -1;
	}

	fwu_base = (uintptr_t)(ctx->input_sm).buffer;
	fwu_size = (ctx->input_sm).size;

	for (int i = 0; i < ARRAY_SIZE(fwu_info); i++) {

		/* For firmware that does not need to be updated, set the input size to 0. */
		if (fwu_info[i].skip) {
			fwu_param[i].length = 0;
			continue;
		}

		sprintf(filepath, "%s/%s", ctx->dir, fwu_info[i].infile);

		res = load_file(filepath, (void *)fwu_base, &fwu_size);
		if (res)
			return -1;

		fwu_param[i].offset = fwu_base - (uintptr_t)(ctx->input_sm).buffer;
		fwu_param[i].length = fwu_size;

		fwu_base = fwu_base + fwu_size;
		fwu_size = (ctx->input_sm).size - fwu_param[i].offset;
	}

	return 0;
}

static int update_keyring_session(st_sample_ctx_t *ctx)
{
	TEEC_Result res;
	TEEC_Operation op;
	uint32_t origin;

	op.paramTypes = (uint32_t)TEEC_PARAM_TYPES(
				TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.buffer = (ctx->input_sm).buffer;
	op.params[0].tmpref.size = (ctx->input_sm).size;
	op.params[1].tmpref.buffer = (ctx->output_sm).buffer;
	op.params[1].tmpref.size = (ctx->output_sm).size;

	res = TEEC_InvokeCommand(ctx->sess, TSIP_CMD_UPDATE_KEYRING, &op, &origin);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_InvokeCommand(UPDATE_KEYRING) failed 0x%x origin 0x%x\n", res, origin);
		return -1;
	}

	return 0;
}

static int update_firmware_session(st_sample_ctx_t *ctx, st_update_fw_t *fwu_param)
{
	TEEC_Result res;
	TEEC_Operation op;
	uint32_t origin;

	op.paramTypes = (uint32_t)TEEC_PARAM_TYPES(
				 TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INOUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT);
	op.params[0].value.a = ARRAY_SIZE(fwu_info);
	op.params[1].tmpref.buffer = (void *)fwu_param;
	op.params[1].tmpref.size = sizeof(st_update_fw_t) * op.params[0].value.a;
	op.params[2].tmpref.buffer = (ctx->input_sm).buffer;
	op.params[2].tmpref.size = (ctx->input_sm).size;
	op.params[3].tmpref.buffer = (ctx->output_sm).buffer;
	op.params[3].tmpref.size = (ctx->output_sm).size;

	res = TEEC_InvokeCommand(ctx->sess, TSIP_CMD_UPDATE_FIRMWARE, &op, &origin);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_InvokeCommand(UPDATE_FIRMWARE) failed 0x%x origin 0x%x\n", res, origin);
		return -1;
	}

	return 0;
}

static int save_updated_keyring(st_sample_ctx_t *ctx)
{
	int res;
	uint64_t size_info = (uint64_t)(ctx->output_sm).size;

	sprintf(filepath, "%s/%s", ctx->dir, key_info.outfile);

	/* Add size info to the top of the firmware image */
	res = save_file(filepath, &size_info, sizeof(size_info), 1);
	if (res)
		return -1;

	res = save_file(filepath, (ctx->output_sm).buffer, (ctx->output_sm).size, 0);
	if (res)
		return -1;

	printf("\t%s (%ld bytes)\n", filepath, (ctx->output_sm).size + sizeof(size_info));

	return 0;
}

static int save_updated_firmware(st_sample_ctx_t *ctx, st_update_fw_t *fwu_param)
{
	int res;
	uintptr_t fwu_base = (uintptr_t)(ctx->output_sm).buffer;

	for (int i = 0; i < ARRAY_SIZE(fwu_info); i++) {

		uint64_t size_info = (uint64_t)fwu_param[i].length;

		if (fwu_info[i].skip)
			continue;

		sprintf(filepath, "%s/%s", ctx->dir, fwu_info[i].outfile);

		/* Add size info to the top of the firmware image */
		res = save_file(filepath, &size_info, sizeof(size_info), 1);
		if (res)
			return -1;

		res = save_file(filepath, (void *)(fwu_base + fwu_param[i].offset), fwu_param[i].length, 0);
		if (res)
			return -1;

		printf("\t%s (%ld bytes)\n", filepath, fwu_param[i].length + sizeof(size_info));
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int res;
	st_sample_ctx_t ctx;
	st_update_fw_t fwu_param[16];

	memset(&ctx, 0, sizeof(ctx));
	memset(&fwu_param, 0, sizeof(fwu_param));

	/* Parse command line options */
	get_args(argc, argv, &ctx);

	printf("Prepare session with the TA\n");
	res = prepare_tee_session(&ctx);
	if (res)
		goto err;

	if (ctx.fwu_mode) {

		/* Firmware update mode */

		printf("Load temporary encrypted firmware\n");
		res = alloc_firmware_memory(&ctx, &fwu_param[0]);
		if (res)
			goto err;

		printf("Update firmware session\n");
		res = update_firmware_session(&ctx, &fwu_param[0]);
		if (res)
			goto err;

		printf("Save the updated firmware to file\n");
		res = save_updated_firmware(&ctx, &fwu_param[0]);
		if (res)
			goto err;

	} else {

		/* Keyring update mode */

		printf("Load temporary encrypted keyring\n");
		res = alloc_keyring_memory(&ctx);
		if (res)
			goto err;

		printf("Update keyring session\n");
		res = update_keyring_session(&ctx);
		if (res)
			goto err;

		printf("Save the updated keyring to file\n");
		res = save_updated_keyring(&ctx);
		if (res)
			goto err;
	}

err:
	if ((ctx.input_sm).buffer)
		TEEC_ReleaseSharedMemory(&ctx.input_sm);
	if ((ctx.output_sm).buffer)
		TEEC_ReleaseSharedMemory(&ctx.output_sm);

	terminate_tee_session(&ctx);

	return 0;
}
