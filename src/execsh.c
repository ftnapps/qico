/******************************************************************
 * exec prog's
 * $Id: execsh.c,v 1.1.1.1 2003/07/12 21:26:33 sisoft Exp $
 ******************************************************************/
#include "headers.h"

int execsh(char *cmd)
{
	int pid,status,rc,sverr;

	if (!(pid=fork())) {
		to_dev_null();
		rc=execl(SHELL,"sh","-c",cmd,NULL);
		exit(-1);
	}
	do {
		rc=wait(&status);sverr=errno;
	} while (((rc > 0) && (rc != pid)) || ((rc == -1) && (sverr == EINTR)));
	if(rc<0) {
		write_log("wait() returned %d, status %d,%d",rc,status>>8,status&0xff);
		return -1;
	}
	return WEXITSTATUS(status);
}

int execnowait(char *cmd,char *p1,char *p2,char *p3)
{
	int pid,rc;

	if (!(pid=fork())) {
		to_dev_null();
		setsid();
		rc=execl(cmd,cmd,p1,p2,p3,NULL);
		if(rc<0) write_log("can't exec %s: %s", cmd, strerror(errno));
		exit(-1);
	}
	return 0;
}
