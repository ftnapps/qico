/* MD5Q.H - header file for MD5Q.C
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

/*
 * $Id: md5q.h,v 1.1 2005/03/28 15:33:56 mitry Exp $
 *
 * $Log: md5q.h,v $
 * Revision 1.1  2005/03/28 15:33:56  mitry
 * MD5 logic now in separate files
 *
 */


#include "config.h"

#ifndef HAVE_LIBMD

/* RFC 1321              MD5 Message-Digest Algorithm            April 1992 */

/* RSAREF types and constants
 */

/* PROTOTYPES should be set to one if and only if the compiler supports
  function argument prototyping.

  The following makes PROTOTYPES default to 0 if it has not already
  been defined with C compiler flags.
 */
#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
#ifdef UINT16
typedef UINT16 UINT2;
#else
typedef unsigned short int UINT2;
#endif

/* UINT4 defines a four byte word */
#ifdef UINT16
typedef UINT32 UINT4;
#else
typedef unsigned long int UINT4;
#endif

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
  returns an empty list.
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif


/* MD5 context. */
typedef struct {
  UINT4 state[4];                                   /* state (ABCD) */
  UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;


#endif /* HAVE_LIBMD */

#define MD5_DIGEST_LEN 16
#define MD_CHALLENGE_LEN 16

typedef unsigned char md_caddr_t[MD5_DIGEST_LEN];

extern void md5_cram_get(const unsigned char *secret, const unsigned char *challenge,
         int challenge_length, unsigned char *digest);
extern void md5_cram_set(const unsigned char *challenge);

extern unsigned char *md5_challenge(const unsigned char *buf);
extern char *md5_digest(const char *pwd, const unsigned char *challenge);
