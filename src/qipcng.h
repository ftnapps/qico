/**********************************************************
 * File: qipcng.h
 * Created at Wed Apr  4 00:05:05 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: qipcng.h,v 1.1 2001/04/24 19:21:00 lev Exp $
 **********************************************************/
#ifndef __QIPCNG_H__
#define __QIPCNG_H__

#define LOG_BUFFER_SIZE 10
#define MAX_CLIENTS		10
#define LINE_TIMEOUT	240

/* State of file transfer */
typedef struct _TRANSFERSTATE {
	char phase;				/* phase of process */
#define TXRX_PHASE_BEGIN	'b'
#define TXRX_PHASE_HSHAKE	'h'
#define TXRX_PHASE_FINFO	'i'
#define TXRX_PHASE_TRANSFER	't'
#define TXRX_PHASE_FINISH	'f'
#define TXRX_PHASE_EOT		'e'
	UINT32 maxblock;		/* Maximum block size */
	UINT32 curblock;		/* Current block size */
	UINT32 crcsize;			/* Size of CRC control sum in bits */
	UINT32 filesize;		/* current file size */
	UINT32 totalsize;		/* total transfer size */
	UINT32 filepos;			/* current file pos (bytes have been sent) */
	UINT32 totalpos;		/* total stream pos (bytes have been sent) */
	UINT32 filenum;			/* number of current file, from 1 */
	UINT32 totalfiles;		/* number of files to transfer */
	UINT32 filestarted;		/* UNIX time of file transfer start time */
	UINT32 transferstarted;	/* UNIX time of transfer starts */
	char *file;				/* Name of current file */
} txrxstate_t;

/* Information about one line */
typedef struct _LINESTATE {
	char phase;				/* Phase of session */
#define SESS_PHASE_NOPROC		'n'
#define SESS_PHASE_BEGIN		'b'
#define SESS_PHASE_CONNECT		'c'
#define SESS_PHASE_HSHAKEOUT	'o'
#define SESS_PHASE_HSHAKEIN		'i'
#define SESS_PHASE_INPROCESS	'p'
	char mode;
	UINT32 flags;			/* Different flags, see opts.h */
	UINT32 type;			/* Type of session */
	UINT32 proto;			/* Used protocol */
	txrxstate_t send;
	txrxstate_t recv;
	ninfo_t remote;
	char *tty;
	char *connect;
	char *cid;
	char *log_buffer[LOG_BUFFER_SIZE];
	/* Internal variables */
	int log_head,log_tail;
	pid_t pid;
	int socket;
	time_t lastevent;
	int clients_sock[MAX_CLIENTS];
	int clients;
	_LINESTATE *next;
	_LINESTATE *prev;
} linestate_t;

/* Line-to-manager and manager-to-line event structure */
typedef struct _EVTLAM {
	UINT32 fulllength;			/* Full length, include type and fulllength fields */
	CHAR password[8];			/* Password, zero-padded */
	/* Retranslated data begins here */
	UINT32 pid;					/* Line PID */
	UINT8 type;					/* See EVTL2M_XXXX / EVTM2L_XXXX */
	CHAR *data;					/* Data for event */
} evtlam_t;

/* Manager-to-UI and UI-to-manager event structure */
typedef struct _EVTLAU {
	UINT32 fulllength;			/* Full length, include type and fulllength fields */
	/* Retranslated data begins here */
	UINT8 type;					/* See EVTU2M_XXXX / EVTM2U_XXXX */
	CHAR *data;					/* Data for event */
} evtlau_t;

typedef int (*event_handler)(linestat_t *line, evtl2m_t *event);

/* Events from lines to manager */
#define EVTL2M_GROUP_GLOBAL		0x10	/* Very global event */
#define EVTL2M_REGISTER			0x11	/* Register new line, set tty and mode -- answer or call */
/* Signature: "sc" -- TTY,ANSWER. */
#define EVTL2M_CLOSE			0x12	/* Unregister line and delete it */
/* Signature: "" */

#define EVTL2M_GROUP_SESSION	0x20	/* Session related events */
#define EVTL2M_STAT_CONNECT		0x21	/* Modem connected, carrier detected. Speed and CID */
/* Signature: "ss"  -- CONNECT STRING,CID*/
#define EVTL2M_STAT_DETECTED	0x22	/* Mailer detected. Session type. */
/* Signature: "d"  -- SESSION TYPE */
#define EVTL2M_STAT_DATAGOT		0x23	/* We know about remote node. node_t info */
/* Signature: "d"  -- SESSION TYPE */
#define EVTL2M_STAT_DATASENT	0x24	/* Remote know about us */
#define EVTL2M_STAT_HANDSHAKED	0x25	/* Handshake finished. Protocols and options */

#define EVTL2M_GROUP_BATCH		0x30	/* Batch related events */
#define EVTL2M_SEND_STARTED		0x31	/* Send batch started */
#define EVTL2M_RECV_STARTED		0x32	/* Receive batch started */
#define EVTL2M_SEND_ENDED		0x33	/* Send batch finished */
#define EVTL2M_RECV_ENDED		0x34	/* Receive batch finished */

#define EVTL2M_GROUP_SEND		0x40	/* File send related events */
#define EVTL2M_FILE_SEND_INIT	0x41	/* Start to send new file -- send filename to peer */
#define EVTL2M_FILE_SEND_START	0x42	/* Start to send file data */
#define EVTL2M_FILE_SEND_END	0x43	/* File data finished (from position) */
#define EVTL2M_FILE_SEND_DONE	0x44	/* File finished (after END, ERROR, SKIP or SUSPEND) */
#define EVTL2M_FILE_SEND_ERROR	0x45	/* File sending error occured */
#define EVTL2M_FILE_SEND_SKIP	0x46	/* File was skipped by peer */
#define EVTL2M_FILE_SEND_SUSP	0x47	/* File was suspend by peer */
#define EVTL2M_FILE_SEND_BLOCK	0x48	/* Block was sended (size of block) */
#define EVTL2M_FILE_SEND_REPOS	0x49	/* Peer asks for repos (position) */
#define EVTL2M_FILE_SEND_BSCH	0x4a	/* Send block size changed */

#define EVTL2M_GROUP_RECV		0x50	/* File receiving related events */
#define EVTL2M_FILE_RECV_INIT	0x51	/* Start to receive new file -- receive filename from peer */
#define EVTL2M_FILE_RECV_START	0x52	/* Start to receive file data (from position) */
#define EVTL2M_FILE_RECV_END	0x53	/* File received finished */
#define EVTL2M_FILE_RECV_DONE	0x54	/* File finished (after END, ERROR, SKIP or SUSPEND) */
#define EVTL2M_FILE_RECV_ERROR	0x55	/* File receiving error occured */
#define EVTL2M_FILE_RECV_SKIP	0x56	/* File was skipped by us */
#define EVTL2M_FILE_RECV_SUSP	0x57	/* File was suspend by us */
#define EVTL2M_FILE_RECV_BLOCK	0x58	/* Block was received (size of block) */
#define EVTL2M_FILE_RECV_REPOS	0x59	/* We asks for repos (position) */
#define EVTL2M_FILE_RECV_BSCH	0x5a	/* Receive block size changed */

#define EVTL2M_GROUP_CHAT		0x60	/* Chat related events */
#define EVTL2M_CHAT_INIT		0x61	/* Remote request for chat */
#define EVTL2M_CHAT_LINE		0x62	/* Chat string received */
#define EVTL2M_CHAT_CLOSE		0x63	/* Remote close chat */

#define EVTL2M_GROUP_LOG		0x70	/* Logging related events */
#define EVTL2M_SLINE			0x71	/* Status line */
#define EVTL2M_LOG				0x72	/* Log string */

/* Events from line to manager */
#define EVTM2L_HANGUP			0		/* Hang up now */
#define EVTM2L_SKIP				1		/* Skip current file */
#define EVTM2L_SUSPEND			2		/* Suspend current file */
#define EVTM2L_CHAT_INIT		3		/* Start chat with remote */
#define EVTM2L_CHAT_LINE		4		/* Send line to remote chat window */
#define EVTM2L_CHAT_CLOSE		5		/* Close chat with remote */

/* Events and requests from UI to manager */
#define EVTU2M_GROUP_GLOBAL			0x10	/* Global events */
#define EVTU2M_LOGIN				0x11	/* New UI want to connect */
#define EVTU2M_LOGOUT				0x12	/* UI is closed */
#define EVTU2M_QUIT					0x13	/* Quit manager */

#define EVTU2M_GROUP_CONFIG			0x20	/* Configuration and line presense requests */
#define EVTU2M_CONFIG_STATION		0x21	/* Request for station (EMSI) info */
#define EVTU2M_CONFIG_TTYS			0x22	/* Request for list of configured ttys (lines) */
#define EVTU2M_CONFIG_TIME			0x23	/* Request for current time and timezone */
#define EVTU2M_CONFIG_MAILONLY		0x24	/* Request for mail-only time intervals */
#define EVTU2M_CONFIG_LINES			0x2f	/* Lines, which are active right now (with TCP/IP ones) */

#define EVTU2M_GROUP_LINE			0x30	/* Line related events and requests */
#define EVTU2M_LINE_FULL_STATUS		0x31	/* Request for full line status */
#define EVTU2M_LINE_CONNECT_STATUS	0x32	/* Request for connect status */
#define EVTU2M_LINE_REMOTE_STATUS	0x33	/* Request for remote node info */
#define EVTU2M_LINE_SEND_STATUS		0x34	/* Request for sending status */
#define EVTU2M_LINE_RECV_STATUS		0x35	/* Request for receiving status */
#define EVTU2M_LINE_CHAT_STATUS		0x36	/* Request for chat status -- enabled/disabled on/off */
#define EVTU2M_LINE_HANGUP			0x3d	/* Hang up line */
#define EVTU2M_LINE_SKIP			0x3e	/* Skip current file */
#define EVTU2M_LINE_SUSPEND			0x3f	/* Suspend current file */

#define EVTU2M_GROUP_QUEUE			0x40	/* Common queue related events and requests */
#define EVTU2M_QUEUE_GET			0x41	/* Request for outbound list */
#define EVTU2M_QUEUE_RESCAN			0x42	/* Force rescan of queue */

#define EVTU2M_GROUP_NODE			0x50	/* Single node operations group */
#define EVTU2M_NODE_POLL			0x51	/* Poll node */
#define EVTU2M_NODE_CHANGE_STATUS	0x52	/* Change status of node */
#define EVTU2M_NODE_ATTACH			0x53	/* Attach files(s) to node */
#define EVTU2M_NODE_KILL			0x54	/* Kill outbound for node */
#define EVTU2M_NODE_CALL			0x55	/* Call node on given (or any free) line */
#define EVTU2M_NODE_INFO			0x56	/* Get nodelist info about node */
#define EVTU2M_NODE_QUEUE			0x57	/* Get list of files for node */
                                      
#define EVTU2M_GROUP_CHAT			0x60	/* Chat-related eventts */
#define EVTU2M_CHAT_INIT			0x61	/* Start chat with remote */
#define EVTU2M_CHAT_LINE			0x62	/* Send chat line to remote */
#define EVTU2M_CHAT_CLOSE			0x63	/* Finish chat with remote */
#define EVTU2M_CHAT_BUFFER			0x64	/* Request for current chat buffer */

#define EVTU2M_GROUP_LOG			0x70	/* Log-related events */
#define EVTU2M_LOG_BUFFER			0x71	/* Request for log buffer */
#define EVTU2M_LOG_SLINE			0x72	/* Request for current status line */

#define EVTU2M_GROUP_REGISTER		0xf0	/* Group for registration requests */
#define EVTU2M_REG_REGMASK			0xf1	/* Register for events -- binary string for register  */
#define EVTU2M_REG_UNREGMASK		0xf2	/* Unregister for events -- binary string for unregister */

/* Events and information from manager to UI */
#define EVTM2U_GROUP_GLOBAL			0x10	/* Answers on global requests */
#define EVTM2U_HELLO				0x11	/* Login result */
#define EVTM2U_GOODBYE				0x12	/* Answer on LOGOUT */
#define EVTM2U_OK					0x13	/* Command completed Ok (may be with warning) */
#define EVTM2U_ERROR				0x14	/* Command WAS NOT completed (with error message) */

#define EVTM2U_GROUP_CONFIG			0x20	/* Configuration and line presence info */
#define EVTM2U_CONFIG_STATION		0x21	/* Station (EMSI) info */
#define EVTM2U_CONFIG_TTYS			0x22	/* List of configured ttys (lines) */
#define EVTM2U_CONFIG_TIME			0x23	/* Current time and timezone */
#define EVTM2U_CONFIG_MAILONLY		0x24	/* Mail-only time intervals */
#define EVTM2U_CONFIG_LINES			0x2f	/* Lines, which are active right now (with TCP/IP ones) */

#define EVTM2U_GROUP_LINE			0x30	/* Line related events and answers */
#define EVTM2U_LINE_CONNECT_STATUS	0x31	/* Connect status */
#define EVTM2U_LINE_REMOTE_STATUS	0x32	/* Remote node info */
#define EVTM2U_LINE_SEND_STATUS		0x33	/* Sending status */
#define EVTM2U_LINE_RECV_STATUS		0x34	/* Receiving status */
#define EVTM2U_LINE_CHAT_STATUS		0x35	/* Chat status */

#define EVTM2U_GROUP_QUEUE			0x40	/* Common queue related events and requests */
#define EVTM2U_QUEUE_START			0x41	/* Start of queue sending */
#define EVTM2U_QUEUE_ITEM			0x42	/* Queue item */
#define EVTM2U_QUEUE_FINISH			0x43	/* Queue finished */

#define EVTM2U_GROUP_NODE			0x50	/* Single node operations group */
#define EVTM2U_NODE_INFO			0x51	/* Nodelist info about node */
#define EVTM2U_NODE_QUEUE_STRAT		0x52	/* List of files for node starts */
#define EVTM2U_NODE_QUEUE_ITEM		0x53	/* One file */
#define EVTM2U_NODE_QUEUE_FINISH	0x54	/* List of files for node finihs */
                                      
#endif
