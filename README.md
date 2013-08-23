boxeebox-xbmc
=============

Aiming to get xbmc up and running on the boxee box.

The build scripts will download, compile and install all dependencies,
including cross compilers. In other words, you don't need to have any
Boxeebox or XBMC related code or SDK's installed on your system and even if you had
they won't be used.

@quarnster is performing his builds on OSX 10.8.3, but occasionally tests with a Ubuntu VM.
Pull requests (if needed) enabling this to work on other systems are welcome.

# Prerequisites

Ubuntu seemed to require:
```
sudo apt-get install cmake make gcc texinfo gawk bison flex
```

I'm pretty sure my OSX brew line was similar.

Your system must be capable of executing "configure" scripts and your filesystem needs
to be case sensitive.

# Building

```bash
git clone https://github.com/quarnster/boxeebox-xbmc.git
cd boxeebox-xbmc
mdkir build
cd build
cmake ..
make
```

# Cross compilation reference material

* LFS: http://www.linuxfromscratch.org/lfs/view/stable/
* Python cross compilation: http://randomsplat.com/id5-cross-compiling-python-for-embedded-linux.html
