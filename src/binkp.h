/******************************************************************
 * BinkP protocol defines. by sisoft\\trg'2003.
 * $Id: binkp.h,v 1.5 2003/10/05 15:01:19 sisoft Exp $
 ******************************************************************/
#ifndef __BINKP_H__
#define __BINKP_H__

#define BP_VERSION	"binkp/1.1"
#define BP_BLKSIZE	(4*1024u) /* block size */
#define BP_TIMEOUT	300    /* session timeout */
#define BP_BUFFER	32770  /* buffer size */

/* options */
#define O_NO	0
#define O_WANT	1
#define O_WE	2
#define O_THEY	4
#define O_NEED	8
#define O_EXT	16
#define O_YES	32

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
