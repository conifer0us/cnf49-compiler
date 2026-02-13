### CS 441 Project!

cnf49 | 2/12/2026

##### Building and Sample Programs

After cloning the repository, the compiler can be built with make.

##### Milestone 1

This milestone includes a parser, a conversion to IR (with tag checking for integers and pointers), a peephole optimization, and a naive implementation of SSA.

The chosen peephole optimization prevents tag checking on 'this' since it's always a pointer. This optimization is done in the IRBuilder.h and IRBuilder.cpp in the helper functions for outputting tag checking, tag stripping, and tagging. Instead of emitting IR, if the pinhole optimization is turned on, these functions will simply pass if dealing with %this.

#### A Note on the Name
This project is called SufferLang not because it caused me suffering. It's a repo I already had for a project I've been brainstorming for a while that will generate a new compiler for every user of a language with keywords and symbols randomized. It doesn't do that right now, but the repo is named "Sufferlang" so I can continue this project in that direction after completion of the class. 
