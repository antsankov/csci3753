#MAKE file only compiles files that have't changed


obj-m += rkit.o
#create an object called hello.o

#kernel source code -- so we can compile the kernel 
#this compiles the source code while we are in the source code 
KDIR = /usr/src/linux-headers-3.2.0-60-generic-pae

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod *.sysmvers *.order
