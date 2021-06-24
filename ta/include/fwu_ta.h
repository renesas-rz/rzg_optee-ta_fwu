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
#ifndef FWU_TA_H
#define FWU_TA_H

/* This UUID is generated with uuidgen
   the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html */

/* UUID of the Firmware update trusted application */
#define FWU_TA_UUID                                                \
    {                                                              \
        0x12F74D4FU, 0x175DU, 0x4646U,                             \
        {                                                          \
            0xAAU, 0xB5U, 0xBFU, 0x26U, 0x17U, 0xE2U, 0xC2U, 0xCAU \
        }                                                          \
    }

/* The function IDs implemented in this TA */
/*
 * FWU_CMD_CALC_WORK_SIZE - calculate work ram size
 * param[0] (memref) Input data 
 * param[1] (value) work size
 * param[2] unused
 * param[3] unused
 */
#define FWU_CMD_CALC_WORK_SIZE 1

/*
 * FWU_CMD_FIRMWARE_UPDATE - Update Firmware data and save to SPI flash  
 * param[0] (memref) Input data 
 * param[1] (memref) work buffer
 * param[2] unused
 * param[3] unused
 */
#define FWU_CMD_FIRMWARE_UPDATE 2


#endif /* FWU_TA_H */
