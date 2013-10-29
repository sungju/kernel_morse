kernel_morse
============

Morse sounds generator : kernel example

~~~
$ make
$ su
$ make modules_install
$ modprobe morse_io
$ echo SOS > /dev/morse0
$ rmmod morse_io
~~~
