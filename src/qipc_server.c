/**********************************************************
 * File: qipc_server.c
 * Created at Wed Apr  4 23:53:12 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: qipc_server.c,v 1.2 2001/09/17 18:56:48 lev Exp $
 **********************************************************/
#include "headers.h"
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

/* Some global variables */
static anylist_t *ip_lines;
static anylist_t *tty_lines;
static anylist_t *clients;
static int line_socket;
static int client_socket;

/* 
 *   Support functions for anylist_t with uiclient_t
 */
/* Allocate client record for anylist */
al_item_t *uiclient_alloc(void *key)
{
	uiclient_t *p;
	if(!(p=xmalloc(sizeof(*p)))) return NULL;
	memset(p,0,sizeof(*p));
	p->socket = *((int*)key);
	return (al_item_t*)p;
}

/* Free client record for anylist */
void uiclient_free(al_item_t *i)
{
	uiclient_t *p = (uiclient_t*)i;
	if(!p) return;
	shutdown(p->socket,3); close(p->socket);
	xfree(p);
}

/* Compare client record with key for anylist */
int uiclient_cmp(al_item_t *i, void *key)
{
	uiclient_t *p = (uiclient_t*)i;
	return p->socket != *((int*)key);
}

/* 
 *   Support functions for anylist_t with line2ui_t
 */
/* Allocate mapping record for anylist */
al_item_t *line2ui_alloc(void *key)
{
	line2ui_t *p;
	if(!(p=xmalloc(sizeof(*p)))) return NULL;
	memset(p,0,sizeof(*p));
	p->client = (uiclient_t*)key;
	return (al_item_t*)p;
}

/* Free mapping record for anylist */
void line2ui_free(al_item_t *i)
{
	line2ui_t *p = (line2ui_t*)i;
	if(!p) return;
	xfree(p);
}

/* Compare mapping record with key for anylist */
int line2ui_cmp(al_item_t *i, void *key)
{
	line2ui_t *p = (line2ui_t*)i;
	return p->client != (uiclient_t*)key;
}

/* 
 *   Support functions for anylist_t with linestate_t -- PID as key (IP lines)
 */
/* Allocate line record for anylist */
al_item_t *ipline_alloc(void *key)
{
	linestate_t *p;
	if(!(p=xmalloc(sizeof(*p)))) return NULL;
	memset(p,0,sizeof(*p));
	p->pid = *((pid_t*)key);
	p->phase = SESS_PHASE_NOPROC;
	if(!(p->clients=al_new(lien2ui_alloc,line2ui_free,line2ui_cmp))) return NULL;
	return (al_item_t*)p;
}

/* Free line record for anylist */
/* Use anyline_free() */

/* Compare client record with key for anylist */
int ipline_cmp(al_item_t *i, void *key)
{
	linestate_t *p = (line2ui_t*)i;
	return p->pid != *((pid_t*)key);
}

/* 
 *   Support functions for anylist_t with linestate_t -- tty as key (TTY lines)
 */
/* Allocate line record for anylist */
al_item_t *ttyline_alloc(void *key)
{
	linestate_t *p;
	if(!(p=xmalloc(sizeof(*p)))) return NULL;
	memset(p,0,sizeof(*p));
	p->phase = SESS_PHASE_NOPROC;
	if(!(p->tty=xstrdup((char*)key))) return NULL;
	if(!(p->clients=al_new(lien2ui_alloc,line2ui_free,line2ui_cmp))) return NULL;
	return (al_item_t*)p;
}

/* Free line record for anylist */
/* Use anyline_free() */

/* Compare client record with key for anylist */
int ttyline_cmp(al_item_t *i, void *key)
{
	linestate_t *p = (line2ui_t*)i;
	return strcmp(p->tty,(char*)key);
}

/* Free any line in list */
void anyline_free(al_item_t *i)
{
	linestate_t *p = (line2ui_t*)i;
	int i;
	if(!p) return;
	nlfree(&p->remote);
	if(p->tty) xfree(p->tty);
	if(p->connect) xfree(p->connect);
	if(p->cid) xfree(p->cid);
	for(i=0;i<LOG_BUFFER_SIZE;i++) if(p->log_buffer[i]) xfree(p->log_buffer[i]);
	if(p->clients) al_free(p->clients);
	xfree(p);
}


/* Clear tty line -- delete all internal contents, but leave registartion and tty unchanged */
void ttyline_clear(linestate_t *p)
{
	int i;
	anylist_t *l;
	CHAR *c;
	if(!p) return;
	nlfree(&p->remote);
	if(p->connect) xfree(p->connect);
	if(p->cid) xfree(p->cid);
	for(i=0;i<LOG_BUFFER_SIZE;i++) if(p->log_buffer[i]) xfree(p->log_buffer[i]);
	l = p->clients;
	c = p->tty;
	memset(p,0,sizeof(*p));
	p->clients = l;
	p->tty = c;
	p->phase = SESS_PHASE_NOPROC;
}

static int set_all_opts(int s)
{
	int x = 1;
	if(!setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&x,sizeof(x))) return -1;
	if(!setsockopt(s,SOL_SOCKET,SO_REUSEPORT,&x,sizeof(x))) return -1;

	x = fcntl(s,F_GETFL,0L);
	if(x < 0) return -1;
	x |= O_NONBLOCK;
	if(fcntl(s,F_SETFL,x) < 0) return -1;
	return 0;
}


int server_init()
{
	struct sockaddr_in sin;
	slist_t ports = NULL;

	/* Allocate list for IP lines */
	if(!(ip_lines=al_new(ipline_alloc,anyline_free,ipline_cmp)))return -1;
	/* Allocate list fot TTY lines */
	if(!(tty_lines=al_new(ttyline_alloc,ttyline_free,ttyline_cmp)))return -1;
	/* Allocate list for clients */
	if(!(clients=al_new(uiclient_alloc,uiclient_free,uiclient_cmp)))return -1;

	/* Insert all TTY lines into list */
	ports = cfgsl(CFG_PORT);
	while(ports) {
		al_finda(tty_lines,ports->str,NULL);
		ports = ports->next;
	}

	/* Prepare and open sockets */
	/* Common part */
    sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(cfgi(CFG_MANAGERPORT));

	/* UDP socket for lines. On loopback only for security reasons. */
	sin.sin_addr.s_addr = INADDR_LOOPBACK;
	line_socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(line_socket == -1) return -1;
	if(!set_all_opts(line_socket)) return -1;
	if(bind(line_socket,(struct sockaddr*)&sin,sizeof(sin))) return -1;


	/* TCP socket for UIs. */
	sin.sin_addr.s_addr = INADDR_ANY;
	client_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(client_socket == -1) return -1;
	if(!set_all_opts(client_socket)) return -1;
	if(bind(client_socket,(struct sockaddr*)&sin,sizeof(sin))) return -1;
	if(listen(client_socket,4)) return -1;

	return 0;
}

void server_delete_client(uiclient_t *client)
{
	linestate_t *line;

	/* Delete client from TTY lines */
	line = (linestat_t*) tty_lines->head;
	while(line) {
		al_delk(line->clients,(void*)client);
		line = line->next;
	}

	/* Delete client from IP lines */
	line = (linestat_t*) ip_lines->head;
	while(line) {
		al_delk(line->clients,(void*)client);
		line = line->next;
	}

	/* Close socket */
	shutdown(client->socket,3);
	close(client->socket);

	/* Delete client from global of clients */
	al_deli(clients,(al_item_t*)client);
}

static txrxstate_t *get_txrx_state(linestate_t *line, char d)
{
	switch(toupper(d)) {
	case 'R':
		return &line->recv;
	case 'S':
		return &line->send;
	default:
		write_log("Invalid direction: '%c'",d);
		return NULL;
	}
}

int server_process_line_event(linestate_t *line, evtlam_t *evt, struct sockaddr_in *sa)
{
	char *pwd;
	char c;
	txrxstate_t *txrx;

    /* First of all -- check PASSWORD */
	pwd = cfgs(CFG_LINEPASSWORD);
	if(memcmp(pwd,evt->password,sizeof(evt->password))) {
		write_log("Invalid password from line %d from '%16s': '%8s'",evt->pid,evt->tty[0]?evt->tty:'tcp',evt->password);
		return -1;
	}

	/* Check for PIDs */
	if(line->pid && line->pid != evt->pid) {
		write_log("Invalid PID: %d, was %d  from '%16s'",evt->pid,line->pid,evt->tty[0]?evt->tty:'tcp');
		if(!evt->tty[0]) al_deli(ip_lines,(al_item_t*)line);
		else ttyline_clear(line);
		return -1;
	}

	evt.fulllength -= sizeof(*evt);

	/* Process event */
	switch(evt->type) {
	case EVTL2M_REGISTER:					/* Register new line, set mode -- answer or call */
											/* Signature: "c" -- [A]NSWER/[C]ALL. */
		line->pid = evt->pid;
		line->phase = SESS_PHASE_BEGIN;
		memcpy(&line->from,sa,sizeof(line->from));
		if(1!=unpack_ipc_packet(evt->data,evt.fulllength,"c",&line->mode)) {
			write_log("Too short event data: %d (event %d) from '%16s'",evt.fulllength,evt->type,evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		line->mode = toupper(line->mode);
		if(line->mode != 'A' && line->mode != 'C') {
			write_log("Invalid call mode: '%c' from '%16s'",line->mode,evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		break;
	case EVTL2M_CLOSE:						/* Unregister line and delete it */
											/* Signature: "" */
		if(!evt->tty[0]) al_deli(ip_lines,(al_item_t*)line);
		else ttyline_clear(line);
		break;
	case EVTL2M_STAT_CONNECT:				/* Modem connected, carrier detected. Speed and CID */
											/* Signature: "dss"  -- SPEED,CONNECT,CID*/
		if(line->phase != SESS_PHASE_BEGIN) {
			write_log("Invalid CONNECT event from '%16s'",evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		if(3!=unpack_ipc_packet(evt->data,evt.fulllength,"dss",&line->speed,&line->connect,&line->cid)) {
			write_log("Too short event data: %d (event %d) from '%16s'",evt.fulllength,evt->type,evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		line->phase = SESS_PHASE_CONNECT;
		break;
	case EVTL2M_STAT_DETECTED:				/* Mailer detected. Session type. */
											/* Signature: "d"  -- SESSION TYPE */
		if(line->phase != SESS_PHASE_CONNECT) {
			write_log("Invalid DETECTED event from '%16s'",evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		if(1!=unpack_ipc_packet(evt->data,evt.fulllength,"d",&line->type)) {
			write_log("Too short event data: %d (event %d) from '%16s'",evt.fulllength,evt->type,evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		break;
	case EVTL2M_STAT_DATAGOT:				/* We know about remote node. node_t info */
											/* Signature: "n" -- REMOTE NODE INFO */
		if(line->phase != SESS_PHASE_CONNECT && line->phase != SESS_PHASE_HSHAKEOUT) {
			write_log("Invalid DATAGOT event from '%16s'",evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		if(1!=unpack_ipc_packet(evt->data,evt.fulllength,"n",&line->remote)) {
			write_log("Too short event data: %d (event %d) from '%16s'",evt.fulllength,evt->type,evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		line->phase = SESS_PHASE_HSHAKEIN;
		break;
	case EVTL2M_STAT_DATASENT:				/* Remote know about us */
											/* Signature: "" */
		if(line->phase != SESS_PHASE_CONNECT && line->phase != SESS_PHASE_HSHAKEIN) {
			write_log("Invalid DATASENT event from '%16s'",evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		line->phase = SESS_PHASE_HSHAKEOUT;
		break;
	case EVTL2M_STAT_HANDSHAKED:			/* Handshake finished. Protocols and options */
											/* Signature: "dd" -- PROTOCOL,FINAL OPTIONS*/
		if(line->phase != SESS_PHASE_HSHAKEOUT && line->phase != SESS_PHASE_HSHAKEIN) {
			write_log("Invalid HANDSHAKED event from '%16s'",evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		if(2!=unpack_ipc_packet(evt->data,evt.fulllength,"dd",&line->proto,&line->flags)) {
			write_log("Too short event data: %d (event %d) from '%16s'",evt.fulllength,evt->type,evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		line->phase = SESS_PHASE_INPROCESS;
		break;
	case EVTL2M_BATCH_STARTED:				/* Batch started */
											/* Signature: "c" -- DIRECTION */
		if(line->phase != SESS_PHASE_INPROCESS) {
			write_log("Invalid BATCH_STARTED event from '%16s'",evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		if(1!=unpack_ipc_packet(evt->data,evt.fulllength,"c",&c)) {
			write_log("Too short event data: %d (event %d) from '%16s'",evt.fulllength,evt->type,evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		if(!(rxtx = get_txrx_state(line,c))) return -1;
		rxtx->phase = TXRX_PHASE_BEGIN;
		rxtx->transferstarted = time(NULL);
		break;
	case EVTL2M_BATCH_ENDED:				/* Batch finished */
											/* Signature: "c" -- DIRECTION */
		if(line->phase != SESS_PHASE_INPROCESS) {
			write_log("Invalid BATCH_ENDED event from '%16s'",evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		if(1!=unpack_ipc_packet(evt->data,evt.fulllength,"c",&c)) {
			write_log("Too short event data: %d (event %d) from '%16s'",evt.fulllength,evt->type,evt->tty[0]?evt->tty:'tcp');
			return -1;
		}
		if(!(rxtx = get_txrx_state(line,c))) return -1;
		rxtx->phase = TXRX_PHASE_EOT;
		break;
	case EVTL2M_BATCH_INFO:					/* Batch info */
											/* Signature: "cdd" -- DIRECTION,FILES,TOTAL SIZE */
		break;
	case EVTL2M_BATCH_STATE:	/* State changed */
							/* Signature: "cc" -- DIRECTION,NEW STATE  */
		break;
	case EVTL2M_FILE_INFO:	/* Info about new file sended/received */
							/* Signature: "csdd" -- DIRECTION,NAME,SIZE,TIME  */
		break;
	case EVTL2M_FILE_BLOCK:	/* Block sended/received */
							/* Signature: "cd" -- DIRECTION,SIZE  */
		break;
	case EVTL2M_FILE_REPOS:	/* Repos */
							/* Signature: "cd" -- DIRECTION,POS  */
		break;
	case EVTL2M_FILE_END:	/* End */
							/* Signature: "cc" -- DIRECTION,REASON  */
		break;
	case EVTL2M_CHAT_INIT:	/* Remote request for chat */
							/* Signature: "" */
		break;
	case EVTL2M_CHAT_LINE:	/* Chat string received */
							/* Signature: "s" -- LINE */
		break;
	case EVTL2M_CHAT_CLOSE:	/* Remote close chat */
							/* Signature: "" */
		break;
	case EVTL2M_SLINE:	/* Status line */
						/* Signature: "s" -- LINE */
		break;
	case EVTL2M_LOG:	/* Log string */
						/* Signature: "s" -- LINE */
		break;
	default:
		break;
	}

	return 0;
}

int server_process_ui_event(uiclient_t *client, evtuam_t evt)
{
	return 0;
}

int server_dispatch_events(int usec)
{
	struct sockaddr_in sa;
	CHAR *buf;
	fd_set rfds, efds;
	struct timeval tv;
	time_t st,ft;
	int rc;
	int ncl = 0;
    uiclient_t *uic, *uic_tmp;
    evtlam_t *levt;
    evtuam_t *cevt;
    linestate_t *line;

	
	ft = time(NULL);
	do {
		st = ft;
		FD_ZERO(&rfds);
		FD_ZERO(&efds);
		FD_SET(line_socket,&rfds);
		FD_SET(client_socket,&rfds);
		FD_SET(line_socket,&efds);
		FD_SET(client_socket,&efds);

		uic = (uiclient_t*)clients->head;
		ncl = 0;
		while(uic) {
			ncl++;
			FD_SET(uic->socket,&rfds);
			FD_SET(uic->socket,&efds);
			uic = uic->next;
		}

		tv.tv_sec  = usec / 1000;
		tv.tv_usec = usec % 1000;
		rc = select(2+ncl,&rfds,NULL,&efds,&tv);
		if(rc<0) return -1;
		if(!rc) return 0;

		/* Here is event from line */
		if(FD_ISSET(rfds,line_socket)) {
			levt = (evtlam_t*)receive_ipc_packet_udp(line_socket,&sa);
			if(levt) {
				if(!levt->tty[0]) {
					/* Find TCP line, andd new one if it is needed */
					line = al_finda(ip_lines,levt->pid,NULL);
				} else {
					/* Find TTY line, add new one if it is needed */
					line = al_finda(tty_lines,levt->tty,&rc);
					if(!rc)
						write_log("Strange: request from not-configured TTY line '%16s'",levt->tty);
				}
				server_process_line_event(line,levt,&sa);
				xfree(levt);
			}
		}
		/* Here is request for new client connection */
		if(FD_ISSET(rfds,line_socket)) {
			uic = xmalloc(sizeof(*uic));
			if(uic) {
				uic->socket = accept(client_socket,(struct sockaddr*)&sa,sizeof(sa));
				if(uic->socket != -1) al_toend(clients,(al_item_t*)uic);
				else xfree(uic);
			}
		}

		/* Check all clients */
		uic = (uiclient_t*)clients->head;
		while(uic) {
			if(FD_ISSET(efds,uic->socket)) { /* Error? Delete then this one */
				uic_tmp = uic;
				uic = uic->next;
				server_delete_client(uic_tmp);
			} else if(FD_ISSET(rfds,uic->socket)) { /* Process request */
				cevt = (evtuam_t*)receive_ipc_packet_tcp(uic->socket);
				if(cevt) {
					server_process_ui_event(uic,cevt);
					xfree(cevt);
				}
				uic = uic->next;
			} else {
				uic = uic->next;
			}
		}
		/* Change process time */
		ft = time(NULL);
		usec -= (ft - st) * 1000;
	} while(usec > 0);
	return 0;
}

