# Design

## CMake

The technical requirements of this SDK are subtle.  Some tools are intended 
to be built and run on the current build host, such as the 6502 simulator.
And many are intended to be run on a 65xx target.

CMake's good at figuring out how to compile a program to be run on the local
machine.  But it's *not* good at having multiple compiler toolchains present,
as the design of the SDK necessitates as per above.
