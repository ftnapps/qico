/* $Id: globals.h,v 1.1.1.1 2003/07/12 21:26:39 sisoft Exp $ */
#ifndef __GLOBALS_H__
#define __GLOBALS_H__

extern char *devnull;
extern char *osname;
extern int is_ip;
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

#endif
