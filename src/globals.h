/* $Id: globals.h,v 1.4 2003/09/23 12:55:54 sisoft Exp $ */
#ifndef __GLOBALS_H__
#define __GLOBALS_H__

extern char *devnull;
extern char *osname;
extern int is_ip;
extern int bink;
extern int do_rescan;
extern pfile_t sendf;
extern pfile_t recvf;
extern long int temptot;
extern long int temptot2;
extern char ip_id[10];
extern ninfo_t *rnode;
extern ftnaddr_t DEFADDR;
extern unsigned long int totalf, totalm, totaln;
extern int was_req, got_req;
extern flist_t *fl;
extern char *connstr;

#endif
