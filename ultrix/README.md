# Netbooting ULTRIX

ULTRIX is a Unix operating system released by DEC (Digital Equipment
Corporation) for PDP-11, VAX, MicroVAX and DECstation. This file explains
how to boot ULTRIX over the network on a VAX.

The ULTRIX boot procedure starts with 3 file loads via MOP (Maintennance
Operations Protocol). First, the VAX firmware requests a program, an operating
system. This needs to load `/usr/mdec/netload` from ULTRIX. Then netload makes
two more MOP program requests. Its first request (the second MOP request) asks
for a secondary bootstrap, which needs to be a C struct with configuration
information. This tells the kernel what IP address to use and where to find
root and swap. The last request asks for a tertiary bootstrap, which is the
ULTRIX kernel. It then passes control to the kernel. Once the kernel is
running, it uses the information from the secondary bootstrap to mount root
and swap, and begins the usual startup.

Modifications were made to mopd to make this work. The first stage is just
like loading a NetBSD 1k page size a.out binary, except the machine ID is zero.
The second and third requests send 0xFF as the length of the SOFTID field,
and expect transfers to finish with Memory Load with Transfer Address, with
the load address being required but data being optional. (The first stage won't
work with that, and continues to use Parameter Load with Transfer Address, like
before.)

## Quick instructions

If you don't know the VAX MAC address, as root run `moptrace -a` and attempt a
netboot via the VAX console. Names for boot files are based on that MAC address.
The hex digits must be in lowercase, without colons. The first stage is the 
MAC address followed `.SYS` in uppercase, such as `08002b90649f.SYS`. The
second stage has `.2` between the MAC address and `.SYS`, like
`08002b90649f.2.SYS`. The third stage has `.3` between the MAC address and
`.SYS`, like `08002b90649f.2.SYS`. Symbolic links can be used.

Obtain `/usr/mdec/netboot` from ULTRIX and use it as the first stage.

Set up NFS server with ULTRIX root directory and create a swap file there.
Build `mkultconf` here and run `mkultconf -t > template` to create a template
file for configuration. Fill out the template. More information is available
via `mkultconf -h`. Create the stage 2 MOP file using 
`mkultconf config.txt stage2.mop` and make it accessible at the stage 2 file
name.

Obtain ULTRIX kernel and make it accessible at the third stage file name. The
kernel must support netbooting. In ULTRIX 4.00, `/genvmunix` does not but
`/usr/diskless/vmunix` does. This kernel must have the same symbol table as
`/vmunix` in the root file system exported via NFS, because `ps` and other
tools read kernel memory using those symbols.

## NFS problems

#### ULTRIX tries to truncate device files

Attempts to write to devices mounted from a Linux NFS server, like 
`echo > /dev/console`, will fail. Though, it is possible to append, such as
`echo >> /dev/console`. This is because ULTRIX sends an NFS V2 SETATTR, setting
the length of `/dev/console` to zero. Linux rejects this. ULTRIX even sends this
on a read only mounted NFS file system. Clearly ULTRIX has a bug. Because of it,
you can't see `/etc/rc` output which is written to the console this way.

In Linux, functions implementing `open()` cause `O_TRUNC` to be ignored in
such cases. ULTRIX can be patched, so that `copen()` in `gfs_syscalls` acts
similarly. For ULTRIX 4.00 and maybe other versions, Change
`ca8fff0fffff50d1508f001000001207` to `d38f0070000050d38f00700000501307`.

#### ULTRIX only sees 22 bits of disk usage

From the server, ULTRIX receives a block size and numbers of total, used and
free blocks. It multiplies those numbers by the block size and then divides
by 1024. It seems VAX multiplication and division only works with 32-bit
values, and not double width ones like x86. So the sizes in bytes must fit
within a signed 32-bit number. This patch divides the block size from the
server by 1024, and then multiplies received sizes by that number. It gives the
wrong answer if the block size isn't a multiple of 1024, but it's practically
always a multiple of 1024. If it is smaller than 1024, block sizes are
multiplied by 1. Even when giving wrong results, this is better than what was
there before.

In ULTRIX 4.0 in nfs_statfs() in nfs_vfsops.o or the kernel, find:
`C5ADF0ADF450C68F0004000050D0AC0851D050A108C5ADF0ADF850C68F0004000050D0AC0851D050A10CC5ADF0ADFC50C68F0004000050D0AC0851D050A110`
and change it to:
`D0EDF0FFFFFF53C68F00040000531203D00153D0AC0855C553EDF4FFFFFFA508C553EDF8FFFFFFA50CC553EDFCFFFFFFA510D08FEFBEADDE53D00053D00053`
Test by running `df` in ULTRIX and comparing output to `df` for that file
system in Linux.

## Other notes

* To run X at startup, take a look at `man dxsession` and add that line to
`/etc/ttys`, commenting out the console `getty` line.

* `/usr/var` needs to be writable. If mounting `/usr` read only, symlink it
elsewhere.

* `/etc/rc` assumes the system is diskless if `/etc/netblk.o` exists. At least
`touch` the file so the test passes and other NFS file systems get mounted.

* `/etc/fstab` uses `:` instead of whitespace for field separators. For an
NFS filesystem you need to use `/path@server` instead of the usual
`server:/path`. NFS servers need to be in `/etc/hosts`.

* Familiar X programs like `xterm` are on the unsupported tape.
