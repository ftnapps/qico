/**********************************************************
 * File: tcp.c
 * Created at Tue Aug 10 14:05:19 1999 by pk // aaz@ruxy.org.ru
 * tcp open
 * $Id: tcp.c,v 1.1.1.1 2000/07/18 12:37:21 lev Exp $
 **********************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include "tty.h"
#include "ftn.h"
#include "qipc.h"

int opentcp(char *name)
{
	struct servent *se;
	struct hostent *he;
	int a1,a2,a3,a4;
	char *portname;
	int fd;
	short portnum;
	struct sockaddr_in server;
	
	server.sin_family=AF_INET;
	if ((portname=strchr(name,':'))) {
			*portname++='\0';
			if ((portnum=atoi(portname)))
				server.sin_port=htons(portnum);
			else if ((se=getservbyname(portname,"tcp")))
				server.sin_port=se->s_port;
			else server.sin_port=htons(60179);
	} else {
		if ((se=getservbyname("fido","tcp")))
			server.sin_port=se->s_port;
		else server.sin_port=htons(60179);
	}
	if (sscanf(name,"%d.%d.%d.%d",&a1,&a2,&a3,&a4) == 4)
		server.sin_addr.s_addr=inet_addr(name);
	else if ((he=gethostbyname(name)))
		memcpy(&server.sin_addr,he->h_addr,he->h_length);
	else {
		log("can't resolve ip for\n",name);
		return 0;
	}
	sline("Connecting to %s:%d",
		  inet_ntoa(server.sin_addr),(int)ntohs(server.sin_port));
	signal(SIGPIPE,tty_sighup);
	fflush(stdin);
	fflush(stdout);
	setbuf(stdin,NULL);
	setbuf(stdout,NULL);
	close(0);
	close(1);
	if ((fd=socket(AF_INET,SOCK_STREAM,0)) != 0) {
		log("cannot create socket");
		open("/dev/null",O_RDONLY);
		open("/dev/null",O_WRONLY);
		return 0;
	}
	if (dup(0) != 1) {
		log("cannot dup socket");
		open("/dev/null",O_WRONLY);
		return 0;
	}
	clearerr(stdin);
	clearerr(stdout);
	if (connect(fd,(struct sockaddr *)&server,sizeof(server))<0) {
		log("cannot connect %s",inet_ntoa(server.sin_addr));
		return 0;
	}
	log("TCP/IP connection with %s",inet_ntoa(server.sin_addr));
	sline("Fido-server found... Waiting for reply...");
	return 1;
}

void closetcp(void)
{
  shutdown(0,2);
  signal(SIGPIPE,SIG_DFL);
}

int tcp_call(char *host, ftnaddr_t *fa)
{
	int rc;

	log("connecting to %s at %s", ftnaddrtoa(fa), host);
	rc=opentcp(host);
	if(rc) {
		rc=session(1, SESSION_AUTO, fa, TCP_SPEED);
		closetcp();
		if((rc&S_MASK)==S_REDIAL) {
			log("creating poll for %s", ftnaddrtoa(fa));
			bso_poll(fa); 
		} 
	} else rc=S_REDIAL;
	title("Waiting...");
	vidle();
	sline("");
	return rc;
}
	
