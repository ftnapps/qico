/**********************************************************
 * ip routines
 * $Id: tcp.c,v 1.17 2004/02/13 22:29:01 sisoft Exp $
 **********************************************************/
#include "headers.h"
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
#include "qipc.h"
#include "tty.h"
#include "crc.h"

#define H_BUF 4096

static int proxy_conn(char *name)
{
	time_t t1;
	int rc,i;
	char buf[H_BUF];
	char *n,*p,*dp="";
	if((p=strchr(cfgs(CFG_PROXY),' ')))*p++=0;
	if(!strchr(name,':'))dp=bink?":24554":"60179";
	i=snprintf(buf,H_BUF,"CONNECT %s%s HTTP/1.0\r\n",name,dp);
	if(p) {
		if((n=strchr(p,' ')))*n=':';
		i+=snprintf(buf+i,H_BUF-i,"Proxy-Authorization: basic ");
		i+=base64(p,strlen(p),buf+i);
		buf[i++]='\r';buf[i++]='\n';
	}
	buf[i++]='\r';buf[i++]='\n';
	PUTBLK((unsigned char*)buf,i);
	t1=t_set(cfgi(CFG_HSTIMEOUT));
	for(i=0;i<H_BUF;i++) {
		while((rc=GETCHAR(1))==TIMEOUT&&!t_exp(t1))getevt();
		if(rc==RCDO||tty_hangedup) {
			write_log("got hangup");
			return 1;
		}
		if(rc==TIMEOUT) {
			write_log("proxy timeout");
			return 1;
		} else if(rc<0) {
			write_log("connection closed by proxy");
			return 1;
		}
		buf[i]=rc;buf[i+1]=0;
		if((i>=H_BUF)||((p=strstr(buf,"\r\n\r\n")))) {
			if((p=strchr(buf,'\n'))) {
				*p=0;p--;
				if(*p=='\r')*p=0;
			}
			p=buf;if(strstr(buf," 200 "))break;
			if(!strncasecmp(buf,"HTTP",4))p=strchr(buf,' ')+1;
			write_log("connect rejected by proxy (%s)",p);
			return 1;
		}
	}
	return 0;
}

static int socks_conn(char *name)
{
	int rc,i,port;
	char *p,*n,*auth=NULL;
	char buf[MAX_STRING];
	struct in_addr da;
	struct hostent *he;
	if((p=strchr(cfgs(CFG_SOCKS),' ')))*p++=0;
	if(!(n=strchr(name,':')))port=bink?24554:60179;
	    else port=atoi(n+1);
	if(p) {
		auth=xstrdup(p);
		*buf=5;buf[2]=0;
		n=strchr(auth,' ');
		if(*auth=='*') {
			buf[1]=1;
			PUTBLK((unsigned char*)buf,3);
		} else {
			buf[1]=2;
			buf[3]=n?2:1;
			PUTBLK((unsigned char*)buf,4);
		}
		rc=GETCHAR(60);
		if(rc>0) {
			*buf=rc;
			rc=GETCHAR(10);
			buf[1]=rc;
		}
		if(rc<0||tty_hangedup) {
			write_log("connection lost %d,%d",rc,errno);
			return 1;
		}
		if(buf[1]&&buf[1]!=1&&buf[1]!=2) {
			write_log("Auth. method not supported by server");
			return 1;
		}
		DEBUG(('S',2,"socks5 auth=%d",buf[1]));
		if(buf[1]==2) {
			*buf=1;
			if(!n)i=strlen(auth);
			    else i=n-auth;
			buf[1]=i;
			memcpy(buf+2,auth,i);
			i+=2;
			if(!n)buf[i++]=0;
			    else {
				n++;
				buf[i]=strlen(n);
				xstrcpy(buf+i+1,n,buf[i]);
				i+=buf[i]+1;
			}
			PUTBLK((unsigned char*)buf,i);
			rc=GETCHAR(40);
			if(rc>0) {
				*buf=rc;
				rc=GETCHAR(10);
				buf[1]=rc;
			}
			if(rc<0||tty_hangedup) {
				write_log("connection lost %d,%d",rc,errno);
				return 1;
			}
			if(buf[1]) {
				write_log("Auth. failed (socks5 return %02x%02x)",(unsigned char)buf[0],(unsigned char)buf[1]);
				return 1;
			}
		}
	}
	if(!p) {
		*buf=4;buf[1]=1;
		buf[2]=(unsigned char)((port>>8)&0xff);
		buf[3]=(unsigned char)(port&0xff);
		if(!(he=gethostbyname(name))) {
			write_log("can't resolve addr: %s",strerror(errno));
			return 1;
		}
		memcpy(buf+4,he->h_addr,4);
		buf[8]=0;
		PUTBLK((unsigned char*)buf,9);
	} else {
		*buf=5;buf[1]=1;buf[2]=0;
		if(isdigit(*name)&&(da.s_addr=inet_addr(name))!=INADDR_NONE) {
			buf[3]=1;
			memcpy(buf+4,&da,4);
			n=buf+8;
		} else {
			buf[3]=3;
			i=strlen(name);
			buf[4]=(unsigned char)i;
			memcpy(buf+5,name,i);
			n=buf+5+i;
		}
		*n++=(unsigned char)((port>>8)&0xff);
		*n++=(unsigned char)(port&0xff);
		PUTBLK((unsigned char*)buf,n-buf);
	}
	for(i=0;i<MAX_STRING;i++) {
		getevt();
		rc=GETCHAR(45);
		if(rc<0||tty_hangedup) {
			write_log("connection lost %d,%d",rc,errno);
			return 1;
		}
		buf[i]=rc;
		if(!p&&i>6) {
			if(*buf) {
				write_log("bad reply from server");
				return 1;
			}
			if(buf[1]!=90) {
				write_log("cannection reject by socks4 server (%d)",(unsigned char)buf[1]);
				return 1;
			}
			xfree(auth);
			return 0;
		} else if(p&&i>5) {
			if(*buf!=5) {
				write_log("bad reply from server");
				return 1;
			}
			if(buf[3]==1&&i<9)continue;
			if(buf[3]==3&&i<(6+(unsigned char)buf[4]))continue;
			if(buf[3]==4&&i<21)continue;
			if(!buf[1]) {
				xfree(auth);
				return 0;
			}
			switch (buf[1]) {
			    case 1: write_log("general SOCKS5 server failure"); break;
			    case 2: write_log("connection not allowed by ruleset (socks5)"); break;
			    case 3: write_log("Network unreachable (socks5)"); break;
			    case 4: write_log("Host unreachable (socks5)"); break;
			    case 5: write_log("Connection refused (socks5)"); break;
			    case 6: write_log("TTL expired (socks5)"); break;
			    case 7: write_log("Command not supported by socks5"); break;
			    case 8: write_log("Address type not supported"); break;
			    default:write_log("Unknown reply (0x%02x) from socks5 server",(unsigned char)buf[1]);
			}
			return 1;
		}
	}
	write_log("too long answer");
	return 1;
}

int opentcp(char *name,char *prx,int sp)
{
	char *portname,*p;
	struct servent *se;
	struct hostent *he;
	int a1,a2,a3,a4,fd;
	unsigned short portnum;
	struct sockaddr_in server;
	server.sin_family=AF_INET;
	if((portname=strchr(prx?prx:name,':'))) {
		*portname++=0;
		if((p=strchr(portname,' ')))*p=0;
		if((portnum=atoi(portname)))server.sin_port=htons(portnum);
		else if((se=getservbyname(portname,"tcp")))server.sin_port=se->s_port;
		else server.sin_port=htons(prx?(sp?1080:3128):(bink?24554:60179));
	    } else {
		if((se=getservbyname(prx?(sp?"socks":"proxy"):(bink?"binkp":"fido"),"tcp")))server.sin_port=se->s_port;
		else if(prx&&!sp&&(se=getservbyname("squid","tcp")))server.sin_port=se->s_port;
		else server.sin_port=htons(prx?(sp?1080:3128):(bink?24554:60179));
	}
	if(sscanf(prx?prx:name,"%d.%d.%d.%d",&a1,&a2,&a3,&a4)==4)server.sin_addr.s_addr=inet_addr(prx?prx:name);
	else if((he=gethostbyname(prx?prx:name)))memcpy(&server.sin_addr,he->h_addr,he->h_length);
	else {
		write_log("can't resolve ip for %s%s",prx?(sp?"socks ":"proxy "):"",prx?prx:name);
		return -1;
	}
	sline("Connecting to %s%s%s%s:%d",prx?name:"",prx?" via ":"",prx?(sp?"socks ":"proxy "):"",inet_ntoa(server.sin_addr),(int)ntohs(server.sin_port));
	signal(SIGPIPE,tty_sighup);
	if ((fd=socket(AF_INET,SOCK_STREAM,0))<0) {
		write_log("can't create socket: %s",strerror(errno));
		return -1;
	}
	fflush(stdin);fflush(stdout);
	setbuf(stdin,NULL);setbuf(stdout,NULL);
	close(STDIN_FILENO);close(STDOUT_FILENO);
	if(dup2(fd,STDIN_FILENO)!=STDIN_FILENO) {
		write_log("can't dup socket: %s",strerror(errno));
		return -1;
	}
	if(dup2(fd,STDOUT_FILENO)!=STDOUT_FILENO) {
		write_log("can't dup socket: %s",strerror(errno));
		return -1;
	}
	clearerr(stdin);clearerr(stdout);
	if(connect(fd,(struct sockaddr*)&server,sizeof(server))<0) {
		write_log("can't connect %s: %s",inet_ntoa(server.sin_addr),strerror(errno));
		close(fd);
		return -1;
	}
	write_log("TCP/IP connection with %s%s:%d",prx?(sp?"socks ":"proxy "):"",inet_ntoa(server.sin_addr),(int)ntohs(server.sin_port));
	if(prx) {
		sline("%s-server found.. waiting for reply...",sp?"Socks":"Proxy");
		if(!sp) {
		    if(proxy_conn(name)) {
			close(fd);
			return -1;
		    }
		} else if(socks_conn(name)) {
			close(fd);
			return -1;
		}
	}
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

int tcp_call(char *host,ftnaddr_t *fa)
{
	int rc,fd,s=0;
	char *p=NULL,*t=NULL;
	if(cfgs(CFG_PROXY))p=xstrdup(ccs);
	    else if(cfgs(CFG_SOCKS)){p=xstrdup(ccs);s=1;}
	if(p&&(t=strchr(p,' ')))*t=0;
	write_log("connecting to %s at %s%s%s%s [%s]",ftnaddrtoa(fa),host,
	    p?" via ":"",p?(s?"socks ":"proxy "):"",p?p:"",bink?"binkp":"ifcico");
	if(t)*t=' ';
	fd=opentcp(host,p,s);xfree(p);
	if(fd>=0) {
		rc=session(1,bink?SESSION_BINKP:SESSION_AUTO,fa,TCP_SPEED);
		closetcp(fd);
		if((rc&S_MASK)==S_REDIAL&&cfgi(CFG_FAILPOLLS)) {
			write_log("creating poll for %s",ftnaddrtoa(fa));
			if(BSO)bso_poll(fa,F_ERR);
			    else if(ASO)aso_poll(fa,F_ERR);
		}
	} else rc=S_REDIAL;
	title("Waiting...");
	sline("");
	vidle();
	return rc;
}
