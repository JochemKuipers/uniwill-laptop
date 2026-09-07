CFLAGS_uniwill-acpi.o := -DDEBUG
CFLAGS_uniwill-wmi.o := -DDEBUG
obj-m += uniwill-laptop.o
uniwill-laptop-y := uniwill-acpi.o uniwill-wmi.o

KDIR ?= /lib/modules/$(shell uname -r)/build

# PikaOS kernels are built with clang. gcc rejects those kbuild flags.
ifneq ($(shell grep -s CONFIG_CC_IS_CLANG=y $(KDIR)/include/config/auto.conf),)
LLVM ?= 1
endif

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) $(if $(LLVM),LLVM=$(LLVM)) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) $(if $(LLVM),LLVM=$(LLVM)) clean
