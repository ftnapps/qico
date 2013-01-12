/******************************************************************
 * BinkP protocol
 * $Id: binkp.h,v 1.7 2005/08/22 17:19:41 mitry Exp $
 ******************************************************************/

#ifdef WITH_BINKP

#ifndef __BINKP_H__
#define __BINKP_H__

#define BP_PROT		"binkp"
#define BP_VERSION	"1.1"
#define BP_BLKSIZE	4096			/* block size */
#define BP_TIMEOUT	300			/* session timeout */
#define BLK_HDR_SIZE    2			/* header size */
#define MAX_BLKSIZE	0x7fffu			/* max block size */
#define BP_BUFFER	( MAX_BLKSIZE + 16 )	/* buffer size */


/* options */
#define O_NO		0
#define O_WANT		1
#define O_WE		2
#define O_THEY		4
#define O_NEED		8
#define O_EXT		16
#define O_YES		32

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
    BPM_ADR,			/* List of addresses */
    BPM_PWD,			/* Session password */
    BPM_FILE,			/* File information */
    BPM_OK,			/* Password was acknowleged (data ignored) */
    BPM_EOB,			/* End Of Batch (data ignored) */
    BPM_GOT,			/* File received */
    BPM_ERR,			/* Misc errors */
    BPM_BSY,			/* All AKAs are busy */
    BPM_GET,			/* Get a file from offset */
    BPM_SKIP,			/* Skip a file (RECEIVE LATER) */
    BPM_RESERVED,		/* Reserved for later */
    BPM_CHAT,			/* For chat */
    BPM_MIN = BPM_NUL,		/* Minimal message type value */
    BPM_MAX = BPM_CHAT		/* Maximal message type value */
} bp_msg;


typedef struct _BP_MSG BPMSG;

struct _BP_MSG {
    char id;			/* Message ID */
    int len;			/* Message length (with header and id) */
    byte *msg;			/* Message itself (with header and id) */
};

typedef struct _BP_SESSION BPS;

struct _BP_SESSION {
    byte to;			/* Session: 1 - outgoing, 0 - incoming */

    byte *rx_buf;		/* Receive buffer */
    int rx_ptr;			/* Current rx pointer - receive new data here */
    int rx_size;		/* Current block size. -1 - new block */
    char *rx_mbuf;		/* Message buffer pointer */

    byte *tx_buf;		/* Transmit buffer */
    int tx_ptr;			/* Next byte to send pointer */
    int tx_left;		/* Data size left to send */

    int is_msg;			/* Receiving: 0 - data, 1 - data */

    BPMSG *mqueue;		/* Outgoing message queue */
    byte nmsgs;			/* Number of messages in queue */

    word mib;			/* Messages In Batch (MIB 3 :) */

    int init;			/* Init state (handshake) */
    int rc;			/* Return code */
    int error;			/* I/O error flag */
    int saved_errno;		/* what went wrong */

    int opt_nr;			/* Non-Reliable mode */
    int opt_nd;			/* ND mode */
    int opt_md;			/* CRAM-MD5 mode */
    int opt_cr;			/* CRypt mode */
    int opt_mb;			/* Multi-Batch mode */
    int opt_cht;		/* CHAT mode */

    byte *MD_chal;		/* CRAM challenge data */
    char *pwd;			/* */

    byte ver_major;		/* Remote binkp major number */
    byte ver_minor;		/* ... minor */

    unsigned long keys_out[3];	/* Encription keys for outbound */
    unsigned long keys_in[3];	/* Encription keys for inbound */

    byte send_file;		/* We are in send file state */
    byte recv_file;		/* ... receive file state */

    size_t txpos;		/* Transmitting file pos */
    size_t rxpos;		/* Receiving file pos */

    flist_t *oflist;		/* List of files to transmit */
    
    char *rfname;		/* Receiving file name */
    time_t rmtime;		/* ... mtime */

    int ticskip;
    int nofiles;
    int cls;
    sts_t ostatus;

    int wait_for_get;		/* In NR mode we sent -1 as file offset 
    				   and wait for M_GET */
    int wait_got;
    int wait_addr;		/* Awaiting remote address */
    int sent_eob;
    int recv_eob;
    int delay_eob;		/* Delay sending M_EOB message until 
    				   remote's one if remote is binkp/1.0 and
    				   pretends to have FREQ on us */

    ftnaddr_t *remaddr;		/* Remote address list */
};

int binkp_devfree(void);
int binkp_devsend(byte *, word);
int binkpsession(int, ftnaddr_t *);

#endif
#endif
