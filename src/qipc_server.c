/**********************************************************
 * File: qipc_server.c
 * Created at Wed Apr  4 23:53:12 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: qipc_server.c,v 1.1 2001/04/24 19:21:00 lev Exp $
 **********************************************************/
#include "headers.h"
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

static linestat_t *tty_lines = NULL;
static linestat_t *ip_lines = NULL;
static int clients_sock[MAX_CLIENTS];
static int clients = 0;
static int tcp_sock = 0;
static int udp_sock = 0;
static time_t gcollected;
static event_handler HANDLERS[] = {
};

/* Create new line */
linestat_t *new_line(pid_t pid)
{
	int i;
	linestat_t *l;
	l = xcalloc(1,sizeof(*l));
	l->rnode=
	l->pid = pid;
	l->phase = SESS_PHASE_BEGIN;
	l->recv.phase = l->send.phase = TXRX_PHASE_BEGIN;
	return l;
}

/* Free line */
void free_line(linestat_t *l)
{
	nlinfo_t nl = l->remote;
	xfree(l->send.file);
	xfree(l->recv.file);
	xfree(l->tty);
	xfree(l->connect);
	xfree(l->cid);
	falist_kill(&nl.addrs);
	xfree(nl.name);
	xfree(nl.place);
	xfree(nl.sysop);
	xfree(nl.phone);
	xfree(nl.wtime);
	xfree(nl.flags);
	xfree(nl.pwd);
	xfree(nl.mailer);
	xfree(l);
}

/* Create server TCP socket and UDP socket */
int init_server()
{
	struct sockaddr_in la;

	la.sin_len = sizeof(localaddr);
	la.sin_family = AF_INET;

	tcp_sock = socket(AF_INET,SOCK_STREAM,PROTO_TCP);
	if(!tcp_sock) return -1;
	la.sin_port = htons(cfgi(CFG_CLIENTPORT));
	la.sin_addr.s_addr = INADDR_ANY;
	if(!bind(tcp_sock,(struct sockaddr *)&la,sizeof(la))) return -1;

	udp_sock = socket(AF_INET,SOCK_DGRAM,PROTO_UDP);
	if(!udp_sock) return -1;
	la.sin_port = htons(cfgi(CFG_LINEPORT));
	la.sin_addr.s_addr = INADDR_LOOPBACK;
	if(!bind(udp_sock,(struct sockaddr *)&la,sizeof(la))) return -1;
	gcollected = time(NULL);
}

/* Close sockets, free memory */
int done_server() 
{
	linestat_t *l, *p;
	int i;
	if(tcp_sock) { shutdown(tcp_sock,2); close(tcp_sock); }
	if(udp_sock) { shutdown(udp_sock,2); close(udp_sock); }
	for(i = 0; i < clients; i++) { shutdown(clients_sock[i],2); close(clients_sock[i]); }
	if (lines) {
		l = lines;
		while (l->next) l = l->next;
		while (l) { *p = l; l = l->prev; free_line(p); }
		lines = NULL;
	}
}

/* Find or create new line state record */
linestat_t *find_line(pid_t pid)
{
	linestat_t *l, *p;

	if (lines) {
		l = lines;
		while(l->next && l->pid != pid) l = l->next;
		if (l->pid == pid) return l;
		p = new_line(pid);
		l->next = p;
		p->prev = l;
		return p;
	} else {
		return lines = new_line(pid);
	}
}

/* Delete line from list */
void delete_line(pid_t pid)
{
	linestat_t *l, *p;
	if (!lines) return;
	l = lines;
	while(l && l->pid != pid) l = l->next;
	if (l) {
		if (l->prev) {
			if((p = l->prev)) {
				p->next = l->next;
				if (p->next) p->next->prev = p;
			}
		} else {
			if((lines = l->next)) {
				lines->prev = NULL;
				if (lines->next) lines->next->prev = lines;
			}
		}	
		free_line(l);
	}
}

/* check all lines for timeout in event, and delete all out-timed lines */
void garbage_collector() {
	linestat *l, *p;
	if (!lines) return;
	gcollected = time(NULL);
	l = lines;
	while(l) {	
		if(gcollected - l->lastevent < LINE_TIMEOUT) {
			write_log("Line [%d] %s timeout",l->pid,l->tty?l->tty:"???");
			if (l->prev) {
				if((p = l->prev)) {
					p->next = l->next;
					if (p->next) p->next->prev = p;
				}
				free_line(l);
				l = p;
			} else {
				if((lines = l->next)) {
					lines->prev = NULL;
					if (lines->next) lines->next->prev = lines;
				}
				free_line(l);
				l = lines;
			}
		} else {
			l = l->next;
		}
	}
}

/* Wait for any event for `timeout' seconds and dispatch this event */
void server_dispatch(time_t timeout)
{
	fd_set read;
	fd_set error;
	int i;

	FD_ZERO(&read);
	FD_ZERO(&error);

}