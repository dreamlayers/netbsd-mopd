/* $NetBSD$ */

/*
 * Copyright (c) 2020 Boris Gjenero.  All rights reserved.
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
 */

/*
 * This file takes a text configuration file for netbooting Ultrix and
 * creates a binary file in MOP format for use as the secondary bootstrap.
 * I don't like this very much. There should be a simpler way to write
 * a simple C struct to a file in a portable way. This is way too much code.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "mopdef.h"
#include "file.h"

enum fieldtype {
    F_STRING,
    F_IPV4ADDR,
    F_SHORT
};

const struct {
    const char *name;
    enum fieldtype type;
    int arg;
    const char *desc;
} nbinfo[] = {
    { "srvname", F_STRING, 32, "boot server hostname" },
    { "srvipadr", F_IPV4ADDR, 0, "boot server IP address" },
    { "cliname", F_STRING, 32, "client hostname" },
    { "cliipadr", F_IPV4ADDR, 0, "client IP address" },
    { "brdcst", F_IPV4ADDR, 0, "broadcast address" },
    { "netmsk", F_IPV4ADDR, 0, "network mask" },
    { "swapfs", F_SHORT, 0, "swap device (1 for NFS)" },
    { "rootfs", F_SHORT, 0, "root file system type (5 for NFS)" },
    { "swapsz", F_SHORT, 0, "swap size in 1/2 Meg units" },
    { "dmpflg", F_SHORT, 0, "dump flag 0 - disabled, 1 - enabled" },
    { "rootdesc", F_STRING, 80, "root NFS path" },
    { "swapdesc", F_STRING, 80, "swap file NFS path" },
    { "reserved", F_STRING, 20, "for later use" },
};

#define NUM_FIELDS (sizeof(nbinfo) / sizeof(nbinfo[0]))

/* Global variables avoid passing around pointers to pointers. */

static int line;
static char buf[100];

/* 512 bytes for the header and one block after that.
 * The struct is smaller, but size is in blocks.
 */
static u_char outbuf[512 * 2];
static u_char *outp = NULL;

void fatal(const char *msg) {
    fprintf(stderr, "Fatal error: %s\n", msg);
    exit(-1);
}

void fatalat(const char *ptr, const char *msg){
    int l, col = ptr - buf;
    fprintf(stderr, "Fatal error at line %i, character %i:\n", line + 1, col + 1);
    l = strlen(buf);
    if (l > 0 && buf[l-1] == '\n') l--;
    fwrite(buf, l, 1, stderr);
    fprintf(stderr, "\n");
    for (; col > 0; col--) fputc(' ', stderr);
    fprintf(stderr, "^\n%s\n", msg);
    exit(-1);
}

void print_struct(int template)
{
    int i;
    for (i = 0; i < NUM_FIELDS; i++) {
        printf("%s: ", nbinfo[i].name);
        if (template) {
            printf("\n");
        } else {
            switch(nbinfo[i].type) {
            case F_STRING:
                printf("string(%i), ", nbinfo[i].arg);
                break;
            case F_IPV4ADDR:
                fputs("IPv4 address, ", stdout);
                break;
            case F_SHORT:
                fputs("short, ", stdout);
                break;
            default:
                fputs("UNKNOWN, ", stdout);
                break;
            }
            puts(nbinfo[i].desc);
        }
    }
}

void convert_str(const char *str, int maxlen)
{
    int l = strlen(str);
    if (str[l-1] == '\n') l--;
    if (l > maxlen) fatalat(str + maxlen, "String truncated");
    memcpy(outp, str, l);
    if (l < maxlen) memset(outp + l, 0, maxlen - l);
    outp += maxlen;
}

int get_ipelem(const char **p)
{
    char *endptr;
    long l;
    l = strtol(*p, &endptr, 10);
    if (l < 0 || l > 255) fatalat(*p, "IP address number out of range");
    *p = endptr;
    return l;
}

void convert_ipv4(const char *str)
{
    int i;
    const char *p = str;

    for (i = 3; i > 0; i--) {
        *(outp+i) = get_ipelem(&p);
        if (*p != '.') fatalat(p, "Expected . in IP address");
        p++;
    }
    *outp = get_ipelem(&p);

    if (*p != '\n' && *p != 0) fatalat(p, "Junk after IP address");
    outp += 4;
}

void convert_short(const char *str)
{
    char *endptr;
    long l;
    l = strtol(str, &endptr, 10);
    if (endptr == str || (*endptr != 0 && *endptr != '\n'))
        fatalat(endptr, "Error in short value");
    if (l < -32768 || l > 0xFFFF)
        fatalat(str, "Overflow in short value");
    *(outp++) = l & 0xFF;
    *(outp++) = (l >> 8) & 0xff;
}

void fill_netblk(FILE *in)
{
    for (line = 0; line < NUM_FIELDS; line++) {
        int idx;

        if (fgets(buf, sizeof(buf), in) != buf) {
            fprintf(stderr, "Fatal error: read failed at line %i\n", line + 1);
            exit(-1);
        }
        idx = strlen(nbinfo[line].name);
        if (memcmp(buf, nbinfo[line].name, idx) ||
            buf[idx] != ':'  || buf[idx] != ':' )
            fatalat(buf, "Unrecognized label");

        switch(nbinfo[line].type) {
        case F_STRING:
            convert_str(&buf[idx+2], nbinfo[line].arg);
            break;
        case F_IPV4ADDR:
            convert_ipv4(&buf[idx+2]);
            break;
        case F_SHORT:
            convert_short(&buf[idx+2]);
            break;
        default:
            fatal("internal error: unknown field");
            break;
        }
    }
}

void fill_mophdr(void)
{
    memset(outp, 0, 512);
    /* Taken from mopcopy.c */
    mopFilePutLX(outp,IHD_W_SIZE,0xd4,2);   /* Offset to ISD section. */
    mopFilePutLX(outp,IHD_W_ACTIVOFF,0x30,2);/* Offset to 1st section.*/
    mopFilePutLX(outp,IHD_W_ALIAS,IHD_C_NATIVE,2);/* It's a VAX image.*/
    mopFilePutLX(outp,IHD_B_HDRBLKCNT,1,1); /* Only one header block. */
    /* These must be zero. They're already 0 from memset() above.
     * mopFilePutLX(header,0xd4+ISD_V_VPN,0/512,2); load Addr
     * mopFilePutLX(header,0x30+IHA_L_TFRADR1,0,4); Xfer Addr
     */
    mopFilePutLX(outp,0xd4+ISD_W_PAGCNT,1,2);/* Imagesize in blks.*/
    outp += 512;
}

void print_usage(void)
{
    printf(
"This program creates the secondary bootstrap for netbooting ULTRIX.\n"
"It uses a text configuration file as input and creates a file in\n"
"MOP format as output for serving by mopd.\n\n"
"Usage:\n"
"Convert text configuration file: mkultconf infile outfile\n"
"Use standard input instead of file: mkultconf outfile\n"
"Output template for text configuration file: mkultconf -t\n"
"\n"
"Text configuration file explanation:\n"
    );
    print_struct(0);
    exit(1);
}

FILE *safe_open(const char *name, const char *modes)
{
    FILE *f = fopen(name, modes);
    if (f == NULL) {
        fprintf(stderr, "Error opening file: ");
        perror(name);
        exit(-1);
    }
    return f;
}

void write_mopnetblk(FILE *inf, char *outfn)
{
    FILE *outf;

    outp = outbuf;
    fill_mophdr();
    fill_netblk(inf);

    outf = safe_open(outfn, "wb");
    if (fwrite(outbuf, sizeof(outbuf), 1, outf) != 1)
        fatal("failure writing to output file");
    fclose(outf);
}

int main(int argc, char **argv) {
    if (argc == 2) {
        if (!strcmp(argv[1], "-t")) {
            print_struct(1);
        } else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-?") ||
                   !strcmp(argv[1], "--help")) {
            print_usage();
            return 0;
        } else {
            write_mopnetblk(stdin, argv[1]);
        }
    } else if (argc == 3) {
        FILE *inf = safe_open(argv[1], "r");
        write_mopnetblk(inf, argv[2]);
        fclose(inf);
    } else {
        print_usage();
        return -1;
    }
    return 0;
}
