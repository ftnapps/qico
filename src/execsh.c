/******************************************************************
 * File: execsh.c
 * Created at Sun Nov 21 12:05:23 1999 by pk // aaz@ruxy.org.ru
 * Base version of this file was taken from by Eugene Crosser's ifcico 
 * $Id: execsh.c,v 1.5 2001/02/18 11:39:33 aaz Exp $
 ******************************************************************/
#include "headers.h"

char devnull[]="/dev/null";

int execsh(char *cmd)
{
	int pid,status,rc,sverr;

	if (!(pid=fork())) {
		close(0);close(1);close(2);
		if(open(devnull,O_RDONLY) != 0) {
			write_log("reopening of stdin failed");exit(-1);
		}
		if(open(devnull,O_WRONLY | O_APPEND | O_CREAT,0600) != 1) {
			write_log("reopening of stdout failed");exit(-1);
		}
		if (open(devnull,O_WRONLY | O_APPEND | O_CREAT,0600) != 2) {
			write_log("reopening of stderr failed");exit(-1);
		}
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
		close(0);close(1);close(2);
		if(open(devnull,O_RDONLY) != 0) {
			write_log("reopening of stdin failed");exit(-1);
		}
		if(open(devnull,O_WRONLY | O_APPEND | O_CREAT,0600) != 1) {
			write_log("reopening of stdout failed");exit(-1);
		}
		if (open(devnull,O_WRONLY | O_APPEND | O_CREAT,0600) != 2) {
			write_log("reopening of stderr failed");exit(-1);
		}
		setsid();
		rc=execl(cmd,cmd,p1,p2,p3,NULL);
		if(rc<0) write_log("can't exec %s: %s", cmd, strerror(errno));
		exit(-1);
	}
	return 0;
}
