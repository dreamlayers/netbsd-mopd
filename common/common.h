/*
 * Copyright (c) 1993-95 Mats O Jansson.  All rights reserved.
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
 *
 *	$Id: common.h,v 1.14 1996/08/13 12:22:29 moj Exp $
 *
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#define MAXDL		16		/* maximum number concurrent load */
#define IFNAME_SIZE	32		/* maximum size if interface name */
#define BUFSIZE		1600		/* main receive buffer size	*/
#define HDRSIZ		22		/* room for 803.2 header	*/

#ifndef DEFAULT_HOSTNAME
#define DEFAULT_HOSTNAME	"ipc"
#endif

#ifndef MOP_FILE_PATH
#define MOP_FILE_PATH	"/tftpboot/mop"
#endif

#define DEBUG_ONELINE	1
#define DEBUG_HEADER	2
#define DEBUG_INFO	3

/*
 * structure per interface
 *
 */

struct if_info {
	int	fd;			/* File Descriptor                 */
	int	trans;			/* Transport type Ethernet/802.3   */
	u_char	eaddr[6];		/* Ethernet addr of this interface */
	char	if_name[IFNAME_SIZE];	/* Interface Name		   */
	int	(*iopen)();		/* Interface Open Routine	   */
	int	(*write)();		/* Interface Write Routine	   */
	void	(*read)();		/* Interface Read Routine          */
	struct if_info *next;		/* Next Interface		   */
};

#define DL_STATUS_FREE		 0
#define DL_STATUS_READ_IMGHDR	 1
#define DL_STATUS_SENT_MLD	 2
#define DL_STATUS_SENT_PLT	 3

#define FTYPE_NONE	0		/* unindentified image		*/
#define FTYPE_MOP	1		/* DEC MOP image		*/
#define FTYPE_AOUT	2		/* a.out executable		*/
#define FTYPE_COFF	3		/* COFF executable		*/
#define FTYPE_ELF	4		/* ELF executable		*/

#define SEG_TEXT	0		/* Segment table entry numbers	*/
#define SEG_DATA	1		/* for file types that use	*/
#define SEG_BSS		2		/* fixed segments		*/

#define MAXSEG		16		/* Max. number of program segments */

struct seg {
	off_t	seek;			/* File offset of segment	*/
	u_long	data;			/* Size of segment data		*/
	u_long	fill;			/* Size of segment fill		*/
};

struct segs {
	struct seg s[MAXSEG];		/* File segments		*/
};

struct dllist {
	u_char	status;			/* Status byte			*/
	struct if_info *ii;		/* interface pointer		*/
	u_char	eaddr[6];		/* targets ethernet addres	*/
	int	ldfd;			/* filedescriptor for loadfile	*/
	u_short	dl_bsz;			/* Data Link Buffer Size	*/
	int	timeout;		/* Timeout counter		*/
	u_char	count;			/* Packet Counter		*/
	u_long	loadaddr;		/* Load Address			*/
	u_long	xferaddr;		/* Transfer Address		*/
	u_long	nloadaddr;		/* Next Load Address		*/
	int	ftype;			/* File type			*/
	struct segs seg;		/* File segments		*/
	u_long	addr;			/* Current relative address	*/
};

#endif _COMMON_H_
