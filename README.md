boxeebox-xbmc
=============

Aiming to get xbmc up and running on the boxee box.

The build scripts will download, compile and install all dependencies,
including cross compilers. In other words, you don't need to have any
Boxeebox or XBMC related code or SDK's installed on your system and even if you had
they won't be used.

Your system needs to have cmake, make and the normal gcc suspects installed as well as
be running on a system capable of executing "configure" scripts. Your filesystem needs
to be case sensitive.

@quarnster is performing his builds on OSX 10.8.3. Pull requests (if needed) enabling this to work
on other systems are welcome.

```bash
git clone https://github.com/quarnster/boxeebox-xbmc.git
cd boxeebox-xbmc
mdkir build
cd build
cmake ..
make
```

Some reference material:

LFS: http://www.linuxfromscratch.org/lfs/view/stable/
Python cross compilation: http://randomsplat.com/id5-cross-compiling-python-for-embedded-linux.html
