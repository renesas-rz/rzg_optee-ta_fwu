
export V?=0

# If _HOST or _TA specific compilers are not specified, then use CROSS_COMPILE
HOST_CROSS_COMPILE ?= $(CROSS_COMPILE)
TA_CROSS_COMPILE ?= $(CROSS_COMPILE)

.PHONY: all
all:
	make -C host CROSS_COMPILE=$(HOST_CROSS_COMPILE) --no-builtin-variables
	make -C ta CROSS_COMPILE="$(TA_CROSS_COMPILE)" LDFLAGS=""
	
.PHONY: clean
clean:
	make -C host clean
	make -C ta clean
	
