obj-m	:= keap.o
subdir-ccflags-y := -I$(src)/include
subdir-ccflags-y += -Wall -g

EXTRA_CFLAGS += -Wno-frame-address

PWD := $(shell pwd)

modules:
	$(MAKE) -C '$(LINUX_DIR)' M='$(PWD)' modules
clean:
	$(MAKE) -C '$(LINUX_DIR)' M='$(PWD)' clean
