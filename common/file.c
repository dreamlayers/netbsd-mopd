/*
 * Copyright (c) 1995-96 Mats O Jansson.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mats O Jansson.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LINT
static char rcsid[] = "$Id: file.c,v 1.4 1996/08/16 22:39:22 moj Exp $";
#endif

#include "os.h"
#include "common.h"
#include "mopdef.h"

#ifndef NOAOUT
#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/exec_aout.h>
#endif
#if defined(__bsdi__)
#define NOAOUT
#endif
#if defined(__FreeBSD__)
#include <sys/imgact_aout.h>
#endif
#if defined(__linux__)
#include <a.out.h>
/*
 * Linux provides appropriate structures but access macros
 * have different names and BSD machine ids are missing.
 */
#define N_GETMID	N_MACHTYPE
#define N_GETMAGIC	N_MAGIC
#define MID_SUN010	M_68010	/* sun 68010/68020 binary */
#define MID_SUN020	M_68020	/* sun 68020-only binary */
#define MID_PC386	M_386	/* 386 PC binary */
#define MID_I386	134	/* i386 binary */
#define MID_M68K	135	/* m68k binary with 8K page sizes */
#define MID_M68K4K	136	/* m68k binary with 4K page sizes */
#define MID_NS32532	137	/* ns32532 binary */
#define MID_SPARC	138	/* sparc binary */
#define MID_PMAX	139	/* pmax (little-endian MIPS) binary */
#define MID_VAX		140	/* vax binary */
#define MID_ALPHA	141	/* Alpha binary */
#define MID_MIPS	142	/* big-endian MIPS binary */
#define MID_ARM6	143	/* ARM6 binary */
#endif
#if !defined(MID_VAX)
#define MID_VAX 140
#endif
#endif

#ifndef NOELF
#include <libelf/libelf.h>
#endif

struct mopphdr {
	off_t	offset;
	u_long	paddr;
	u_long	filesz;
	u_long	memsz;
	u_long	fill;
};

struct mopphdrs {
	struct mopphdr p[MAXSEG];
};

int fileinfo;

void
mopFilePutLX(buf, index, value, cnt)
	u_char	*buf;
	int	index, cnt;
	u_long	value;
{
	int i;
	for (i = 0; i < cnt; i++) {
		buf[index+i] = value % 256;
		value = value / 256;
	}
}

void
mopFilePutBX(buf, index, value, cnt)
	u_char	*buf;
	int	index, cnt;
	u_long	value;
{
	int i;
	for (i = 0; i < cnt; i++) {
		buf[index+cnt-1-i] = value % 256;
		value = value / 256;
	}
}

u_long
mopFileGetLX(buf, index, cnt)
	u_char	*buf;
	int	index, cnt;
{
	u_long ret = 0;
	int i;

	for (i = 0; i < cnt; i++) {
		ret = ret*256 + buf[index+cnt-1-i];
	}

	return(ret);
}

u_long
mopFileGetBX(buf, index, cnt)
	u_char	*buf;
	int	index, cnt;
{
	u_long ret = 0;
	int i;

	for (i = 0; i < cnt; i++) {
		ret = ret*256 + buf[index+i];
	}

	return(ret);
}

void
mopFileSwapX(buf, index, cnt)
	u_char	*buf;
	int	index, cnt;
{
	int i;
	u_char c;

	for (i = 0; i < (cnt / 2); i++) {
		c = buf[index+i];
		buf[index+i] = buf[index+cnt-1-i];
		buf[index+cnt-1-i] = c;
	}

}

int
CheckMopFile(fd)
	int	fd;
{
	u_char	header[512];
	short	image_type;

	(void)lseek(fd, (off_t) 0, SEEK_SET);

	if (read(fd, header, 512) != 512)
		return(-1);

	image_type = (u_short)(header[IHD_W_ALIAS+1]*256 +
			       header[IHD_W_ALIAS]);

	switch(image_type) {
		case IHD_C_NATIVE:		/* Native mode image (VAX)   */
		case IHD_C_RSX:			/* RSX image produced by TKB */
		case IHD_C_BPA:			/* BASIC plus analog         */
		case IHD_C_ALIAS:		/* Alias		     */
		case IHD_C_CLI:			/* Image is CLI		     */
		case IHD_C_PMAX:		/* PMAX system image	     */
		case IHD_C_ALPHA:		/* ALPHA system image	     */
			break;
		default:
			return(-1);
	}

	return(0);
}

int
GetMopFileInfo(fd, load, xfr, ftype, seg)
	int	fd, *ftype;
	u_long	*load, *xfr;
	struct segs *seg;
{
	u_char	header[512];
	short	image_type;
	u_long	load_addr, xfr_addr, isd, iha;
	off_t	hbcnt, isize;

	(void)lseek(fd, (off_t) 0, SEEK_SET);

	if (read(fd, header, 512) != 512)
		return(-1);

	image_type = (u_short)(header[IHD_W_ALIAS+1]*256 +
			       header[IHD_W_ALIAS]);

	switch(image_type) {
		case IHD_C_NATIVE:		/* Native mode image (VAX)   */
			isd = (header[IHD_W_SIZE+1]*256 +
			       header[IHD_W_SIZE]);
			iha = (header[IHD_W_ACTIVOFF+1]*256 +
			       header[IHD_W_ACTIVOFF]);
			hbcnt = (header[IHD_B_HDRBLKCNT]);
			isize = (header[isd+ISD_W_PAGCNT+1]*256 +
				 header[isd+ISD_W_PAGCNT]) * 512;
			load_addr = ((header[isd+ISD_V_VPN+1]*256 +
				      header[isd+ISD_V_VPN]) & ISD_M_VPN)
					* 512;
			xfr_addr = (header[iha+IHA_L_TFRADR1+3]*0x1000000 +
				    header[iha+IHA_L_TFRADR1+2]*0x10000 +
				    header[iha+IHA_L_TFRADR1+1]*0x100 +
				    header[iha+IHA_L_TFRADR1]) & 0x7fffffff;
			if (fileinfo) {
				printf("Native Image (VAX)\n");
				printf("Header Block Count: %d\n",hbcnt);
				printf("Image Size:         %08x\n",isize);
				printf("Load Address:       %08x\n",load_addr);
				printf("Transfer Address:   %08x\n",xfr_addr);
			}
			break;
		case IHD_C_RSX:			/* RSX image produced by TKB */
			hbcnt = header[L_BBLK+1]*256 + header[L_BBLK];
			isize = (header[L_BLDZ+1]*256 + header[L_BLDZ]) * 64;
			load_addr = header[L_BSA+1]*256 + header[L_BSA];
			xfr_addr  = header[L_BXFR+1]*256 + header[L_BXFR];
			if (fileinfo) {
				printf("RSX Image\n");
				printf("Header Block Count: %d\n",hbcnt);
				printf("Image Size:         %08x\n",isize);
				printf("Load Address:       %08x\n",load_addr);
				printf("Transfer Address:   %08x\n",xfr_addr);
			}
			break;
		case IHD_C_BPA:			/* BASIC plus analog         */
			if (fileinfo) {
				printf("BASIC-Plus Image, not supported\n");
			}
			return(-1);
			break;
		case IHD_C_ALIAS:		/* Alias		     */
			if (fileinfo) {
				printf("Alias, not supported\n");
			}
			return(-1);
			break;
		case IHD_C_CLI:			/* Image is CLI		     */
			if (fileinfo) {
				printf("CLI, not supported\n");
			}
			return(-1);
			break;
		case IHD_C_PMAX:		/* PMAX system image	     */
			isd = (header[IHD_W_SIZE+1]*256 +
			       header[IHD_W_SIZE]);
			iha = (header[IHD_W_ACTIVOFF+1]*256 +
			       header[IHD_W_ACTIVOFF]);
			hbcnt = (header[IHD_B_HDRBLKCNT]);
			isize = (header[isd+ISD_W_PAGCNT+1]*256 +
				 header[isd+ISD_W_PAGCNT]) * 512;
			load_addr = (header[isd+ISD_V_VPN+1]*256 +
				     header[isd+ISD_V_VPN]) * 512 | 0x80000000;
			xfr_addr = (header[iha+IHA_L_TFRADR1+3]*0x1000000 +
				    header[iha+IHA_L_TFRADR1+2]*0x10000 +
				    header[iha+IHA_L_TFRADR1+1]*0x100 +
				    header[iha+IHA_L_TFRADR1]);
			if (fileinfo) {
				printf("PMAX Image \n");
				printf("Header Block Count: %d\n",hbcnt);
				printf("Image Size:         %08x\n",isize);
				printf("Load Address:       %08x\n",load_addr);
				printf("Transfer Address:   %08x\n",xfr_addr);
			}
			break;
		case IHD_C_ALPHA:		/* ALPHA system image	     */
			isd = (header[EIHD_L_ISDOFF+3]*0x1000000 +
			       header[EIHD_L_ISDOFF+2]*0x10000 +
			       header[EIHD_L_ISDOFF+1]*0x100 +
			       header[EIHD_L_ISDOFF]);
			hbcnt = (header[EIHD_L_HDRBLKCNT+3]*0x1000000 +
				 header[EIHD_L_HDRBLKCNT+2]*0x10000 +
				 header[EIHD_L_HDRBLKCNT+1]*0x100 +
				 header[EIHD_L_HDRBLKCNT]);
			isize = (header[isd+EISD_L_SECSIZE+3]*0x1000000 +
				 header[isd+EISD_L_SECSIZE+2]*0x10000 +
				 header[isd+EISD_L_SECSIZE+1]*0x100 +
				 header[isd+EISD_L_SECSIZE]);
			load_addr = 0;
			xfr_addr = 0;
			if (fileinfo) {
				printf("Alpha Image \n");
				printf("Header Block Count: %d\n",hbcnt);
				printf("Image Size:         %08x\n",isize);
				printf("Load Address:       %08x\n",load_addr);
				printf("Transfer Address:   %08x\n",xfr_addr);
			}
			break;
		default:
			if (fileinfo) {
				printf("Unknown Image (%d)\n",image_type);
			}
			return(-1);
	}

	if (load != NULL) {
		*load = load_addr;
	}

	if (xfr != NULL) {
		*xfr  = xfr_addr;
	}

	if (seg != NULL) {
		off_t	hsize;

		hsize = hbcnt * 512;
		if (!hsize || !isize) {
			hsize = 512;
			isize = lseek(fd, (off_t) 0, SEEK_END);
			if (isize < 0)
				return(-1);
			isize -= hsize;
		}

		seg->s[SEG_TEXT].seek = hsize;
		seg->s[SEG_TEXT].data = isize;
	}

	if (ftype != NULL) {
		*ftype = FTYPE_MOP;
	}

	return(0);
}

#ifndef NOAOUT
int
getMID(old_mid,new_mid)
	int	old_mid, new_mid;
{
	int	mid;

	mid = old_mid;

	switch (new_mid) {
	case MID_I386:
	case MID_SPARC:
#ifdef MID_SUN010
	case MID_SUN010:
#endif
#ifdef MID_SUN020
	case MID_SUN020:
#endif
#ifdef MID_PC386
	case MID_PC386:
#endif
#ifdef MID_M68K
	case MID_M68K:
#endif
#ifdef MID_M68K4K
	case MID_M68K4K:
#endif
#ifdef MID_NS32532
	case MID_NS32532:
#endif
#ifdef MID_PMAX
	case MID_PMAX:
#endif
#ifdef MID_VAX
	case MID_VAX:
#endif
#ifdef MID_ALPHA
	case MID_ALPHA:
#endif
#ifdef MID_MIPS
	case MID_MIPS:
#endif
#ifdef MID_ARM6
	case MID_ARM6:
#endif
#ifdef M_SPARC
	case M_SPARC:
#endif
#ifdef M_ARM
	case M_ARM:
#endif
#ifdef M_MIPS1
	case M_MIPS1:
#endif
#ifdef M_MIPS2
	case M_MIPS2:
#endif
		mid = new_mid;
		break;
	default:
		;
	}

	return(mid);
}

int
getCLBYTES(mid)
	int	mid;
{
	int	clbytes;

	switch (mid) {
#ifdef MID_VAX
	case MID_VAX:
		clbytes = 1024;
		break;
#endif
	case MID_I386:
#ifdef MID_M68K4K
	case MID_M68K4K:
#endif
#ifdef MID_NS32532
	case MID_NS32532:
#endif
	case MID_SPARC:				/* It might be 8192 */
#ifdef MID_PMAX
	case MID_PMAX:
#endif
#ifdef MID_MIPS
	case MID_MIPS:
#endif
#ifdef MID_ARM6
	case MID_ARM6:
#endif
		clbytes = 4096;
		break;
#ifdef MID_M68K
	case MID_M68K:
#endif
#ifdef MID_ALPHA
	case MID_ALPHA:
#endif
#if defined(MID_M68K) || defined(MID_ALPHA)
		clbytes = 8192;
		break;
#endif
	default:
		clbytes = 0;
	}

	return(clbytes);
}
#endif

/*###406 [cc] syntax error before `int'%%%*/
int
CheckAOutFile(fd)
	int	fd;
{
#ifdef NOAOUT
	return(-1);
#else
	struct exec ex, ex_swap;
	int	mid = -1;

	(void)lseek(fd, (off_t) 0, SEEK_SET);
	
/*###416 [cc] `fd' undeclared (first use this function)%%%*/
	if (read(fd, (char *)&ex, sizeof(ex)) != sizeof(ex))
		return(-1);

	bcopy(&ex, &ex_swap, sizeof(ex_swap));
	mopFileSwapX((u_char *)&ex_swap, 0, 4);

	mid = getMID(mid, N_GETMID (ex));

	if (mid == -1) {
		mid = getMID(mid, N_GETMID (ex_swap));
	}

	if (mid != -1) {
		return(0);
	} else {
		return(-1);
	}
#endif NOAOUT
}

/*###440 [cc] syntax error before `int'%%%*/
int
GetAOutFileInfo(fd, load, xfr, ftype, seg)
	int	fd, *ftype;
	u_long	*load, *xfr;
	struct segs *seg;
{
#ifdef NOAOUT
	return(-1);
#else
	struct exec ex, ex_swap;
	int	mid = -1;
	u_long	magic, clbytes, clofset;

	(void)lseek(fd, (off_t) 0, SEEK_SET);

	if (read(fd, (char *)&ex, sizeof(ex)) != sizeof(ex))
		return(-1);

	bcopy(&ex, &ex_swap, sizeof(ex_swap));
	mopFileSwapX((u_char *)&ex_swap, 0, 4);

	mid = getMID(mid, N_GETMID (ex));

	if (mid == -1) {
		mid = getMID(mid, N_GETMID (ex_swap));
		if (mid != -1) {
			mopFileSwapX((u_char *)&ex, 0, 4);
		}
	}

	if (mid == -1) {
		return(-1);
	}

	if (N_BADMAG (ex)) {
		return(-1);
	}

	switch (mid) {
	case MID_I386:
#ifdef MID_NS32532
	case MID_NS32532:
#endif
#ifdef MID_PMAX
	case MID_PMAX:
#endif
#ifdef MID_VAX
	case MID_VAX:
#endif
#ifdef MID_ALPHA
	case MID_ALPHA:
#endif
#ifdef MID_ARM6
	case MID_ARM6:
#endif
		ex.a_text  = mopFileGetLX((u_char *)&ex_swap,  4, 4);
		ex.a_data  = mopFileGetLX((u_char *)&ex_swap,  8, 4);
		ex.a_bss   = mopFileGetLX((u_char *)&ex_swap, 12, 4);
		ex.a_syms  = mopFileGetLX((u_char *)&ex_swap, 16, 4);
		ex.a_entry = mopFileGetLX((u_char *)&ex_swap, 20, 4);
		ex.a_trsize= mopFileGetLX((u_char *)&ex_swap, 24, 4);
		ex.a_drsize= mopFileGetLX((u_char *)&ex_swap, 28, 4);
		break;
#ifdef MID_M68K
	case MID_M68K:
#endif
#ifdef MID_M68K4K
	case MID_M68K4K:
#endif
	case MID_SPARC:
#ifdef MID_MIPS
	case MID_MIPS:
#endif
		ex.a_text  = mopFileGetBX((u_char *)&ex_swap,  4, 4);
		ex.a_data  = mopFileGetBX((u_char *)&ex_swap,  8, 4);
		ex.a_bss   = mopFileGetBX((u_char *)&ex_swap, 12, 4);
		ex.a_syms  = mopFileGetBX((u_char *)&ex_swap, 16, 4);
		ex.a_entry = mopFileGetBX((u_char *)&ex_swap, 20, 4);
		ex.a_trsize= mopFileGetBX((u_char *)&ex_swap, 24, 4);
		ex.a_drsize= mopFileGetBX((u_char *)&ex_swap, 28, 4);
		break;
	default:
/*###525 [cc] syntax error before `}'%%%*/
	}

	if (fileinfo) {
		printf("a.out image (");
		switch (N_GETMID (ex)) {
		case MID_I386:
			printf("i386");
			break;
#ifdef MID_M68K
		case MID_M68K:
			printf("m68k");
			break;
#endif
#ifdef MID_M68K4K
		case MID_M68K4K:
			printf("m68k 4k");
			break;
#endif
#ifdef MID_NS32532
		case MID_NS32532:
			printf("pc532");
			break;
#endif
		case MID_SPARC:
			printf("sparc");
			break;
#ifdef MID_PMAX
		case MID_PMAX:
			printf("pmax");
			break;
#endif
#ifdef MID_VAX
		case MID_VAX:
			printf("vax");
			break;
#endif
#ifdef MID_ALPHA
		case MID_ALPHA:
			printf("alpha");
			break;
#endif
#ifdef MID_MIPS
		case MID_MIPS:
			printf("mips");
			break;
#endif
#ifdef MID_ARM6
		case MID_ARM6:
			printf("arm32");
			break;
#endif
		default:
		}
		printf(") Magic: ");
		switch (N_GETMAGIC (ex)) {
		case OMAGIC:
			printf("OMAGIC");
			break;
		case NMAGIC:
			printf("NMAGIC");
			break;
		case ZMAGIC:
			printf("ZMAGIC");
			break;
		case QMAGIC:
			printf("QMAGIC");
			break;
		default:
			printf("Unknown %d",N_GETMAGIC (ex));
		}
		printf("\n");
		printf("Size of text:       %08x\n",ex.a_text);
		printf("Size of data:       %08x\n",ex.a_data);
		printf("Size of bss:        %08x\n",ex.a_bss);
		printf("Size of symbol tab: %08x\n",ex.a_syms);
		printf("Transfer Address:   %08x\n",ex.a_entry);
		printf("Size of reloc text: %08x\n",ex.a_trsize);
		printf("Size of reloc data: %08x\n",ex.a_drsize);
	}
	magic = N_GETMAGIC (ex);
	clbytes = getCLBYTES(mid);
	clofset = clbytes - 1;

/*###608 [cc] `load' undeclared (first use this function)%%%*/
	if (load != NULL) {
		*load   = N_TXTADDR(ex);
	}

/*###612 [cc] `xfr' undeclared (first use this function)%%%*/
	if (xfr != NULL) {
		*xfr    = ex.a_entry;
	}

/*###616 [cc] `seg' undeclared (first use this function)%%%*/
	if (seg != NULL) {
		u_long	fill;

		seg->s[SEG_TEXT].seek = N_TXTOFF(ex);
		seg->s[SEG_TEXT].data = ex.a_text;
		if (magic == ZMAGIC || magic == NMAGIC) {
			fill = clbytes - (ex.a_text & clofset);
			if (fill == clbytes) {
				fill = 0;
			}
			seg->s[SEG_TEXT].fill = fill;
	        }

		seg->s[SEG_DATA].seek = N_DATOFF(ex);
		seg->s[SEG_DATA].data = ex.a_data;
		if (magic == ZMAGIC || magic == NMAGIC) {
			fill = clbytes - (ex.a_data & clofset);
			if (fill == clbytes) {
				fill = 0;
			}
			seg->s[SEG_DATA].fill = fill;
	        }

		if (magic == ZMAGIC || magic == NMAGIC) {
			fill = clbytes - (ex.a_bss & clofset);
	        } else {
			fill = clbytes -
			       ((ex.a_text + ex.a_data + ex.a_bss) & clofset);
	        }
		if (fill == clbytes) {
			fill = 0;
		}
		seg->s[SEG_BSS].fill = ex.a_bss + fill;
	}

/*###665 [cc] `ftype' undeclared (first use this function)%%%*/
	if (ftype != NULL) {
		*ftype = FTYPE_AOUT;
	}

	return(0);
#endif NOAOUT
}

#ifndef NOELF
void
sortELFphdrs(p, l, r)
	struct mopphdrs *p;
	int	l, r;
{
	struct mopphdr tmp;
	int	lp = l, rp = r, mp = (l + r) / 2;

	do {
		while (p->p[lp].paddr < p->p[mp].paddr)
			lp++;
		while (p->p[mp].paddr < p->p[rp].paddr)
			rp--;
		if (lp <= rp) {
			tmp = p->p[lp];
			p->p[lp] = p->p[rp];
			p->p[rp] = tmp;
			lp++;
			rp--;
		}
	} while (lp <= rp);
	if (l < rp)
		sortELFphdrs(p, l, rp);
	if (lp < r)
		sortELFphdrs(p, lp, r);
}
#endif /* NOELF */

int
CheckELFFile(fd)
	int	fd;
{
#ifdef NOELF
	return(-1);
#else
	Elf	*elf;

	if (elf_version(EV_CURRENT) == EV_NONE)
		return(-1);

	elf = elf_begin(fd, ELF_C_READ, NULL);
	if (!elf)
		return(-1);

	if (elf_kind(elf) != ELF_K_ELF) {
		elf_end(elf);
		return(-1);
	}

	elf_end(elf);
	return(0);
#endif /* NOELF */
}

int
GetELFFileInfo(fd, load, xfr, ftype, seg)
	int	fd, *ftype;
	u_long	*load, *xfr;
	struct segs *seg;
{
#ifdef NOELF
	return(-1);
#else
	struct mopphdrs p;
	Elf	*elf;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	Elf64_Ehdr *e64hdr;
	Elf64_Phdr *p64hdr;
	char	ei_class;
	char	ei_data;
	int	e_type;
	u_long	e_entry;
	int	e_phnum;
	long	p_type;
	int	i, j;

	elf = elf_begin(fd, ELF_C_READ, NULL);
	if (!elf)
		return(-1);

	ehdr = elf32_getehdr(elf);
	phdr = elf32_getphdr(elf);
	e64hdr = elf64_getehdr(elf);
	p64hdr = elf64_getphdr(elf);

	if ((!ehdr || !phdr) && (!e64hdr || !p64hdr)) {
		elf_end(elf);
		return(-1);
	}

	e_type = ehdr ? ehdr->e_type : e64hdr->e_type;
	if (e_type != ET_EXEC) {
		if (fileinfo)
			printf("ELF non-executable, not supported\n");
		elf_end(elf);
		return(-1);
	}

	e_entry = ehdr ? ehdr->e_entry : e64hdr->e_entry;

	bzero(&p, sizeof(p));

	i = 0;
	j = 0;
	e_phnum = ehdr ? ehdr->e_phnum : e64hdr->e_phnum;
	while (i < e_phnum) {

		p_type = phdr ? phdr->p_type : p64hdr->p_type;
		if (p_type == PT_DYNAMIC) {
			if (fileinfo)
				printf("ELF dynamic executable, "
				       "not supported\n");
			elf_end(elf);
			return(-1);
		}

		if (p_type == PT_LOAD) {

			if (j >= MAXSEG) {
				if (fileinfo)
					printf("ELF executable, "
					       "over %u segments, "
					       "not supported\n", MAXSEG);
				elf_end(elf);
				return(-1);
			}

			if (phdr) {
				p.p[j].offset = phdr->p_offset;
				p.p[j].paddr = phdr->p_paddr;
				p.p[j].filesz = phdr->p_filesz;
				p.p[j].memsz = phdr->p_memsz;
			} else {
				p.p[j].offset = p64hdr->p_offset;
				p.p[j].paddr = p64hdr->p_paddr;
				p.p[j].filesz = p64hdr->p_filesz;
				p.p[j].memsz = p64hdr->p_memsz;
			}

			j++;
		}

		if (phdr)
			phdr++;
		else
			p64hdr++;
		i++;
	};
	if (!j) {
		if (fileinfo)
			printf("ELF executable, no segments to load\n");
		elf_end(elf);
		return(-1);
	}

	sortELFphdrs(&p, 0, j - 1);

	for (i = 0; i < j; i++) {
		p.p[i].fill = p.p[i].memsz - p.p[i].filesz;
		if (i > 0)
			p.p[i - 1].fill += p.p[i].paddr -
					   (p.p[i - 1].paddr +
					    p.p[i - 1].memsz);
		if (seg != NULL) {
			seg->s[i].seek = p.p[i].offset;
			seg->s[i].data = p.p[i].filesz;
			seg->s[i].fill = p.p[i].fill;
			if (i > 0)
				seg->s[i - 1].fill = p.p[i - 1].fill;
		}
	};

	if (load != NULL) {
		*load	= p.p[0].paddr;
	}
	if (xfr != NULL) {
		*xfr	= e_entry;
	}

	if (fileinfo) {
		char *clstr = "";
		char *dtstr = "";

		ei_class = ehdr ? ehdr->e_ident[EI_CLASS] :
				  e64hdr->e_ident[EI_CLASS];
		ei_data = ehdr ? ehdr->e_ident[EI_DATA] :
				 e64hdr->e_ident[EI_DATA];
		switch (ei_class) {
		case ELFCLASS32:
			clstr = " 32-bit";
			break;
		case ELFCLASS64:
			clstr = " 64-bit";
			break;
		default:
			;
		}
		switch (ei_data) {
		case ELFDATA2LSB:
			dtstr = " LSB";
			break;
		case ELFDATA2MSB:
			dtstr = " MSB";
			break;
		default:
			;
		}
		printf("ELF%s%s executable\n", clstr, dtstr);
		for (i = 0; i < j; i++)
		printf("Size of seg #%02d:    %08x (+ %08x fill)\n",
			       i, p.p[i].filesz, p.p[i].fill);
		printf("Load Address:       %08x\n", p.p[0].paddr);
		printf("Transfer Address:   %08x\n", e_entry);
	}

	if (ftype != NULL) {
		*ftype = FTYPE_ELF;
	}

	elf_end(elf);

	return(0);
#endif /* NOELF */
}

/*###673 [cc] syntax error before `int'%%%*/
int
GetFileInfo(fd, load, xfr, ftype, seg)
	int	fd, *ftype;
	u_long	*load, *xfr;
	struct segs *seg;
{
	int	err;

	*ftype = FTYPE_NONE;
	bzero(seg, sizeof(*seg));

	err = CheckELFFile(fd);

	if (err == 0) {
		err = GetELFFileInfo(fd, load, xfr, ftype, seg);
		if (err != 0) {
			return(-1);
		}
		return(0);
	}

	err = CheckAOutFile(fd);

	if (err == 0) {
		err = GetAOutFileInfo(fd, load, xfr, ftype, seg);
		if (err != 0) {
			return(-1);
		}
		return(0);
	}

	err = CheckMopFile(fd);

	if (err == 0) {
		err = GetMopFileInfo(fd, load, xfr, ftype, seg);
		if (err != 0) {
			return(-1);
		}
		return(0);
	}

	return(-1);
}

ssize_t
/*###711 [cc] syntax error before `mopFileRead'%%%*/
mopFileRead(dlslot, buf)
	struct dllist *dlslot;
	u_char	*buf;
{
	ssize_t len, outlen;
	int	bsz, i;
	long	pos, notdone, total;

	bsz = dlslot->dl_bsz;
	pos = dlslot->addr;
	total = 0;
	len = 0;

	i = 0;
	while (i < MAXSEG) {
		if (pos == total)
			lseek(dlslot->ldfd, dlslot->seg.s[i].seek, SEEK_SET);

		total += dlslot->seg.s[i].data;
		if (pos < total) {
			notdone = total - pos;
			if (notdone <= bsz) {
/*###731 [cc] subscripted value is neither array nor pointer%%%*/
				outlen = read(dlslot->ldfd,&buf[len],notdone);
			} else {
/*###733 [cc] subscripted value is neither array nor pointer%%%*/
				outlen = read(dlslot->ldfd,&buf[len],bsz);
			}
			len = len + outlen;
			pos = pos + outlen;
			bsz = bsz - outlen;
		}

		dlslot->addr = pos;
		if (!bsz)
			break;

		total += dlslot->seg.s[i].fill;
		if ((bsz > 0) && (pos < total)) {
			notdone = total - pos;
			if (notdone <= bsz) {
				outlen = notdone;
			} else {
				outlen = bsz;
			}
/*###749 [cc] subscripted value is neither array nor pointer%%%*/
			bzero(&buf[len],outlen);
			len = len + outlen;
			pos = pos + outlen;
			bsz = bsz - outlen;
		}

		dlslot->addr = pos;
		if (!bsz)
			break;

		i++;
	}

	return(len);
}
/*###820 [cc] syntax error at end of input%%%*/
