/******************************************************************
 * IP tools defines.
 ******************************************************************/
/*
 * $Id: tcp.h,v 1.2 2005/05/06 20:47:37 mitry Exp $
 *
 * $Log: tcp.h,v $
 * Revision 1.2  2005/05/06 20:47:37  mitry
 * Misc code cleanup
 *
 * Revision 1.1  2005/03/28 16:47:59  mitry
 * New file
 *
 */

#ifndef __IP_TOOLS_H__
#define __IP_TOOLS_H__

#include <config.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

/* max hostname size */
#ifndef MAXHOSTNAMELEN
  #define MAXHOSTNAMELEN 255
#endif

char *get_hostname(struct sockaddr_in *, char *, int);
int tcp_setsockopts(int);

int tcp_dial(ftnaddr_t *, char *);
void tcp_done(void);

#endif
