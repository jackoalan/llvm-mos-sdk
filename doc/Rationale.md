# Strategy

Strategies for developing this SDK are as follows:

- **Out-of-box experience** should work as expected for Windows, MacOS, and
Ubuntu platforms.  The compiler should be downloaded if needed, but the paranoid
should be given sufficient instructions to build it themselves from source.
- Depend on as **few external tools** as possible.  Try to depend on tools that
are already likely to be present on the system.  LLVM requires CMake and Python
to be present, but we shouldn't require Python per se to get up and running.
Set up a method for calling CMake exactly once, and then permit the user to use 
per-platform configuration scripts. (mysterymath's .cfg idea)
- **Show, don't tell.** Copious example getting-started type programs, that
run on as many platforms as possible.
- **Extremely DRY design.**  In particular, generate linker scripts per platform
based on a simple CMake function call, and compute the --num-zp-regs flag
to the compiler.
- **Test suite.**  If you have MAME installed, you should be able to run all the 
programs on emulated machines by building a specific CMake target.
- **Debug support.** The SDK should facilitate running lldb in conjunction with
MAME, so that LLVM-MOS programs may be debugged at the source level or the
assembly level.
- **At least one libc, and hopefully more.** It's reasonable to expect a stubbed
version of libc (notlibc), which at least permits programs to compile.  It's 
also reasonable to expect a low-fat version of libc, such as
[picolibc](https://github.com/picolibc/picolibc), to work with most or all of
the supported targets, such that link-time optimization keeps the code size to
a pay-for-what-you-use minimum.

# Non-goals

- **Hardware support outside of the C standard.** While this is *absolutely* a
worthwhile goal, we see such a project as *dependent on* this SDK, and not
necessarily part of it.  It would be very reasonable to create a downstream
set of cross-platform graphics or audio libraries, as cc65 has done, and 
we'd be very happy to work alongside such a project.
