My own Kernel, it is kind of old, just recently published. I couldn't publish the iso cuz its more then 100mb big, and github doesnt like that. Anyways building it is simple, i left lots of .cmd files which i used for compilation.
There is a Makefile i left, for easier use. If you are on windows just build the docker container and use the Makefile to compile it. I implemented ahci driver, but not a real fs(reason why lsdisks just returns unformatted bytes).
It has some basic stuff, you can use it as a base for another kernel, and now i do not have time left to write the makefile bye.
