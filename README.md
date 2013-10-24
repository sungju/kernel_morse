kernel_morse
============

Morse sounds generator : kernel example

$ make
$ su
$ insmod ./morse_io.ko
$ DEVID=`grep morse /proc/devices | awk '{print $1}'`
$ mknod ./morse c $DEVID 0
