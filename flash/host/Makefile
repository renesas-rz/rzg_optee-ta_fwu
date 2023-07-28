CC      ?= $(CROSS_COMPILE)gcc
LD      ?= $(CROSS_COMPILE)ld
AR      ?= $(CROSS_COMPILE)ar
NM      ?= $(CROSS_COMPILE)nm
OBJCOPY ?= $(CROSS_COMPILE)objcopy
OBJDUMP ?= $(CROSS_COMPILE)objdump
READELF ?= $(CROSS_COMPILE)readelf

OBJS = flash.o

CFLAGS += -Wall -I../ta/include -I./include
CFLAGS += -DMACHINE_${MACHINE}
LDADD += -lteec

ifneq ($(TEEC_EXPORT),)
CFLAGS += -I$(TEEC_EXPORT)/include
LDADD += -L$(TEEC_EXPORT)/lib
endif

BINARY = rzg_optee_ta_flash

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDADD)

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
