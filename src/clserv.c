/**********************************************************
 * client/server tools
 * $Id: clserv.c,v 1.2 2004/01/15 23:39:41 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "clserv.h"

int (*xsend_cb)(int sock,char *buf,int len)=NULL;

int cls_conn(int type,char *port)
{
	int rc,f=1;
	struct sockaddr_in sa;
	struct servent *se;
	sa.sin_family=AF_INET;
	if(port&&atoi(port))sa.sin_port=htons(atoi(port));
	    else if(port&&(se=getservbyname(port,(type&CLS_UDP)?"udp":"tcp")))sa.sin_port=se->s_port;
		else sa.sin_port=htons(60178);
	rc=socket(AF_INET,type&CLS_UDP?SOCK_DGRAM:SOCK_STREAM,0);
    	if(rc<0)return rc;
	    else {
		xsend_cb=xsend;
		if(type&CLS_SERVER) {
			if(setsockopt(rc,SOL_SOCKET,SO_REUSEADDR,&f,sizeof(f))<0)return -1;
			sa.sin_addr.s_addr=htonl((type&CLS_UDP)?INADDR_LOOPBACK:INADDR_ANY);
			f=fcntl(rc,F_GETFL,0);
			if(f>=0)fcntl(rc,F_SETFL,f|O_NONBLOCK);
			if(bind(rc,(struct sockaddr*)&sa,sizeof(sa))<0)return -1;
			if(!(type&CLS_UDP))if(listen(rc,4)<0)return -1;
			return rc;
		}
		inet_pton(AF_INET,"127.0.0.1",&sa.sin_addr);
		if(connect(rc,(struct sockaddr*)&sa,sizeof(sa))<0)return -1;
		    else return rc;
	}
	return -1;
}

void cls_close(int sock)
{
	if(sock>=0)close(sock);
}

void cls_shutd(int sock)
{
#ifdef HAVE_SHUTDOWN	
	if(sock>=0)shutdown(sock,3);
#endif
	cls_close(sock);
	xsend_cb=xsend;
}

int xsend(int sock,char *buf,int len)
{
	int rc;
	unsigned short l=len;
//{FILE *f=fopen("/debg","a");fprintf(f,"%d\t:send %d: %d\n",getpid(),sock,sizeof(short));fclose(f);}
	if(sock<0){errno=EBADF;return -1;}
	rc=send(sock,&l,sizeof(short),MSG_DONTWAIT);
//{FILE *f=fopen("/debg","a");fprintf(f,"%d\t: rc=%d\n",getpid(),rc);fclose(f);}
	if(rc<=0)return rc;
//{FILE *f=fopen("/debg","a");fprintf(f,"%d\t:send %d: %d\n",getpid(),sock,len);fclose(f);}
	rc=send(sock,buf,len,0);
//{FILE *f=fopen("/debg","a");fprintf(f,"%d\t: rc=%d\n",getpid(),rc);fclose(f);}
	return rc;
}

int xrecv(int sock,char *buf,int len,int wait)
{
	int rc;
	unsigned short l=0;
	if(sock<0){errno=EBADF;return -1;}
//{FILE *f=fopen("/debg","a");fprintf(f,"%d\t:peek %d: %d\n",getpid(),sock,sizeof(short));fclose(f);}
	rc=recv(sock,&l,2,MSG_PEEK|(wait?MSG_WAITALL:MSG_DONTWAIT));
//{FILE *f=fopen("/debg","a");fprintf(f,"%d\t: rc=%d\n",getpid(),rc);fclose(f);}
	if(rc<=0)return rc;
	if(rc>=2&&l>0) {
//{FILE *f=fopen("/debg","a");fprintf(f,"%d\t:recv %d: %d\n",getpid(),sock,sizeof(short));fclose(f);}
		rc=recv(sock,&l,sizeof(short),MSG_WAITALL);
//{FILE *f=fopen("/debg","a");fprintf(f,"%d\t: rc=%d\n",getpid(),rc);fclose(f);}
		if(rc<sizeof(short))return -1;
		if(l>len)l=len;
//{FILE *f=fopen("/debg","a");fprintf(f,"%d\t:recv %d: %d\n",getpid(),sock,l);fclose(f);}
		rc=recv(sock,buf,l,MSG_WAITALL);
//{FILE *f=fopen("/debg","a");fprintf(f,"%d\t: rc=%d\n",getpid(),rc);fclose(f);}
		return rc;
	}
	return 0;
}
