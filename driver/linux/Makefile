all:
	make -C /lib/modules/`uname -r`/build M=`pwd` modules

clean:
	rm -rf *.ko *.o Module.symvers *.mod.c

install:
	rmmod usb_crts
	insmod usb_crts.ko

obj-m := usb_crts.o
