# README of the RZ/G2 Firmware Update TA

<Div Align="right">
Renesas Electronics Corporation

Jul-28-2023
</Div>

## 1. Overview

The Firmware Update TA is sample software for Renesas RZ/G2 Group MPUs.

The Firmware Update TA is used to validate received firmware data and
re-encrypt it using a device-specific key.

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
| 1  | Renesas | Reference Boards of RZ/G2[H,M,N,E] Security Start-Up Guide | Rev.1.00 or later |
| 2  | Renesas | RZ/G2 Trusted Execution Environment Porting Guide          | Rev.2.00 or later |
| 3  | Renesas | Release Note for Verified Linux Package for 64bit kernel   | Rev.1.01 or later |
| 4  | Renesas | RZ/G2 Yocto recipe Start-Up Guide                          | Rev.1.01 or later |
| 5  | Renesas | RZ/G2 Reference Boards Start-up Guide                      | Rev.1.01 or later |

## 2. Operating Environment

### 2.1. Hardware Environment

The following table lists the hardware needed to use this utility.

#### Hardware Environment

| Name         | Description                                 |
|--------------|---------------------------------------------|
| Target board | Hoperun HiHope RZ/G2[M,N,H] platform        |
|              | Silicon Linux RZ/G2E evaluation kit (EK874) |
| Host PC      | Ubuntu Desktop 20.04(64bit) or later        |

### 2.2. Software Environment

The Firmware Update TA must be executed on the TEE for RZ/G2.
For building TEE for RZ/G2, refer to [Related Document](#related-document)[1].

The following table lists the software required to use this sample software.

#### Software Environment

| Name            | Description                                               |
|-----------------|-----------------------------------------------------------|
| Verified Linux  | Linux environment on the evaluation board                 |
| Yocto SDK *1    | Yocto SDK built from Yocto environment for RZ/G2 Group    |
| OP-TEE Client   | libteec.so and headers exported by building OP-TEE Client |

*1: Regarding how to get the Yocto SDK, refer to [Related Document](#related-document)[3][4].

## 3. Software

### 3.1. Function

- Verification and re-encryption of Keyring
- Verification and re-encryption of Firmware Image

### 3.2. Option setting

The Firmware Update TA support the following build options.

#### 3.2.1. MACHINE

Select from the following table according to the BOARD settings.

| MACHINE          | BOARD setting                                                          |
| ---------------- | ---------------------------------------------------------------------- |
| hihope_rzg2h     | Generate binary that works on Hoperun HiHope RZ/G2H platform           |
| hihope_rzg2m     | Generate binary that works on Hoperun HiHope RZ/G2M platform           |
| hihope_rzg2n     | Generate binary that works on Hoperun HiHope RZ/G2N platform           |
| ek874            | Generate binary that works on Silicon Linux RZ/G2E evaluation kit      |


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
git checkout rzg2
```

### 4.3. Build the software

Yocto SDK:

```shell
make clean
make MACHINE=<MACHINE>
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
		rzg_optee_ta_fwu
		rzg_optee_ta_flash
	- Location on the board:
		/usr/bin/

#### 5.3. Execute Application.

```shell
rzg_optee_ta_fwu <-k|-f> <DIR>
	-k    Keyring Update Session
	-f    Firmware Update Session
	<DIR> Directory where the firmware is stored
```

Note) The updated firmware image is encrypted. For more informations about preparing \
to update firmware, refer to [Related Document](#related-document)[2].

## 6. Revision history

Describe the revision history of the Firmware Update TA.

### v1.00

- First release.

### v2.00

- Renewal of RZ/G2 Firmware Update TA.
  Change the method to run PTA directly from CA to update the firmware.
