/**********************************************************
 * global variables
 * $Id: globals.c,v 1.5 2004/01/12 21:41:56 sisoft Exp $
 **********************************************************/
#include "headers.h"

char *devnull="/dev/null";
char *osname="Unix";
int is_ip=0,do_rescan=0,bink=0;
pfile_t sendf,recvf;
char ip_id[10];
ninfo_t *rnode=NULL;
int ssock=-1,uis_sock=-1,lins_sock=-1;
ftnaddr_t DEFADDR={0,0,0,0,NULL};
unsigned long int totalf,totalm,totaln;
int was_req,got_req;
flist_t *fl=NULL;
char *connstr=NULL;
