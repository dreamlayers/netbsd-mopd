# MOP server and utilities for Linux

MOP, the Maintenance Operation Protocol, was developed by DEC (Digital Equipment Corporation). It can be used with VAX, DECstation and other systems. This repository contains a partial implementation, mainly intended for use as a first step when booting NetBSD/VAX over the network. This repository is based on mopd from NetBSD 9.1, which already contained a Linux port. It contains fixes and additions to that port.

## Quick instructions

Build via `make -f Makefile.linux`. If you don't know the VAX MAC address, as root run `moptrace -a` and attempt a netboot via the VAX console. Create `/tftpboot/mop` and put boot files there, named with the MAC address of the computer being booted. The hex digits must be in lowercase, without colons, and followed by `.SYS` in uppercase, such as `08002b90649f.SYS`. Run `mopd -a` as root. Files will now be served to VAXen. Boot your VAX using the appropriate device name, such as `b xqa0` on a MicroVAX II or `b esa0` on a VAXstation 2000. 

You can restrict the programs to a specific Ethernet interface by using the interface name instead of `-a`. With the `-d` switch, `mopd` will stay in the foreground and show debug output on standard output. Otherwise see syslog. MOP is only the first step of booting NetBSD/VAX. Further steps will require other server programs, such as what is described in the [guide to netbooting NetBSD/VAX on a MicroVAX II](https://github.com/qu1j0t3/mopd/blob/master/HOWTO-MicroVAX-II.md).

## Notes

#### 802.3 transport issues

I need to disable 802.3 transport using the `-4` switch. Otherwise, KA650
and VAXstation 3100 m38 start 802.3 transfers which fail. Both also make
ordinary Ethernet transport requests, and those work.

#### Conversion of files

MOP loads files via packets which copy blocks of data to specific locations in memory, and then tells the system to start execution at a particular location. The clients know nothing about a.out or ELF, so the server needs to know how to load those files into client memory. 

This code needs NetBSD header files to read a.out and ELF files. Because Linux header files are different and won't work, previous Linux ports disabled a.out and ELF support. When built that way, mopd only works correctly with files in MOP format, such as boot.mop and boot.mopformat from NetBSD/VAX 1.4.1 and other versions from that time period. ELF and a.out files are sent improperly, leading to various error messages on the VAX, even CTRLERR, as if there is a hardware problem. This port builds ELF and a.out support on Linux using 3 slightly modified header files from NetBSD.

You can use mopcopy to convert ELF and a.out files to MOP format, but you don't need to do it when mopd supports those file formats. Use mopcopy for debugging the conversion or when preparing files for a version of `mopd` which doesn't support that file format.

#### Run mopd as root

The way mopd uses the Ethernet device requires root access.  Without root access, you get "Operation not permitted" errors, which you won't see if you run mopd as a daemon.

#### Partial MOP implementations

Both this code and the code running on the other end is a partial and possibly
quirky implementation of the MOP protocol. Making things work is not simply a
matter of following the specification. You need to send what the other side
can deal with.

#### Booting ULTRIX

This code can now boot ULTRIX 4.00. See the
[README.md in the ultrix folder](ultrix/README.md) for more information.

## Links

* [Link to where this code came from](https://github.com/NetBSD/src/tree/5e388abbac6e24c4f0af9b92c4fbae37f7a0cfcd/usr.sbin/mopd), on the GitHub mirror of NetBSD source
* An older version of this port, at [github.com/qu1j0t3/mopd](https://github.com/qu1j0t3/mopd)
* DEC [AA-D602A-TC Maintenance Operation Protocol (MOP)](http://www.bitsavers.org/www.computer.museum.uq.edu.au/pdf/AA-D602A-TC%20Maintenance%20Operation%20Protocol%20(MOP).pdf) manual
* DEC [AA-K178A-TK Maintenance Operation Protocol Functional Specification](http://decnet.ipv7.net/docs/dundas/aa-k178a-tk.pdf) manual
