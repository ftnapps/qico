/******************************************************************
 * BinkP protocol defines. by sisoft\\trg'2003.
 * $Id: binkp.h,v 1.2 2003/09/23 12:55:54 sisoft Exp $
 ******************************************************************/
#ifndef __BINKP_H__
#define __BINKP_H__

#define BP_VERSION	"binkp/1.1"
#define BP_BLKSIZE	(4*1024u)

/* options */
#define BP_OPT_NR       0x01   /* Non-reliable mode */
#define BP_OPT_MB       0x02   /* Multiple batch mode */
#define BP_OPT_NOPLAIN  0x04   /* Disable plain auth */
#define BP_OPT_MD5      0x08   /* CRAM-MD5 authentication */
#define BP_OPT_SHA1     0x10   /* CRAM-SHA1 authentication */
#define BP_OPT_DES      0x20   /* CRAM-DES authentication */
#define BP_OPT_CRYPT	0x40   /* Crypt traffic */
#define BP_OPT_ND       0x80   /* Non-dupes mode */

#define BP_OPTIONS	BP_OPT_MD5|BP_OPT_NOPLAIN /*|BP_OPT_MB|BP_OPT_CRYPT*/

/* outbound state */
#define BPO_Init	0
#define BPO_WaitNul	1
#define BPO_SendPwd	2
#define BPO_WaitAdr	3
#define BPO_Auth	4
#define BPO_WaitOk	5

/* inbound state */
#define BPI_Init	8
#define BPI_WaitAdr	9
#define BPI_WaitPwd	10
#define BPI_Auth	11

/* messages */
enum {
	BPM_NONE = 99,		/* No available data */
	BPM_DATA = 98,		/* Binary data */
	BPM_NUL = 0,		/* Site information */
	BPM_ADR,		/* List of addresses */
	BPM_PWD,		/* Session password */
	BPM_FILE,		/* File information */
	BPM_OK,			/* Password was acknowleged (data ignored) */
	BPM_EOB,		/* End Of Batch (data ignored) */
	BPM_GOT,		/* File received */
	BPM_ERR,		/* Misc errors */
	BPM_BSY,		/* All AKAs are busy */
	BPM_GET,		/* Get a file from offset */
	BPM_SKIP,		/* Skip a file (RECEIVE LATER) */
	BPM_MIN = BPM_NUL,	/* Minimal message type value */
	BPM_MAX = BPM_SKIP	/* Maximal message type value */
} bp_msg;


#endif
