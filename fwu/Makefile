
export V?=0

# If _HOST specific compilers are not specified, then use CROSS_COMPILE
HOST_CROSS_COMPILE ?= $(CROSS_COMPILE)

.PHONY: all
all:
	make -C host CROSS_COMPILE=$(HOST_CROSS_COMPILE) --no-builtin-variables

.PHONY: clean
clean:
	make -C host clean
