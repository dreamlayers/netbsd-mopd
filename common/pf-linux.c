/*
 * General Purpose AppleTalk Packet Filter Interface
 *
 * Copyright (c) 1992-1995, The University of Melbourne.
 * All Rights Reserved.  Permission to redistribute or
 * use any part of this software for any purpose must
 * be obtained in writing from the copyright owner.
 *
 * This software is supplied "as is" without express
 * or implied warranty.
 *
 * djh@munnari.OZ.AU
 *
 * Supports:
 *	Linux SOCK_PACKET
 *	
 * $Author: djh $
 * $Revision: 1.21 $
 *
 *
 * Modified for use with the linux-mopd port by Karl Maftoum 
 * u963870@student.canberra.edu.au
 *
 */

#ifdef __linux__

/*
 * include header files
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/errno.h>
#include <netinet/if_ether.h>
#include <netinet/if_fddi.h>
#include <netdb.h>
#include <ctype.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include "mopdef.h"

/*
 * map compatible functions
 *
 */

#ifdef	MAPFUNCS
#define	bcopy(a,b,l)	memcpy((char *)(b),(char *)(a),(l))
#define	bcmp(a,b,l)	memcmp((char *)(a),(char *)(b),(l))
#define	bzero(a,l)	memset((char *)(a),0,(l))
#define	rindex(s,c)	strrchr((char *)(s),(c))
#define	index(s,c)	strchr((char *)(s),(c))
#endif	MAPFUNCS

/*
 * select common modules
 *
 */


#define	USE_SADDMULTI

/*
 * definitions
 *
 */

#define	READBUFSIZ	4096
#define	NUMRDS		32

struct RDS {
  u_short dataLen;
  u_char *dataPtr;
};

/*
 * variables
 *
 */

struct socklist {
  int iflen;
  struct sockaddr sa;
} socklist[32];

struct ifreq ifr;
extern int errno;
extern int nomulti;

struct RDS RDS[NUMRDS];

struct mcastent {
  char *interface;
  u_char addr[6];
  struct mcastent *next;
};

struct mcastent *mcastlist = NULL;
int mcastreg = 0;

void reg_cleanup();
void sig_cleanup();

volatile sig_atomic_t sig_in_progress = 0;
void (*hnd_hup)();
void (*hnd_int)();
void (*hnd_quit)();
void (*hnd_segv)();
void (*hnd_term)();

/*
 * establish protocol filter
 *
 */

int
setup_pf(s, prot, typ)
int s, typ;
u_short prot;
{
  int ioarg;
  u_short offset;
  return(0);
}

/*
 * Open and initialize packet filter
 * for a particular protocol type.
 *
 */


int
pfInit(interface, mode, protocol, typ)
char *interface;
u_short protocol;
int typ, mode;
{
  int s;
  int ioarg;
  char device[64];
  unsigned long if_flags;


  { u_short prot;

    prot = ((typ == TRANS_8023) ? htons(ETH_P_802_2) : htons(protocol));
    if ((s = socket(AF_INET, SOCK_PACKET, prot)) < 0) {
      syslog(LOG_ERR, "pfInit: %s: socket: %m", interface);
      return(-1);
    }
    if (s >= 32) {
      close(s);
      return(-1);
    }
  }

  /*
   * set filter for protocol and type (IPTalk, Phase 1/2)
   *
   */

  if (setup_pf(s, protocol, typ) < 0)
    return(-1);

  /*
   * set options, bind to underlying interface
   *
   */

  strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));

  /* record socket interface name and length */
  strncpy(socklist[s].sa.sa_data, interface, sizeof(socklist[s].sa.sa_data));
  socklist[s].iflen = strlen(interface);

  if (bind(s, &socklist[s].sa, sizeof(socklist[s].sa)) < 0) {
    syslog(LOG_ERR, "pfInit: %s: bind: %m", interface);
    exit(1);
  }

  return(s);
}

/*
 * get the interface ethernet address
 *
 */

int
pfEthAddr(s, interface, addr)
int s;
char *interface;
u_char *addr;
{
  int err;

  strncpy(ifr.ifr_name, interface, sizeof (ifr.ifr_name) - 1);
  ifr.ifr_name[sizeof(ifr.ifr_name)] = 0;
  ifr.ifr_addr.sa_family = AF_INET;
  s = socket(AF_INET,SOCK_DGRAM,0);
  err = ioctl(s, SIOCGIFHWADDR, &ifr);
  (void) close(s);
  if (err < 0) {
    syslog(LOG_ERR, "pfEthAddr: %s: SIOCGIFHWADDR: %m", interface);
    exit(-1);
  }
  memcpy((char *)addr, ifr.ifr_hwaddr.sa_data, 6);
  return(0);
}

/*
 * add an interface, multicast address pair
 * to the prune-on-exit list
 *
 */

void
pfRegMulti(interface, addr)
char *interface;
u_char *addr;
{
  struct mcastent **ml = &mcastlist;

  while (*ml != NULL)
    ml = &((*ml)->next);
  if ((*ml = malloc(sizeof(struct mcastent))) == NULL) {
    syslog(LOG_ERR, "pfRegMulti: %s: malloc: %m", interface);
    exit(1);
  }
  (*ml)->next = NULL;
  if (((*ml)->interface = strdup(interface)) == NULL) {
    syslog(LOG_ERR, "pfRegMulti: %s: strdup: %m", interface);
    exit(1);
  }
  memcpy(&((*ml)->addr), addr, 6);
}

/*
 * add a multicast address to the interface
 *
 */

int
pfAddMulti(s, interface, addr)
int s;
char *interface;
u_char *addr;
{
  int sock;

#ifdef	USE_SADDMULTI
  if (nomulti)
    return(0);

  strncpy(ifr.ifr_name, interface, sizeof (ifr.ifr_name) - 1);
  ifr.ifr_name[sizeof(ifr.ifr_name)] = 0;

#ifdef	UPFILT
  /* get the real interface name */
  if (ioctl(s, EIOCIFNAME, &ifr) < 0) {
    syslog(LOG_ERR, "pfAddMulti: %s: EIOCIFNAME: %m", interface);
    return(-1);
  }
#endif	UPFILT

  /*
   * open a socket, temporarily, to use for SIOC* ioctls
   *
   */
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    syslog(LOG_ERR, "pfAddMulti: %s: socket: %m", interface);
    return(-1);
  }
  if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
    syslog(LOG_ERR, "pfAddMulti: %s: SIOCGIFFLAGS: %m", interface);
    return(-1);
  }
  if (ifr.ifr_flags & IFF_MULTICAST) {
    if (!mcastreg) {
      reg_cleanup();
      mcastreg = 1;
    }
    ifr.ifr_addr.sa_family = AF_UNSPEC;
    bcopy((char *)addr, ifr.ifr_addr.sa_data, 6);
    if (ioctl(sock, SIOCADDMULTI, (caddr_t)&ifr) < 0) {
      syslog(LOG_ERR, "pfAddMulti: %s: SIOCADDMULTI: %m", interface);
      close(sock);
      return(-1);
    } else
      pfRegMulti(interface, addr);
  }
  close(sock);
#endif	USE_SADDMULTI
  return(0);
}

/*
 * delete a multicast address from the interface
 *
 */

int
pfDelMulti(s, interface, addr)
int s;
char *interface;
u_char *addr;
{
  int sock;

#ifdef	USE_SADDMULTI

  strncpy(ifr.ifr_name, interface, sizeof (ifr.ifr_name) - 1);
  ifr.ifr_name[sizeof(ifr.ifr_name)] = 0;

  /*
   * open a socket, temporarily, to use for SIOC* ioctls
   *
   */
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    syslog(LOG_ERR, "pfDelMulti: %s: socket: %m", interface);
    return(-1);
  }
  if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
    syslog(LOG_ERR, "pfDelMulti: %s: SIOCGIFFLAGS: %m", interface);
    return(-1);
  }
  if (ifr.ifr_flags & IFF_MULTICAST) {
    ifr.ifr_addr.sa_family = AF_UNSPEC;
    bcopy((char *)addr, ifr.ifr_addr.sa_data, 6);
    if (ioctl(sock, SIOCDELMULTI, (caddr_t)&ifr) < 0) {
      syslog(LOG_ERR, "pfDelMulti: %s: SIOCDELMULTI: %m", interface);
      close(sock);
      return(-1);
    }
  }
  close(sock);
#endif	USE_SADDMULTI


  return(0);
}

/*
 * remove all registered multicast memeberships
 *
 */

void
pfPruneMulti()
{
  struct mcastent **ml = &mcastlist;

  if (!mcastreg)
    return;
  mcastreg = 0;

  while (*ml != NULL) {
    pfDelMulti(-1, (*ml)->interface, (*ml)->addr);
    ml = &((*ml)->next);
  }
}

/*
 * return 1 if ethernet interface capable of multiple opens
 *
 */

int
eth_mopen(phase)
int phase;
{
  if (phase == 2)
    return(0);
  return(1);
}

/*
 * read a packet
 * Read Data Structure describes packet(s) received
 *
 */




int
pfRead(fd, buf, len)
int fd, len;
u_char *buf;
{
  int i, cc;

  int fromlen;
  struct sockaddr sa;

  RDS[0].dataLen = 0;
  fromlen = sizeof(struct sockaddr);

  if ((cc = recvfrom(fd, (char *)buf, len, 0, &sa, &fromlen)) <= 0)
    return(cc);

  /* check if from right interface */
  for (i = socklist[fd].iflen-1; i >= 0; i--)
    if (sa.sa_data[i] != socklist[fd].sa.sa_data[i])
      return(0);

  RDS[0].dataLen = cc;
  RDS[0].dataPtr = buf;
  RDS[1].dataLen = 0;

  return(cc);
}

/*
 * write a packet
 *
 */

int
pfWrite(fd, buf, len)
int fd, len;
u_char *buf;
{

#ifdef	USE_WRITEV
  struct iovec iov[2];
  iov[0].iov_base = (caddr_t)buf;
  iov[0].iov_len = 14;
  iov[1].iov_base = (caddr_t)buf+14;
  iov[1].iov_len = len-14;

  if (writev(fd, iov, 2) == len)
    return(len);

#endif	USE_WRITEV


  if (sendto(fd, buf, len, 0, &socklist[fd].sa, sizeof(struct sockaddr)) == len)
    return(len);

  return(-1);
}

/*
 * remove all registered multicast memeberships
 * when killed
 *
 */

void
sig_cleanup(sig)
int sig;
{
  void (*hnd)();
  if (sig_in_progress)
    raise(sig);
  sig_in_progress = 1;

  pfPruneMulti();

  switch(sig) {
  case SIGHUP:
    hnd = hnd_hup;
    break;
  case SIGINT:
    hnd = hnd_int;
    break;
  case SIGQUIT:
    hnd = hnd_quit;
    break;
  case SIGSEGV:
    hnd = hnd_segv;
    break;
  case SIGTERM:
    hnd = hnd_term;
    break;
  default:
    hnd = SIG_DFL;
    break;
  }
  signal(sig, hnd);
  raise(sig);
}

/*
 * register multicast clean-up functions
 *
 */

void
reg_cleanup()
{
  if (atexit(pfPruneMulti) < 0) {
    syslog(LOG_ERR, "pfAddMulti: atexit: %m");
    exit(1);
  }
  if ((hnd_hup = signal(SIGHUP, sig_cleanup)) == SIG_IGN) {
    signal(SIGHUP, SIG_IGN);
    hnd_hup = SIG_DFL;
  }
  if ((hnd_int = signal(SIGINT, sig_cleanup)) == SIG_IGN) {
    signal(SIGINT, SIG_IGN);
    hnd_int = SIG_DFL;
  }
  if ((hnd_quit = signal(SIGQUIT, sig_cleanup)) == SIG_IGN) {
    signal(SIGQUIT, SIG_IGN);
    hnd_quit = SIG_DFL;
  }
  if ((hnd_segv = signal(SIGSEGV, sig_cleanup)) == SIG_IGN) {
    signal(SIGSEGV, SIG_IGN);
    hnd_segv = SIG_DFL;
  }
  if ((hnd_term = signal(SIGTERM, sig_cleanup)) == SIG_IGN) {
    signal(SIGTERM, SIG_IGN);
    hnd_term = SIG_DFL;
  }
}

/*
 * Return information to device.c how to open device.
 * The driver requires a separate pfOpen for Ethernet type II
 * and IEEE 802.3 frames (SNAP).
 * The driver can handle FDDI IEEE 802.1H (SNAP, RFC1042) and
 * IEEE 802.2 (SNAP) frames in a single pfOpen.
 */

int
pfTrans(interface)
	char *interface;
{
  int s, err;

  strncpy(ifr.ifr_name, interface, sizeof (ifr.ifr_name) - 1);
  ifr.ifr_name[sizeof(ifr.ifr_name)] = 0;
  ifr.ifr_addr.sa_family = AF_INET;
  s = socket(AF_INET,SOCK_DGRAM,0);
  err = ioctl(s, SIOCGIFHWADDR, &ifr);
  (void) close(s);
  if (err < 0) {
    syslog(LOG_ERR, "pfTrans: %s: SIOCGIFHWADDR: %m", interface);
    exit(-1);
  }

  switch (ifr.ifr_hwaddr.sa_family) {
  case ARPHRD_ETHER:
    return TRANS_ETHER + TRANS_8023;
  case ARPHRD_FDDI:
    return TRANS_FDDI_8021H + TRANS_FDDI_8022 + TRANS_AND;
  default:
    return(0);
  }
}

#endif /* __linux__ */
