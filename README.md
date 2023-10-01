# hc-sr04-driver
This is a kernel driver for the ubiquitous hc-sr04 ultrasonic sensor module.
The driver was tested on a RaspberryPi 3B and the kernel was built with the
[Yocto project](https://www.yoctoproject.org/), but you can as well port it
to other build systems or distributions.

For example, you can create an OpenWRT package makefile and `insmod` the
driver into an OpenWRT device. I of course didn't test this, but that's what
you could do.

To test this actual code, you need to setup your machine for Yocto or whatever
build system you have for a RaspberryPi 3B. Please see the documentation for
your build system on how to build an application or kernel module. At the time
of testing this, the `dunfell` branch of Yocto was used.

