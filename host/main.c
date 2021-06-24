/*
 * Copyright (c) 2021, Renesas Electronics Corporation
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <tee_client_api.h>

#include <fwu_ta.h>

int main(int argc, char *argv[])
{
	TEEC_Result res = (TEEC_Result)TEEC_SUCCESS;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = FWU_TA_UUID;
	uint32_t err_origin;

	FILE *fp_in = NULL;
	size_t readSize;
	size_t work_size;
	uint8_t *input_buf = NULL;
	uint8_t *output_buf = NULL;
	struct stat st;

	if (argc != 2)
	{
		res = TEEC_ERROR_GENERIC;
		(void)printf("Error argc=%d\n", argc);
	}

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/* Open a session to the "fwu ta". */
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
						   TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			 res, err_origin);

	/* Get the stat of the file. */
	if (stat(argv[1], &st) != 0)
		errx(1, "File access error %s\n", argv[1]);

	/* Allocates a buffer. */
	input_buf = malloc((size_t)st.st_size);
	if (input_buf == NULL)
		err(1, "Memory allocate error\n");

	/* Input file */
	fp_in = fopen(argv[1], "rb");
	if (fp_in == NULL)
	{
		errx(0, "fopen error in_file=%s\n", argv[1]);
		goto err_end;
	}

	readSize = fread(input_buf, 1U, (size_t)st.st_size, fp_in);
	if (readSize != st.st_size)
	{
		errx(0, "File read error %ld\n", readSize);
		goto err_end;
	}

	/* get size of output FIP */
	(void)memset(&op, 0, sizeof(op));
	op.paramTypes = (uint32_t)TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.buffer = (void *)input_buf;
	op.params[0].tmpref.size = readSize;

	res = TEEC_InvokeCommand(&sess, (uint32_t)FWU_CMD_CALC_WORK_SIZE, &op, &err_origin);
	if (res != TEEC_SUCCESS)
	{
		errx(0, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			 res, err_origin);
		goto err_end;
	}

	/* Allocates a output buffer. */
	work_size = op.params[1].value.a;
	if (0 == work_size)
	{
		errx(0, "Invalid Update data\n");
		goto err_end;
	}
	
	output_buf = malloc(work_size);
	if (output_buf == NULL)
	{
		err(0, "Memory allocate error\n");
		goto err_end;
	}

	/* Update Fip data and save to SPI Flash*/
	(void)memset(&op, 0, sizeof(op));
	op.paramTypes = (uint32_t)TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INOUT, TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.buffer = (void *)input_buf;
	op.params[0].tmpref.size = readSize;
	op.params[1].tmpref.buffer = (void *)output_buf;
	op.params[1].tmpref.size = work_size;

	res = TEEC_InvokeCommand(&sess, (uint32_t)FWU_CMD_FIRMWARE_UPDATE, &op, &err_origin);
	if (res != TEEC_SUCCESS)
	{
		errx(0, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			 res, err_origin);
		goto err_end;
	}

	/*
	 * We're done with the TA, close the session and
	 * destroy the context.
	 */
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	printf("We need to reset system to compele Firmware update process.\n");
	printf("After the update is complete , please remove Update package \n");

err_end:
	if (NULL != fp_in)
		(void)fclose(fp_in);
	if (NULL != input_buf)
		free(input_buf);
	if (NULL != output_buf)
		free(output_buf);

	return 0;
}
