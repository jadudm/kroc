
# KRoC

The Kent Retargetable occam Compiler (KRoC) is an occam/occam-pi language platform, comprised of an occam compiler, native-code translator and supporting run-time system.

## Prerequisites

To compile and use KRoC, you will need to have the following already installed on your machine:

* the bash shell
* the GNU toolchain -- i.e. GCC 2.95.2 or later, binutils 2.0 or later, GNU awk, GNU make
* pkg-config
* the C development headers
* Python 2.4 or later
* xsltproc

Some occam-pi modules can optionally make use of other native libraries if available:

* libpng
* SDL and SDL_Sound
* OpenGL
* MySQL
* Player

To check out and update KRoC from the Git repository, you will also need:

* Git
* automake 1.8 or later, autoconf 2.52 or later (but not autoconf 2.64, which is buggy)

On Debian-based systems, we suggest installing the following packages:

```bash
apt-get install bash gcc binutils gawk make automake autoconf pkg-config \
libc6-dev libsdl1.2-dev libsdl-sound1.2-dev libgl1-mesa-dev \
libmysqlclient15-dev libpng12-dev libxmu-dev libxi-dev \
libplayercore2-dev libplayerc2-dev libltdl3-dev \
perl python xsltproc git
```

On x86-64 Debian systems, you will also need the packages that allow 32-bit binaries to be compiled:

```bash
apt-get install libc6-dev-i386 lib32gcc1 gcc-multilib
```


## Installation

For installation instructions, please see:

http://projects.cs.kent.ac.uk/projects/kroc/trac/wiki/Installation


## Configuration

Before using KRoC, remember to source the relevant setup file:

* ". PREFIX/bin/kroc-setup.sh" for Bourne-style shells such as bash

* "source PREFIX/bin/kroc-setup.csh" for C-style shells such as csh/tcsh

It may be convenient to include this command in your shell's startup file (e.g. "~/.bash_profile", "~/.cshrc", or "/etc/profile").


## Directories

  demos/

    Some standalone demo programs written in occam-pi.

  doc/

    Various items of documentation. The "essentially-kroc.txt" file is a
    good place to start.

  install/

    This is the default installation directory for KRoC.

  modules/

    Reusable modules that can be used in occam-pi programs. Each module
    contains a "libsrc" directory with the module's source code, and an
    "examples" directory with demonstration and test programs.

    The "modules/course" directory will probably be of particular
    interest, since it contains the examples and exercises used to teach
    occam-pi at the University of Kent.

  runtime/

    Low-level runtime support code for the occam-pi environment.

  tests/

    Functional test programs for occam-pi systems.

  tools/

    Tools for compiling, debugging, packaging and documenting occam-pi
    programs.

  licenses/

    Copies of relevant software licenses (see Licensing below).

Note: most directories contain further README files.


## Licensing

KRoC is free software. In general, tools are made available under the
GNU General Public License (v2 or later), and libraries are made
available under the GNU Lesser General Public License (v2 or later).
Information about the GNU licenses can be found at:

http://www.fsf.org/licensing/


## Reporting problems

Known bugs are given in the top-level BUGS file. If KRoC fails to
compile or work correctly on your system, please mail our bug tracking
system at <kroc-bugs@kent.ac.uk>, including a brief description of the
problem and a copy of the "typescript" file generated by "build".
