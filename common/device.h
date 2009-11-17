/*	$NetBSD: device.h,v 1.6 2009/11/17 18:58:07 drochner Exp $	*/

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
 *	$NetBSD: device.h,v 1.6 2009/11/17 18:58:07 drochner Exp $
 *
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

__BEGIN_DECLS
#ifdef	DEV_NEW_CONF
void	deviceEthAddr __P((const char *, u_char *));
#endif
void	deviceInitOne __P((const char *));
void	deviceInitAll __P((void));

/* from loop-bsd.c */
void	Loop __P((void));
int	mopOpenDL __P((struct if_info *, int));
int	mopOpenRC __P((struct if_info *, int));
void	mopReadDL __P((void));
void	mopReadRC __P((void));
__END_DECLS

#endif /* _DEVICE_H_ */
