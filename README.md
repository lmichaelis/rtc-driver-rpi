# rtc-driver-rpi
This is a driver for the DS3231 RTC Chip.

This repository contains code that was written for a university assignment. It is a simple linux device driver for the DS3231 real-time-clock. The target hardware 
was a _Raspberry Pi 3 Model B v1.1_ (revision code `a01041`) using _Raspbian 8 (jesse)_ with Linux Kernel version `4.9.30-v7+`.

TODO: Connection of the chip with the raspberry (which GPIO pins etc)

It suports reading the time from the chip (`cat /dev/ds3231`) in the format `DD. M hh:mm:ss YYYY` (month names are german currently). It also supports writing the current time to the RTC by writing a date of format `YYYY-MM-DD hh:mm:ss` to `/dev/ds3231`.

# Compiling
To compile this code you will need a working C compiler and the linux kernel source
tree (typically located at `/lib/modules/$(shell uname -r)/build`). The kernel source tree can be installed with `sudo apt install linux-headers`. After that simply open a terminal in the `Driver` directory and type `make`. When the compilation finishes you can load the module using `sudo insmod ds3231_drv.ko`. To check if everything was loaded correctly you can examine the kernel message buffer with `dmesg | tail -n 10`. You should see something like this:
```
[82903.900727] ds3231: initializing hardware ...
[82903.902979] ds3231: setting up RTC ...
[82903.904416] ds3231: reset oscillator stop flag (oscillator was stopped).
[82903.905127] ds3231: hardware initialization completed.
```
