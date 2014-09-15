#MAKE file only compiles files that have't changed


obj-m += Hello.o
#create an object called hello.o

#kernel source code -- so we can compile the kernel 
#this compiles the source code while we are in the source code 
KDIR = /usr/src/linux-headers-3.11.0-12-generic

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod *.sysmvers *.order
