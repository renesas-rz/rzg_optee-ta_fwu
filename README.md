# RZ/G2 Sample Trusted Applications related to Firmware Update feature

<Div Align="right">
Renesas Electronics Corporation

Jun-30-2021
</Div>

This git contains source code for Trusted Applications(TA) related to Firmware Update feature.\
Currently the following boards and MPUs are supported:

- Board: EK874 / MPU: R8A774C0 (RZ/G2E)
- Board: HIHOPE-RZG2M / MPU: R8A774A1 (RZG2M)
- Board: HIHOPE-RZG2N / MPU: R8A774B1 (RZG2N)
- Board: HIHOPE-RZG2H / MPU: R8A774E1 (RZG2H)


## 1. Overview

### 1.1. License

- The client applications (`host/*`) are provided under the
[GPL-2.0](http://opensource.org/licenses/GPL-2.0) license.

- The user TAs (`ta/*`) are provided under the
[BSD 2-Clause](http://opensource.org/licenses/BSD-2-Clause) license.


### 1.2. Notice

The RZ/G Firmware Update TA is distributed as a sample software from Renesas without any warranty or support.


### 1.3. Contributing

To contribute to this software, you should email patches to renesas-rz@renesas.com. Please send .patch files as email attachments, not embedded in the email body.

### 1.4. References

The following table shows the document related to this function.

| Number | Issuer  | Title                                                          | Edition           |
|--------|---------|----------------------------------------------------------------|-------------------|
| 1      | Renesas | RZ/G2 Trusted Execution Environment Start-Up Guide             | Rev.1.10 or later |
| 2      | Renesas | RZ/G2 Trusted Execution Environment Porting Guide              | Rev.1.10 or later |

## 2. Operating Environment

### 2.1. Hardware Environment

The following table lists the hardware needed to use this utility.

__Hardware environment__

| Name         | Note                                        |
|--------------|---------------------------------------------|
| Target board | Hoperun HiHope RZ/G2[M,N,H] platform        |
|              | Silicon Linux RZ/G2E evaluation kit (EK874) |

### 2.2. Software Environment

The RZ/G Firmware Update TA must be executed on the TEE for RZ/G2 Environment.

For building TEE for RZ/G2 enviroment , refer to References Document 1.

## 3. Software

### 3.1. Software Function
Trusted Applications related to Firmware Update is used to re-encrypts incoming update firmware data using a device-specific key.


### 3.2. Build instructions
The following is the method to build the sample Firmware Update TA.


#### 3.2.1. Build the dependencies
Then you must build optee_os as well as optee_client first. Build instructions for them can be found on their respective pages.

Please build TEE for RZ/G2 environment from the beginning, or prepare the necessary SDK from the already built Yocto environment. \
When building Yocto environment, follow the procedure of the following reference to build Yocto environment.
Refer to the References Document No.1.

#### 3.2.2. Clone Firmware Update TA

```bash
$ git clone https://github.com/renesas-rz/rzg_optee-ta_fwu.git
$ cd rzg_optee-ta_fwu
$ git checkout -b v1.00
```

#### 3.2.3. Setting the path for Toolchain (64bit) and environment variable.

```bash
$ PATH=<SDK>/sysroots/x86_64-pokysdklinux/usr/bin/aarch64-poky-linux:$PATH
$ source <SDK>/environment-setup-aarch64-poky-linux
```

#### 3.2.4. Build using GNU Make
__Host application__

```bash
$ cd rzg_optee-ta_fwu/host
$ make \
    TEEC_EXPORT=<optee_client>/out/export/usr 
```
With this you end up with a binary 'fwu' in the host folder where you did the build.

__Trusted Application__
```bash
$ cd rzg_optee-ta_fwu/ta
$ make \
    TA_DEV_KIT_DIR=<optee_os>/out/arm/export-ta_arm32
```

With this you end up with a files named uuid.{ta,elf,dmp,map} etc in the ta folder where you did the build.

### 3.3. How to excute the Applications
The following is the method to execute Firmware Update TA.

#### 3.3.1. Turn on the board, make sure that linux is started.


#### 3.3.2. Store the executable files (CA and TA).__

Copy CA\
Location on the
environment : $WORK/rzg_optee-ta_fwu/host/ \
Executable file name : fwu \
Location on the board : /usr/bin/ \
Copy the executable file from the build environment to the board. \
Copy TA \
Location on the build environment : $WORK/rzg_optee-ta_fwu/ta/ \
Executable file name : 12f74d4f-175d-4646-aab5bf2617e2c2ca.ta \
Location on the board : /lib/optee_armtz/ \
Copy the executable file from the build environment to the board. \
Note) If optee_armtz directory does not exist, create it.

#### 3.3.3. Execute CA.__

```bash
    $ fwu {update firmware package}
```

Note) The update firmware package was encrypted and packed to FIP format define for RZ/G2 platform. For more informations about preparing to update firmware , refer to References Document 2.
## 4. Revision history

Describe the revision history of RZ/G OPTEE-TA FWU.

### 4.1. v1.00

- First release.
