obj-m	:= keap.o
subdir-ccflags-y := -I$(src)/include
subdir-ccflags-y += -Wall -g
 
PWD := $(shell pwd)
 
modules:
	$(MAKE) -C '$(LINUX_DIR)' M='$(PWD)' modules
clean:
	$(MAKE) -C '$(LINUX_DIR)' M='$(PWD)' clean
