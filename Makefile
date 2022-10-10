#declare the variable
obj-m += simple.o

all
	make -C libmodules$(shell uname -r)build M=$(PWD) modules
clean
	make -C libmodules$(shell uname -r)build M=$(PWD) clean
test
	sudo dmesg -C
	sudo insmod simple.ko
	sudo rmmod simple.ko
	dmesg