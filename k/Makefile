MODULE_NAME := dvmem
RESMAN_CORE_OBJS:=dvmem.o
RESMAN_GLUE_OBJS:=
ifneq ($(KERNELRELEASE),)    
	$(MODULE_NAME)-objs:=$(RESMAN_GLUE_OBJS) $(RESMAN_CORE_OBJS)
	obj-m := dvmem.o
else
	KDIR := ~/sm8150/android_kernel_xiaomi_sm8150/out
all:
	make -C $(KDIR) M=$(PWD) ARCH=arm64 SUBARCH=arm64 modules
clean:    
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order
endif    
