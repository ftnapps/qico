/**********************************************************
 * tcp open
 * $Id: tcp.c,v 1.1.1.1 2003/07/12 21:27:16 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include "tty.h"

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
		write_log("can't resolve ip for\n",name);
		return 0;
	}
	sline("Connecting to %s:%d",
		  inet_ntoa(server.sin_addr),(int)ntohs(server.sin_port));
	signal(SIGPIPE,tty_sighup);
	if ((fd=socket(AF_INET,SOCK_STREAM,0))<0) {
		write_log("cannot create socket: %s",strerror(errno));
		return 0;
	}
	fflush(stdin); fflush(stdout);
	setbuf(stdin,NULL); setbuf(stdout,NULL);
	close(STDIN_FILENO); close(STDOUT_FILENO);
	if (dup2(fd,STDIN_FILENO) != STDIN_FILENO) {
		write_log("cannot dup socket: %s",strerror(errno));
		return 0;
	}
	if (dup2(fd,STDOUT_FILENO) != STDOUT_FILENO) {
		write_log("cannot dup socket: %s",strerror(errno));
		return 0;
	}
	clearerr(stdin);clearerr(stdout);
	if (connect(fd,(struct sockaddr *)&server,sizeof(server))<0) {
		write_log("cannot connect %s: %s",inet_ntoa(server.sin_addr),strerror(errno));
		return 0;
	}
	write_log("TCP/IP connection with %s",inet_ntoa(server.sin_addr));
	sline("Fido-server found... Waiting for reply...");
	return 1;
}

void closetcp(void)
{
#ifdef HAVE_SHUTDOWN	
  shutdown(STDIN_FILENO,2);
#endif  
  signal(SIGPIPE,SIG_DFL);
}

int tcp_call(char *host, ftnaddr_t *fa)
{
	int rc;

	write_log("connecting to %s at %s", ftnaddrtoa(fa), host);
	rc=opentcp(host);
	if(rc) {
		rc=session(1, SESSION_AUTO, fa, TCP_SPEED);
		closetcp();
		if((rc&S_MASK)==S_REDIAL) {
			write_log("creating poll for %s", ftnaddrtoa(fa));
			bso_poll(fa,F_ERR); 
		} 
	} else rc=S_REDIAL;
	title("Waiting...");
	vidle();
	sline("");
	return rc;
}
	
