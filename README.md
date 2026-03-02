### CS 441 Project

cnf49 | Winter 2026 | CS441

##### Building and Sample Programs

After cloning the repository, the compiler can be built with make.
First, run `cmake .` to put together the makefiles. Then, run `make`.
sample.prg and stack.prg are in the main directory and can be used for testing by running, for example, './comp stack.prg | ./ir441 exec'

##### Milestone 1

This milestone includes a parser, a conversion to IR (with tag checking for integers and pointers), and a peephole optimization. There is some code for naive SSA if partial credit is possible, but I wasn't able to get it working without errors so the functionality has been stripped from the ./comp executable.

The chosen peephole optimization prevents tag checking on 'this' since it's always a pointer. This optimization is done in the IRBuilder.h and IRBuilder.cpp in the helper functions for outputting tag checking, tag stripping, and tagging. Instead of emitting IR, if the pinhole optimization is turned on, these functions will simply pass if dealing with %this.
