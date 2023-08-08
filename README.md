# README of the RZ/G2L Firmware Update TA

<Div Align="right">
Renesas Electronics Corporation

Aug-8-2023
</Div>

## 1. Overview

The Firmware Update TA is sample software for Renesas RZ/G2L Group MPUs.

The Firmware Update TA is used to write received firmware data to flash memory.

### 1.1. License

See [LICENSE.md](LICENSE.md)

### 1.2. Notice

The Firmware Update TA is distributed as a sample software from Renesas
without any warranty or support.

### 1.3. Contributing

To contribute to this software, you should email patches to renesas-rz@renesas.com.
Please send .patch files as email attachments, not embedded in the email body.

### 1.4. References

The following table shows the document related to this function.

#### Related Document

| No | Issuer  | Title                                                      | Edition           |
|----| ------- |------------------------------------------------------------|-------------------|
| 1  | Renesas | RZ/G2L Group and RZ/V2L SMARC EVK Security Start-up Guide  | Rev.1.00 or later |
| 2  | Renesas | Security Package for RZ MPU Security User's Manual         | Rev.2.00 or later |
| 3  | Renesas | Release Note for Verified Linux Package for 64bit kernel   | Rev.1.01 or later |
| 4  | Renesas | RZ/G2L Yocto recipe Start-Up Guide                         | Rev.1.01 or later |
| 5  | Renesas | RZ/G2L Reference Boards Start-up Guide                     | Rev.1.01 or later |

## 2. Operating Environment

### 2.1. Hardware Environment

The following table lists the hardware needed to use this utility.

#### Hardware Environment

| Name         | Description                                       |
|--------------|---------------------------------------------------|
| Target board | RZ/G2L SMARC Concrete Evaluation Kit(RZ/G2L EVK)  |
|              | RZ/G2L SMARC PMIC Evaluation Kit(RZ/G2L PMIC EVK) |
|              | RZ/V2L SMARC Concrete Evaluation Kit(RZ/V2L EVK)  |
|              | RZ/V2L SMARC PMIC Evaluation Kit(RZ/V2L PMIC EVK) |
|              | RZ/G2LC SMARC Evaluation Kit(RZ/G2LC EVK)         |
|              | RZ/G2UL SMARC Evaluation Kit(RZ/G2UL EVK)         |
| Host PC      | Ubuntu Desktop 20.04(64bit) or later              |

### 2.2. Software Environment

The Firmware Update TA must be executed on the TEE for RZ/G2L.
For building TEE for RZ/G2L, refer to [Related Document](#related-document)[1].

The following table lists the software required to use this sample software.

#### Software Environment

| Name            | Description                                               |
|-----------------|-----------------------------------------------------------|
| Verified Linux  | Linux environment on the evaluation board                 |
| Yocto SDK *1    | Yocto SDK built from Yocto environment for RZ/G2L Group   |
| OP-TEE Client   | libteec.so and headers exported by building OP-TEE Client |

*1: Regarding how to get the Yocto SDK, refer to [Related Document](#related-document)[3][4].

## 3. Software

### 3.1. Function

- Write fimrware iamge to Serial Flash.

## 4. How to build

This chapter is described how to build the Firmware Update TA.

### 4.1 Prepare the compiler

Get cross compiler for Yocto SDK.

Yocto SDK:

```shell
source /opt/poky/3.1.5/environment-setup-aarch64-poky-linux
```

### 4.2. Prepare the source code

Get the source code of the Firmware Update TA.

```shell
cd ~/
git clone https://github.com/renesas-rz/rzg_optee-ta_fwu.git
cd rzg_optee-ta_fwu
git checkout rzg2l
```

### 4.3. Build the software

Yocto SDK:

```shell
make clean
make
```

The executable files will be available in the following directory.
 - Client Application: out/ca


### 5. How to excute

This chapter is described how to execute the Firmware Update TA.

#### 5.1. Turn on the board, make sure that linux is started.

Start up the Linux environment on the evaluation board.
For information on how to start, refer to [Related Document](#related-document)[1][5].

#### 5.2. Store the executable files

Copy the executable files to the evaluation board.

#### Client Application

	- Executable files:
		rzg_optee_ta_flash
	- Location on the board:
		/usr/bin/

#### 5.3. Execute Application.

```shell
rzg_optee_ta_flash [-s] <OFFSET> <FILE>
	-s               Wirting to SPI Flash (default)
	<OFFSET>         Offset address to write region
	<FILE>           The path of the file to write
```

## 6. Revision history

Describe the revision history of the Firmware Update TA.

### v1.00

- First release.
