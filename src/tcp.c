/**********************************************************
 * ip routines
 * $Id: tcp.c,v 1.7 2003/10/08 01:26:48 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "defs.h"

#define H_BUF 4096

static char b64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
int base64(char *data,int size,char *p)
{
	int i,c;
	char *s=p;
	unsigned char *q;
	q=(unsigned char*)data;
	for(i=0;i<size;) {
		c=q[i++]*256;
		if(i<size)c+=q[i];
		i++;c*=256;
		if(i<size)c+=q[i];
		p[0]=b64[(c&0x00fc0000)>>18];
		p[1]=b64[(c&0x0003f000)>>12];
		p[2]=b64[(c&0x00000fc0)>>6];
		p[3]=b64[(c&0x0000003f)];
		if(++i>size)p[3]='=';
		if(i>size+1)p[2]='=';
		p+=4;
	}
	return(p-s);
}

int http_connect(char *name,char *proxy)
{
	int rc,i;
	char buf[H_BUF]={0};
	char *n,*p,*a=strdup(proxy);
	if((p=strchr(a,' ')))*p++=0;
	DEBUG(('S',1,"connecting with proxy %s",a));
	i=sprintf(buf,"CONNECT %s HTTP/1.0\r\n",name);
	if(p) {
		if((n=strchr(p,' ')))*n=':';
		i+=sprintf(buf+i,"Proxy-Authorization: basic ");
		i+=base64(p,strlen(p),buf+i);
		buf[i++]='\r';buf[i++]='\n';
	}
	buf[i++]='\r';buf[i++]='\n';
	PUTBLK((unsigned char*)buf,i);
	for(i=0;i<H_BUF;i++) {
		rc=GETCHAR(cfgi(CFG_HSTIMEOUT));
		if(rc==TIMEOUT) {
			write_log("proxy timeout");
			return 1;
		} else if(rc<0) {
			write_log("connection closed by proxy");
			return 1;
		}
		buf[i]=rc;
		if(((i+1)>=H_BUF)||((p=strstr(buf,"\r\n\r\n")))) {
			if((p=strchr(buf,'\n'))) {
				*p=0;p--;
				if(*p=='\r')*p=0;
			}
			if(strstr(buf," 200 "))break;
			write_log("connect rejected by proxy (%s)",buf);
			return 1;
		}
	}
	return 0;
}

int opentcp(char *name)
{
	char *portname;
	struct servent *se;
	struct hostent *he;
	int a1,a2,a3,a4,fd;
	unsigned short portnum;
	struct sockaddr_in server;
	server.sin_family=AF_INET;
	if(cfgs(CFG_PROXY)) {
	
	
	}
	if((portname=strchr(name,':'))) {
			*portname++='\0';
			if ((portnum=atoi(portname)))
				server.sin_port=htons(portnum);
			else if ((se=getservbyname(portname,"tcp")))
				server.sin_port=se->s_port;
			else server.sin_port=htons(bink?24554:60179);
	} else {
		if ((se=getservbyname(bink?"binkp":"fido","tcp")))
			server.sin_port=se->s_port;
		else server.sin_port=htons(bink?24554:60179);
	}
	if (sscanf(name,"%d.%d.%d.%d",&a1,&a2,&a3,&a4) == 4)
		server.sin_addr.s_addr=inet_addr(name);
	else if ((he=gethostbyname(name)))
		memcpy(&server.sin_addr,he->h_addr,he->h_length);
	else {
		write_log("can't resolve ip for %s",name);
		return -1;
	}
	sline("Connecting to %s:%d",inet_ntoa(server.sin_addr),(int)ntohs(server.sin_port));
	signal(SIGPIPE,tty_sighup);
	if ((fd=socket(AF_INET,SOCK_STREAM,0))<0) {
		write_log("cannot create socket: %s",strerror(errno));
		return -1;
	}
	fflush(stdin); fflush(stdout);
	setbuf(stdin,NULL); setbuf(stdout,NULL);
	close(STDIN_FILENO); close(STDOUT_FILENO);
	if (dup2(fd,STDIN_FILENO) != STDIN_FILENO) {
		write_log("cannot dup socket: %s",strerror(errno));
		return -1;
	}
	if (dup2(fd,STDOUT_FILENO) != STDOUT_FILENO) {
		write_log("cannot dup socket: %s",strerror(errno));
		return -1;
	}
	clearerr(stdin);clearerr(stdout);
	if (connect(fd,(struct sockaddr *)&server,sizeof(server))<0) {
		write_log("cannot connect %s: %s",inet_ntoa(server.sin_addr),strerror(errno));
		close(fd);
		return -1;
	}
	write_log("TCP/IP connection with %s:%d",inet_ntoa(server.sin_addr),(int)ntohs(server.sin_port));
	sline("Fido-server found... Waiting for reply...");
	return fd;
}

void closetcp(int fd)
{
	close(fd);
#ifdef HAVE_SHUTDOWN	
	shutdown(STDIN_FILENO,2);
#endif  
	signal(SIGPIPE,SIG_DFL);
}

int tcp_call(char *host, ftnaddr_t *fa)
{
	int rc,fd;
	write_log("connecting to %s at %s [%s]", ftnaddrtoa(fa), host,bink?"binkp":"ifcico");
	fd=opentcp(host);
	if(fd>=0) {
		rc=session(1,bink?SESSION_BINKP:SESSION_AUTO, fa, TCP_SPEED);
		closetcp(fd);
		if((rc&S_MASK)==S_REDIAL) {
			write_log("creating poll for %s",ftnaddrtoa(fa));
			if(is_bso()==1)bso_poll(fa,F_ERR);
			    else if(is_aso()==1)aso_poll(fa,F_ERR);
		}
	} else rc=S_REDIAL;
	title("Waiting...");
	sline("");
	vidle();
	return rc;
}
