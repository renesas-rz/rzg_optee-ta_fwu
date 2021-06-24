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
#define STR_TRACE_USER_TA "FWU_TA"

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <utee_defines.h>
#include <string.h>

#include "fwu_ta.h"
#include "rzg_firmware_image_package.h"
#include "flash_pta.h"
#include "tsip_pta.h"

/******************************************************************************/
/* Macros                                                                     */
/******************************************************************************/
#define TA_NAME "fwu_ta.ta"

#ifndef FIP_FLAGS_END_OF_FILE
#define FIP_FLAGS_END_OF_FILE (0x8000)
#endif

#define INPUT_KEYRING_SIZE (0x2B0)
#define OUTPUT_KEYRING_SIZE (0x510)

#define SPI_FWU_PACKAGE_OFFSET_ADDR (0x3000000)
#define SPI_END_OFFSET_ADDR (0x4000000)

#define UPDATE_BOOT_DATA_MAX 16

/******************************************************************************/
/* Typedefs                                                                   */
/******************************************************************************/
/* Argument */

/******************************************************************************/
/* Static Function Prototypes                                                 */
/******************************************************************************/

static uuid_t uuid_null;
static const TEE_UUID tsip_uuid = TSIP_UUID;
static const TEE_UUID flash_uuid = FLASH_UUID;

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/

static TEE_Result fip_plain_out_size(uintptr_t fip_load_addr, uintptr_t fip_load_max, uint32_t *load_size, uint32_t *out_size)
{
	fip_toc_entry_t *toc_e;
	fip_toc_entry_t *toc_e_end = NULL;

	*out_size = 0;
	/* Get the address of the first TOC entry. */
	toc_e = (fip_toc_entry_t *)(fip_load_addr + sizeof(fip_toc_header_t));

	while ((uintptr_t)(toc_e + 1) <= (fip_load_max + 1))
	{
		/* Find the ToC terminator entry. */
		if (0 == memcmp(&toc_e->uuid, &uuid_null, sizeof(uuid_t)))
		{
			toc_e_end = toc_e;
			break;
		}

		toc_e++;
	}

	if (NULL == toc_e_end)
	{
		EMSG("FIP does not have the ToC terminator entry.\n");
		return TEE_ERROR_GENERIC;
	}

	*load_size = toc_e_end->offset_address;
	*out_size = toc_e_end->offset_address;

	return TEE_SUCCESS;
}

static TEE_Result fip_keyring_out_size(uintptr_t fip_load_addr, uintptr_t fip_load_max, uint32_t *load_size, uint32_t *out_size)
{
	fip_toc_entry_t *toc_e;
	fip_toc_entry_t *toc_e_end = NULL;

	*out_size = 0;
	/* Get the address of the first TOC entry. */
	toc_e = (fip_toc_entry_t *)(fip_load_addr + sizeof(fip_toc_header_t));

	while ((uintptr_t)(toc_e + 1) <= (fip_load_max + 1))
	{
		/* Find the ToC terminator entry. */
		if (0 == memcmp(&toc_e->uuid, &uuid_null, sizeof(uuid_t)))
		{
			toc_e_end = toc_e;
			break;
		}

		if ((INPUT_KEYRING_SIZE > toc_e->size) && (0 < toc_e->size))
		{
			EMSG("Invalid input Keyring data size \n");
			return TEE_ERROR_GENERIC;
		}

		if (INPUT_KEYRING_SIZE <= toc_e->size)
			*out_size += (OUTPUT_KEYRING_SIZE + sizeof(fip_toc_entry_t));

		toc_e++;
	}

	if (NULL == toc_e_end)
	{
		EMSG("FIP does not have the ToC terminator entry.\n");
		return TEE_ERROR_GENERIC;
	}

	*load_size = toc_e_end->offset_address;
	*out_size += (sizeof(fip_toc_entry_t) + sizeof(fip_toc_header_t));

	return TEE_SUCCESS;
}

static TEE_Result fip_encdata_out_size(uintptr_t fip_load_addr, uintptr_t fip_load_max, uint32_t *load_size, uint32_t *out_size)
{
	fip_toc_entry_t *toc_e;
	fip_toc_entry_t *toc_e_top;
	fip_toc_entry_t *toc_e_end = NULL;

	*out_size = 0;
	/* Get the address of the first TOC entry. */
	toc_e_top = (fip_toc_entry_t *)(fip_load_addr + sizeof(fip_toc_header_t));
	toc_e = toc_e_top;

	while ((uintptr_t)(toc_e + 1) <= (fip_load_max + 1))
	{
		/* Find the ToC terminator entry. */
		if (0 == memcmp(&toc_e->uuid, &uuid_null, sizeof(uuid_t)))
		{
			toc_e_end = toc_e;
			break;
		}

		if (0 != toc_e->size)
		{
			if (toc_e == toc_e_top)
			{
				/*output_size = 8(Re-encrypted size) + input_size + 16(MAC size) + 48(boot header) */
				*out_size += (sizeof(fip_toc_entry_t) + (toc_e->size + 72));
			}
			else
			{
				/*output_size = 8(Re-encrypted size) + input_size + 16(MAC size) */
				*out_size += (sizeof(fip_toc_entry_t) + (toc_e->size + 24));
			}
		}
		else
		{
			*out_size += sizeof(fip_toc_entry_t);
		}
		toc_e++;
	}

	if (NULL == toc_e_end)
	{
		EMSG("FIP does not have the ToC terminator entry.\n");
		return TEE_ERROR_GENERIC;
	}

	*load_size = toc_e_end->offset_address;
	*out_size += (sizeof(fip_toc_entry_t) + sizeof(fip_toc_header_t));

	return TEE_SUCCESS;
}

static TEE_Result fwu_calc_work_size(uint32_t type, TEE_Param p[TEE_NUM_PARAMS])
{
	TEE_Result res;
	uintptr_t fip_load_addr, fip_load_max;
	uint32_t load_size, fip_name, fip_flags;
	uint32_t fip_out_size;

	uint32_t exp_type = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
										TEE_PARAM_TYPE_VALUE_OUTPUT,
										TEE_PARAM_TYPE_NONE,
										TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (type != exp_type)
	{
		return TEE_ERROR_BAD_PARAMETERS;
	}

	p[1].value.a = 0;

	if (0 == p[0].memref.size)
		return TEE_SUCCESS;

	fip_load_addr = (uintptr_t)p[0].memref.buffer;
	fip_load_max = fip_load_addr + p[0].memref.size - 1;

	do
	{
		if ((fip_load_addr + sizeof(fip_toc_header_t)) >= fip_load_max)
		{
			EMSG("Loaded data doesn't match the FIP format\n");
			res = TEE_ERROR_GENERIC;
			break;
		}

		fip_name = ((fip_toc_header_t *)fip_load_addr)->name;
		fip_flags = ((fip_toc_header_t *)fip_load_addr)->flags >> 32;

		switch (fip_name)
		{
			case TOC_HEADER_NAME_PLAIN:
			{
				res = fip_plain_out_size(fip_load_addr, fip_load_max, &load_size, &fip_out_size);
				break;
			}
			case TOC_HEADER_NAME_KEYRING:
			{
				res = fip_keyring_out_size(fip_load_addr, fip_load_max, &load_size, &fip_out_size);
				break;
			}
			case TOC_HEADER_NAME_BOOT_FW:
			case TOC_HEADER_NAME_NS_BL2U:
			{
				res = fip_encdata_out_size(fip_load_addr, fip_load_max, &load_size, &fip_out_size);
				break;
			}
			default:
			{
				res = TEE_ERROR_GENERIC;
				EMSG("Unknown FIP name\n");
				break;
			}
		}
		if (TEE_SUCCESS == res)
		{
			p[1].value.a += fip_out_size;
			fip_load_addr += load_size;

			if (0 != (fip_flags & FIP_FLAGS_END_OF_FILE))
			{
				DMSG("The FIP platform flag END_OF_FILE has been detected.\n");
				break;
			}
		}

	} while ((TEE_SUCCESS == res) && (fip_load_addr < fip_load_max));

	return res;
}

static TEE_Result fip_copy_toc_hdr(uintptr_t fip_load_addr, uintptr_t fip_load_max, uintptr_t fip_out_addr, uintptr_t fip_out_max, fip_toc_entry_t **toc_e_end)
{
	fip_toc_entry_t *toc_e;
	fip_toc_entry_t *toc_e_tmp;

	/* Get the address of the first TOC entry. */
	toc_e = (fip_toc_entry_t *)(fip_load_addr + sizeof(fip_toc_header_t));

	/* Copy the TOC header to the work area. */
	memcpy((void *)fip_out_addr, (void *)fip_load_addr, sizeof(fip_toc_header_t));
	toc_e_tmp = (fip_toc_entry_t *)(fip_out_addr + sizeof(fip_toc_header_t));

	*toc_e_end = NULL;

	while ((uintptr_t)(toc_e + 1) < (fip_load_max + 1))
	{
		uintptr_t data_addr = fip_load_addr + toc_e->offset_address;

		if ((uintptr_t)(toc_e_tmp + 1) > (fip_out_max + 1))
		{
			EMSG("The copy data size exceeds the capacity of the output area.\n");
			return TEE_ERROR_GENERIC;
		}

		/* Find the ToC terminator entry. */
		if (0 == memcmp(&toc_e->uuid, &uuid_null, sizeof(uuid_t)))
		{
			*toc_e_end = toc_e;
			break;
		}

		if ((fip_load_max + 1) < (data_addr + toc_e->size))
		{
			EMSG("Data size exceeds FIP size.\n");
			return TEE_ERROR_GENERIC;
		}

		/* Copy the TOC entry to the temporary area. */
		memcpy((void *)toc_e_tmp, toc_e, sizeof(fip_toc_entry_t));

		toc_e++;
		toc_e_tmp++;
	}

	if (NULL == *toc_e_end)
	{
		EMSG("FIP does not have the ToC terminator entry.\n");
		return TEE_ERROR_GENERIC;
	}

	/* Copy the ToC terminator entry to the temporary area. */
	memcpy((void *)toc_e_tmp, *toc_e_end, sizeof(fip_toc_entry_t));

	*toc_e_end = toc_e_tmp;

	return TEE_SUCCESS;
}

static TEE_Result fip_plain_update(uintptr_t fip_load_addr, uint32_t *load_size, uintptr_t fip_out_addr, uint32_t *out_size)
{
	uintptr_t fip_load_max;
	uintptr_t fip_out_max;
	fip_toc_entry_t *toc_e_end;
	fip_toc_entry_t *toc_e;
	uintptr_t data_addr;

	fip_load_max = (fip_load_addr + *load_size) - 1;
	fip_out_max = (fip_out_addr + *out_size) - 1;

	/* Copy the TOC header and TOC entry to the output area */
	if (TEE_SUCCESS != fip_copy_toc_hdr(fip_load_addr, fip_load_max, fip_out_addr, fip_out_max, &toc_e_end))
		return TEE_ERROR_GENERIC;

	toc_e = (fip_toc_entry_t *)(fip_out_addr + sizeof(fip_toc_header_t));

	while (toc_e < toc_e_end)
	{
		data_addr = fip_out_addr + toc_e->offset_address;

		if ((fip_out_max + 1) < (data_addr + toc_e->size))
		{
			EMSG("The copy data size exceeds the capacity of the temporary ram area.\n");
			return TEE_ERROR_GENERIC;
		}

		/* Copy the data to the output area. */
		memcpy((void *)data_addr, (void *)(fip_load_addr + toc_e->offset_address), toc_e->size);

		toc_e++;
	}

	*load_size = toc_e_end->offset_address;
	*out_size = toc_e_end->offset_address;

	return TEE_SUCCESS;
}

static TEE_Result fip_keyring_update(uintptr_t fip_load_addr, uint32_t *load_size, uintptr_t fip_out_addr, uint32_t *out_size)
{
	TEE_Result res_final = TEE_SUCCESS;
	TEE_Result res = TEE_ERROR_GENERIC;
	TEE_TASessionHandle session = TEE_HANDLE_NULL;
	TEE_Param params[TEE_NUM_PARAMS];
	uint32_t ret_origin = 0;
	uint32_t param_types;
	uintptr_t fip_load_max;
	uintptr_t fip_out_max;
	fip_toc_entry_t *toc_e_end;
	fip_toc_entry_t *toc_e;
	uintptr_t data_addr;

	fip_load_max = (fip_load_addr + *load_size) - 1;
	fip_out_max = (fip_out_addr + *out_size) - 1;

	/* Copy the TOC header and TOC entry to the output area */
	if (TEE_SUCCESS != fip_copy_toc_hdr(fip_load_addr, fip_load_max, fip_out_addr, fip_out_max, &toc_e_end))
		return TEE_ERROR_GENERIC;

	res = TEE_OpenTASession(&tsip_uuid, 0, 0, NULL, &session,
							&ret_origin);

	if (res != TEE_SUCCESS)
		return res;

	/* Update the keyring and copy to the output area via pseudo TA*/
	toc_e = (fip_toc_entry_t *)(fip_out_addr + sizeof(fip_toc_header_t));

	data_addr = (uintptr_t)(toc_e_end + 1);

	while (toc_e < toc_e_end)
	{
		if ((fip_out_max + 1) < (data_addr + OUTPUT_KEYRING_SIZE))
		{
			EMSG("The copy data size exceeds the capacity of the temporary ram area.\n");
			res_final = TEE_ERROR_GENERIC;
			break;
		}

		if (INPUT_KEYRING_SIZE > toc_e->size)
		{
			EMSG("Invalid input Keyring data size \n");
			res_final = TEE_ERROR_GENERIC;
			break;
		}

		param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
									  TEE_PARAM_TYPE_MEMREF_OUTPUT,
									  TEE_PARAM_TYPE_NONE,
									  TEE_PARAM_TYPE_NONE);
		memset(&params, 0, sizeof(params));
		params[0].memref.buffer = (void *)((uint64_t)fip_load_addr + toc_e->offset_address);
		params[0].memref.size = INPUT_KEYRING_SIZE;

		params[1].memref.buffer = (void *)data_addr;
		params[1].memref.size = OUTPUT_KEYRING_SIZE;

		res = TEE_InvokeTACommand(session, 0, TSIP_CMD_UPDATE_KEYRING,
								  param_types, params, &ret_origin);
		if (res != TEE_SUCCESS)
		{
			EMSG("Failure when calling TSIP_CMD_UPDATE_KEYRING");
			res_final = TEE_ERROR_GENERIC;
			break;
		}

		/* Update the TOC entry for the injected keyring. */
		toc_e->size = OUTPUT_KEYRING_SIZE;
		toc_e->offset_address = data_addr - fip_out_addr;
		data_addr += toc_e->size;

		toc_e++;
	}

	*load_size = toc_e_end->offset_address;
	toc_e_end->offset_address = data_addr - fip_out_addr;
	*out_size = toc_e_end->offset_address;

	TEE_CloseTASession(session);

	return res_final;
}

static TEE_Result fip_encdata_update(uintptr_t fip_load_addr, uint32_t *load_size, uintptr_t fip_out_addr, uint32_t *out_size)
{
	TEE_Result res_final = TEE_SUCCESS;
	TEE_Result res = TEE_ERROR_GENERIC;
	TEE_TASessionHandle session = TEE_HANDLE_NULL;
	TEE_Param params[TEE_NUM_PARAMS];
	uint32_t ret_origin = 0;
	uint32_t param_types;
	uintptr_t fip_load_max;
	uintptr_t fip_out_max;

	fip_toc_entry_t *toc_e;
	fip_toc_entry_t *toc_e_end;
	uintptr_t data_addr;
	uint32_t data_cnt;

	update_fw_t input_update_fw[UPDATE_BOOT_DATA_MAX] = {0};
	update_fw_t output_update_fw[UPDATE_BOOT_DATA_MAX] = {0};

	fip_load_max = (fip_load_addr + *load_size) - 1;
	fip_out_max = (fip_out_addr + *out_size) - 1;

	/* Copy the TOC header and TOC entry to the output area */
	if (TEE_SUCCESS != fip_copy_toc_hdr(fip_load_addr, fip_load_max, fip_out_addr, fip_out_max, &toc_e_end))
		return TEE_ERROR_GENERIC;

	res = TEE_OpenTASession(&tsip_uuid, 0, 0, NULL, &session,
							&ret_origin);

	if (res != TEE_SUCCESS)
		return res;

	/* Re-encryption. */
	toc_e = (fip_toc_entry_t *)(fip_out_addr + sizeof(fip_toc_header_t));
	data_addr = (uintptr_t)(toc_e_end + 1);
	data_cnt = 0;

	while (toc_e < toc_e_end)
	{
		uint64_t reenc_data_size = 0;

		if (UPDATE_BOOT_DATA_MAX <= data_cnt)
		{
			EMSG("The number of data to be re-encrypted exceeds 16.\n");
			res_final = TEE_ERROR_GENERIC;
			break;
		}

		if (0 != toc_e->size)
		{
			if (0 == data_cnt)
				reenc_data_size = toc_e->size + 64;
			else
				reenc_data_size = toc_e->size + 16;

			if ((fip_out_max + 1) < (data_addr + sizeof(reenc_data_size) + reenc_data_size))
			{
				EMSG("The copy data size exceeds the capacity of the temporary ram area.\n");
				res_final = TEE_ERROR_GENERIC;
				break;
			}

			input_update_fw[data_cnt].data = (unsigned char *)((uint64_t)fip_load_addr + toc_e->offset_address);
			input_update_fw[data_cnt].size = (uint64_t)toc_e->size;
			output_update_fw[data_cnt].data = (unsigned char *)((uint64_t)data_addr + sizeof(reenc_data_size));
			output_update_fw[data_cnt].size = reenc_data_size;
			*(uint64_t *)data_addr = reenc_data_size;

			/* Update the TOC entry for the re-encrypted firmware. */
			toc_e->size = reenc_data_size + sizeof(reenc_data_size);
			toc_e->offset_address = data_addr - fip_out_addr;
			data_addr += toc_e->size;
		}

		data_cnt++;
		toc_e++;
	}

	/* Update the firmware and copy to the output area via pseudo TA*/
	param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
								  TEE_PARAM_TYPE_MEMREF_INPUT,
								  TEE_PARAM_TYPE_MEMREF_INOUT,
								  TEE_PARAM_TYPE_NONE);
	memset(&params, 0, sizeof(params));

	params[0].value.a = UPDATE_BOOT_DATA_MAX;

	params[1].memref.buffer = &input_update_fw;
	params[1].memref.size = sizeof(input_update_fw);

	params[2].memref.buffer = &output_update_fw;
	params[2].memref.size = sizeof(output_update_fw);

	res = TEE_InvokeTACommand(session, 0, TSIP_CMD_UPDATE_FIRMWARE,
							  param_types, params, &ret_origin);
	if (res != TEE_SUCCESS)
	{
		EMSG("Failure when calling TSIP_CMD_UPDATE_FIRMWARE");
		res_final = TEE_ERROR_GENERIC;
	}

	TEE_CloseTASession(session);

	*load_size = toc_e_end->offset_address;
	toc_e_end->offset_address = data_addr - fip_out_addr;
	*out_size = toc_e_end->offset_address;

	return res_final;
}

static TEE_Result fip_write_fw(uintptr_t write_buff, uint32_t write_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	TEE_TASessionHandle session = TEE_HANDLE_NULL;
	TEE_Param params[TEE_NUM_PARAMS] = {0};
	uint32_t ret_origin = 0;

	uint32_t param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
										   TEE_PARAM_TYPE_MEMREF_INPUT,
										   TEE_PARAM_TYPE_NONE,
										   TEE_PARAM_TYPE_NONE);

	if (SPI_END_OFFSET_ADDR < (write_size + SPI_FWU_PACKAGE_OFFSET_ADDR))
	{
		EMSG("The write data size exceeds the capacity of Flash Memory");
		return TEE_ERROR_GENERIC;
	}

	res = TEE_OpenTASession(&flash_uuid, 0, 0, NULL, &session,
							&ret_origin);

	if (res != TEE_SUCCESS)
		return res;

	params[0].value.a = SPI_FWU_PACKAGE_OFFSET_ADDR;
	params[1].memref.buffer = (void *)write_buff;
	params[1].memref.size = write_size;

	res = TEE_InvokeTACommand(session, 0, FLASH_CMD_WRITE_SPI,
							  param_types, params, &ret_origin);
	if (res != TEE_SUCCESS)
		EMSG("Failure when calling FLASH_CMD_WRITE_SPI");

	TEE_CloseTASession(session);

	return res;
}

static TEE_Result fwu_firmware_update(uint32_t type, TEE_Param p[TEE_NUM_PARAMS])
{
	TEE_Result res;

	uintptr_t fip_load_addr, fip_load_max;
	uintptr_t fip_out_addr, fip_out_max;
	uint32_t load_size, out_size;
	uint32_t fip_name, fip_flags;
	uint32_t write_size;
	uintptr_t write_buff;

	uint32_t exp_type = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
										TEE_PARAM_TYPE_MEMREF_INOUT,
										TEE_PARAM_TYPE_NONE,
										TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (type != exp_type)
	{
		EMSG("expect 1 output values as argument");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if ((0 == p[0].memref.size) || (0 == p[1].memref.size))
		return TEE_ERROR_BAD_PARAMETERS;

	fip_load_addr = (uintptr_t)p[0].memref.buffer;
	fip_load_max = fip_load_addr + p[0].memref.size - 1;
	fip_out_addr = (uintptr_t)p[1].memref.buffer;
	fip_out_max = fip_out_addr + p[1].memref.size - 1;

	do
	{
		if ((fip_load_addr + sizeof(fip_toc_header_t)) >= fip_load_max)
		{
			EMSG("Loaded data doesn't match the FIP format\n");
			res = TEE_ERROR_GENERIC;
			break;
		}

		fip_name = ((fip_toc_header_t *)fip_load_addr)->name;
		fip_flags = ((fip_toc_header_t *)fip_load_addr)->flags >> 32;
		load_size = (fip_load_max + 1) - fip_load_addr;
		out_size = (fip_out_max + 1) - fip_out_addr;

		switch (fip_name)
		{
		case TOC_HEADER_NAME_PLAIN:
		{
			res = fip_plain_update(fip_load_addr, &load_size, fip_out_addr, &out_size);
			break;
		}
		case TOC_HEADER_NAME_KEYRING:
		{
			res = fip_keyring_update(fip_load_addr, &load_size, fip_out_addr, &out_size);
			break;
		}
		case TOC_HEADER_NAME_BOOT_FW:
		case TOC_HEADER_NAME_NS_BL2U:
		{
			res = fip_encdata_update(fip_load_addr, &load_size, fip_out_addr, &out_size);
			break;
		}
		default:
		{
			res = TEE_ERROR_GENERIC;
			EMSG("Unknown FIP name\n");
			break;
		}
		}
		if (TEE_SUCCESS == res)
		{
			fip_load_addr += load_size;
			fip_out_addr += out_size;

			if (0 != (fip_flags & FIP_FLAGS_END_OF_FILE))
			{
				DMSG("The FIP platform flag END_OF_FILE has been detected.\n");
				break;
			}
		}

	} while ((TEE_SUCCESS == res) && (fip_load_addr < fip_load_max));

	if (TEE_SUCCESS != res)
		return res;

	write_buff = (uintptr_t)p[1].memref.buffer;
	write_size = fip_out_addr - write_buff;
	res = fip_write_fw(write_buff, write_size);
	if (res != (TEE_Result)TEE_SUCCESS)
	{
		EMSG("fip_write error\n");
		return res;
	}

	return TEE_SUCCESS;
}

/*
 * Trusted Application Entry Points
 */

TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("has been called");
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
	DMSG("has been called");
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t paramTypes __unused,
									TEE_Param __unused pParams[TEE_NUM_PARAMS], void **sessionContext __unused)
{
	DMSG("has been called");

	memset(&uuid_null, 0, sizeof(uuid_t));

	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *sessionContext __unused)
{
	DMSG("has been called");
}

TEE_Result TA_InvokeCommandEntryPoint(void *sessionContext __unused, uint32_t commandID,
									  uint32_t ptypes, TEE_Param params[TEE_NUM_PARAMS])
{
	switch (commandID)
	{
	case FWU_CMD_CALC_WORK_SIZE:
		return fwu_calc_work_size(ptypes, params);
	case FWU_CMD_FIRMWARE_UPDATE:
		return fwu_firmware_update(ptypes, params);
	default:
		break;
	}
	return TEE_ERROR_NOT_IMPLEMENTED;
}