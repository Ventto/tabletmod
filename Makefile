MODULE_NAME = tabletmod
DEBUG = 0

obj-m += $(MODULE_NAME).o

ifeq ($(KDIR),)
ifeq ($(DEBUG),1)

define MESSAGE


==================== WARNING ====================
#
# Makefile: $(MODULE_NAME).ko
#
# The `KDIR` variable is not set.
# Perhaps you compile the kernel module along the kernel tree
#
=================================================


endef

$(warning $(MESSAGE))
endif
else

.PHONY: all
all:
	make -C "$(KDIR)" M="$(PWD)" modules

.PHONY: clean
clean:
	make -C "$(KDIR)" M="$(PWD)" clean

endif
