/**********************************************************
 * global variables
 * $Id: globals.c,v 1.3 2003/08/25 15:27:39 sisoft Exp $
 **********************************************************/
#include "headers.h"

char *devnull="/dev/null";
char *osname="Unix";
int is_ip=0,do_rescan=0,bink=0;
pfile_t sendf,recvf;
char ip_id[10];
ninfo_t *rnode=NULL;

ftnaddr_t DEFADDR={0,0,0,0};

unsigned long int totalf,totalm,totaln;
int was_req,got_req;
flist_t *fl=NULL;
