My own Kernel, it is kind of old, just recently published. I couldn't publish the iso cuz its more then 100mb big, and github doesnt like that. Anyways building it is simple, i left lots of .cmd files which i used for compilation.
There is a Makefile i left, for easier use. If you are on windows just build the docker container and use the Makefile to compile it. I implemented ahci driver, but not a real fs(reason why lsdisks just returns unformatted bytes).
It has some basic stuff, you can use it as a base for another kernel, or your own system. Future plans: real memory management, x86_64 boot(almost done), uefi boot, implement exFAT support, and realtek 802.11n and 802.11ac wi-fi drivers.

LASTLY WORKING ON PIR TABLE CHECK FOR BETTER PCI DEVICE RECOGNITION THEN JUST SCANNING EVERY BUS.

Project state: currently stopped working
