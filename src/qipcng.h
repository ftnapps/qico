/**********************************************************
 * File: qipcng.h
 * Created at Wed Apr  4 00:05:05 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: qipcng.h,v 1.3 2001/06/08 15:43:20 lev Exp $
 **********************************************************/
#ifndef __QIPCNG_H__
#define __QIPCNG_H__

#define LOG_BUFFER_SIZE 10
#define MAX_CLIENTS		10
#define LINE_TIMEOUT	240

/* State of file transfer */
typedef struct _TRANSFERSTATE {
	CHAR phase;				/* phase of process */
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
	CHAR *file;				/* Name of current file */
} txrxstate_t;

/* Information about one line */
typedef struct _LINESTATE {
	CHAR phase;				/* Phase of session */
#define SESS_PHASE_NOPROC		'n'
#define SESS_PHASE_BEGIN		'b'
#define SESS_PHASE_CONNECT		'c'
#define SESS_PHASE_HSHAKEOUT	'o'
#define SESS_PHASE_HSHAKEIN		'i'
#define SESS_PHASE_INPROCESS	'p'
	CHAR mode;
	UINT32 flags;			/* Different flags, see opts.h */
	UINT32 type;			/* Type of session */
	UINT32 proto;			/* Used protocol */
	txrxstate_t send;
	txrxstate_t recv;
	ninfo_t remote;
	CHAR *tty;
	CHAR *connect;
	CHAR *cid;
	CHAR *log_buffer[LOG_BUFFER_SIZE];
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
	CHAR fixed;					/* 0 if it is IP line (dynamic), 1 if fixed (tty) one */
	UINT8 type;					/* See EVTL2M_XXXX / EVTM2L_XXXX */
	CHAR *data;					/* Data for event */
} evtlam_t;

/* Manager-to-UI and UI-to-manager event structure */
typedef struct _EVTLAU {
	UINT32 fulllength;			/* Full length, include type and fulllength fields */
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
/* Signature: "ss"  -- CONNECT,CID*/
#define EVTL2M_STAT_DETECTED	0x22	/* Mailer detected. Session type. */
/* Signature: "d"  -- SESSION TYPE */
#define EVTL2M_STAT_DATAGOT		0x23	/* We know about remote node. node_t info */
/* Signature: "n" -- REMOTE NODE INFO */
#define EVTL2M_STAT_DATASENT	0x24	/* Remote know about us */
/* Signature: "" */
#define EVTL2M_STAT_HANDSHAKED	0x25	/* Handshake finished. Protocols and options */
/* Signature: "dd" -- PROTOCOL,FINAL OPTIONS*/

#define EVTL2M_GROUP_BATCH		0x30	/* Batch related events */
#define EVTL2M_BATCH_STARTED	0x31	/* Batch started */
/* Signature: "c" -- DIRECTION */
#define EVTL2M_BATCH_ENDED		0x32	/* Batch finished */
/* Signature: "c" -- DIRECTION */
#define EVTL2M_BATCH_INFO		0x34	/* Batch info */
/* Signature: "cdd" -- DIRECTION,FILES,TOTAL SIZE */

#define EVTL2M_GROUP_RECVSEND	0x40	/* File send/receive related events */
#define EVTL2M_FILE_STATE		0x41	/* State changed */
/* Signature: "cc" -- DIRECTION,NEW STATE  */
#define EVTL2M_FILE_INFO		0x42	/* Info about new file sended/received */
/* Signature: "csdd" -- DIRECTION,NAME,SIZE,TIME  */
#define EVTL2M_FILE_BLOCK		0x43	/* Block sended/received */
/* Signature: "cd" -- DIRECTION,SIZE  */
#define EVTL2M_FILE_REPOS		0x44	/* Repos */
/* Signature: "cd" -- DIRECTION,POS  */
#define EVTL2M_FILE_END			0x45	/* End */
/* Signature: "cc" -- DIRECTION,REASON  */


#define EVTL2M_GROUP_CHAT		0x50	/* Chat related events */
#define EVTL2M_CHAT_INIT		0x51	/* Remote request for chat */
/* Signature: "" */
#define EVTL2M_CHAT_LINE		0x52	/* Chat string received */
/* Signature: "s" -- LINE */
#define EVTL2M_CHAT_CLOSE		0x53	/* Remote close chat */
/* Signature: "" */

#define EVTL2M_GROUP_LOG		0x60	/* Logging related events */
#define EVTL2M_SLINE			0x61	/* Status line */
/* Signature: "s" -- LINE */
#define EVTL2M_LOG				0x62	/* Log string */
/* Signature: "s" -- LINE */

/* Events from line to manager */
#define EVTM2L_HANGUP			0		/* Hang up now */
/* Signature: "" */
#define EVTM2L_SKIP				1		/* Skip current file */
/* Signature: "" */
#define EVTM2L_SUSPEND			2		/* Suspend current file */
/* Signature: "" */
#define EVTM2L_CHAT_INIT		3		/* Start chat with remote */
/* Signature: "" */
#define EVTM2L_CHAT_LINE		4		/* Send line to remote chat window */
/* Signature: "s" -- LINE */
#define EVTM2L_CHAT_CLOSE		5		/* Close chat with remote */
/* Signature: "" */

/* Events and requests from UI to manager */
#define EVTU2M_GROUP_GLOBAL			0x10	/* Global events */
#define EVTU2M_LOGIN				0x11	/* New UI want to connect */
/* Signature: "s" -- PASSWORD */
#define EVTU2M_LOGOUT				0x12	/* UI is closed */
/* Signature: "" */
#define EVTU2M_QUIT					0x13	/* Quit manager */
/* Signature: "" */

#define EVTU2M_GROUP_CONFIG			0x20	/* Configuration and line presense requests */
#define EVTU2M_CONFIG_STATION		0x21	/* Request for station (EMSI) info */
/* Signature: "" */
#define EVTU2M_CONFIG_TTYS			0x22	/* Request for list of configured ttys (lines) */
/* Signature: "" */
#define EVTU2M_CONFIG_TIME			0x23	/* Request for current time and timezone */
/* Signature: "" */
#define EVTU2M_CONFIG_MAILONLY		0x24	/* Request for mail-only time intervals */
/* Signature: "" */
#define EVTU2M_CONFIG_LINES			0x2f	/* Lines, which are active right now (with TCP/IP ones) */
/* Signature: "" */

#define EVTU2M_GROUP_LINE			0x30	/* Line related events and requests */
#define EVTU2M_LINE_FULL_STATUS		0x31	/* Request for full line status */
/* Signature: "d" -- PID OF LINE */
#define EVTU2M_LINE_CONNECT_STATUS	0x32	/* Request for connect status */
/* Signature: "d" -- PID OF LINE */
#define EVTU2M_LINE_REMOTE_STATUS	0x33	/* Request for remote node info */
/* Signature: "d" -- PID OF LINE */
#define EVTU2M_LINE_RXTX_STATUS		0x34	/* Request for sending/receiving status */
/* Signature: "dc" -- PID OF LINE,DIRECTION */
#define EVTU2M_LINE_CHAT_STATUS		0x35	/* Request for chat status -- enabled/disabled on/off */
/* Signature: "d" -- PID OF LINE */
#define EVTU2M_LINE_HANGUP			0x3d	/* Hang up line */
/* Signature: "d" -- PID OF LINE */
#define EVTU2M_LINE_SKIP			0x3e	/* Skip current file */
/* Signature: "d" -- PID OF LINE */
#define EVTU2M_LINE_SUSPEND			0x3f	/* Suspend current file */
/* Signature: "d" -- PID OF LINE */

#define EVTU2M_GROUP_QUEUE			0x40	/* Common queue related events and requests */
#define EVTU2M_QUEUE_GET			0x41	/* Request for outbound list */
/* Signature: ""  */
#define EVTU2M_QUEUE_RESCAN			0x42	/* Force rescan of queue */
/* Signature: ""  */

#define EVTU2M_GROUP_NODE			0x50	/* Single node operations group */
#define EVTU2M_NODE_POLL			0x51	/* Poll node */
/* Signature: "ac" -- ADDRESS,STATUS  */
#define EVTU2M_NODE_CHANGE_STATUS	0x52	/* Change status of node */
/* Signature: "ac" -- ADDRESS,STATUS  */
#define EVTU2M_NODE_ATTACH			0x53	/* Attach files(s) to node */
/* Signature: "acs" -- ADDRESS,STATUS.PATH  */
#define EVTU2M_NODE_KILL			0x54	/* Kill outbound for node */
/* Signature: "a" -- ADDRESS  */
#define EVTU2M_NODE_CALL			0x55	/* Call node on given (or any free) line */
/* Signature: "acs" -- ADDRESS,CHECK TIME,TTY  */
#define EVTU2M_NODE_INFO			0x56	/* Get nodelist info about node */
/* Signature: "a" -- ADDRESS  */
#define EVTU2M_NODE_QUEUE			0x57	/* Get list of files for node */
/* Signature: "a" -- ADDRESS  */
                                      
#define EVTU2M_GROUP_CHAT			0x60	/* Chat-related eventts */
#define EVTU2M_CHAT_INIT			0x61	/* Start chat with remote */
/* Signature: "d" -- PID OF LINE  */
#define EVTU2M_CHAT_LINE			0x62	/* Send chat line to remote */
/* Signature: "ds" -- PID OF LINE,LINE  */
#define EVTU2M_CHAT_CLOSE			0x63	/* Finish chat with remote */
/* Signature: "d" -- PID OF LINE  */
#define EVTU2M_CHAT_BUFFER			0x64	/* Request for current chat buffer */
/* Signature: "d" -- PID OF LINE  */

#define EVTU2M_GROUP_LOG			0x70	/* Log-related events */
#define EVTU2M_LOG_BUFFER			0x71	/* Request for log buffer */
/* Signature: "d" -- PID OF LINE (may be 0 for manager)  */
#define EVTU2M_LOG_SLINE			0x72	/* Request for current status line */
/* Signature: "d" -- PID OF LINE (may be 0 for manager)  */

#define EVTU2M_GROUP_REGISTER		0xf0	/* Group for registration requests */
#define EVTU2M_REG_REGMASK			0xf1	/* Register for events -- binary string for register  */
/* Signature: "dd" -- PID OF LINE,BIT MASK  */
#define EVTU2M_REG_UNREGMASK		0xf2	/* Unregister for events -- binary string for unregister */
/* Signature: "dd" -- PID OF LINE,BIT MASK  */

/* Events and information from manager to UI */
#define EVTM2U_GROUP_GLOBAL			0x10	/* Answers on global requests */
#define EVTM2U_HELLO				0x11	/* Login result */
/* Signature: "cs" -- RESULT,HELLO STRING  */
#define EVTM2U_GOODBYE				0x12	/* Answer on LOGOUT */
/* Signature: ""  */
#define EVTM2U_OK					0x13	/* Command completed Ok (may be with warning) */
/* Signature: "s" -- WARNING (may be empty)  */
#define EVTM2U_ERROR				0x14	/* Command WAS NOT completed (with error message) */
/* Signature: "s" -- ERROR  */
#define EVTM2U_LINE_EVENT			0x15	/* Retranslation of line event */
/* Signature: "dd..." -- PID,LINE EVENT,line event data */

#define EVTM2U_GROUP_CONFIG			0x20	/* Configuration and line presence info */
#define EVTM2U_CONFIG_STATION		0x21	/* Station (EMSI) info */
/* Signature: "ssssdsss" -- STATION,PLACE,SYSOP,PHONE,SPEED,FLAGS,WORK TIME,FREQ TIME */
#define EVTM2U_CONFIG_TTYS			0x22	/* List of configured ttys (lines) */
/* Signature: "s..." -- TTY1,... */
#define EVTM2U_CONFIG_TIME			0x23	/* Current time and timezone */
/* Signature: "dd" -- UNIX TIME,SECONDS TO GMT */
#define EVTM2U_CONFIG_MAILONLY		0x24	/* Mail-only time intervals */
/* Signature: "s" -- MAIL-ONLY TIMES */
#define EVTM2U_CONFIG_LINES			0x2f	/* Lines, which are active right now (with TCP/IP ones) */
/* Signature: "sd..." -- TTY/IP,PID,... */

#define EVTM2U_GROUP_LINE			0x30	/* Line related events and answers */
#define EVTM2U_LINE_CONNECT_STATUS	0x31	/* Connect status */
/* Signature: "ss" -- CONNECT,CID (may be empty, empty CONNECT, if disconnected) */
#define EVTM2U_LINE_REMOTE_STATUS	0x32	/* Remote node info */
#define EVTM2U_LINE_RXTX_STATUS		0x34	/* Sending/Receiving status */
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
