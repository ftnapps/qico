/******************************************************************
 * File: execsh.c
 * Created at Sun Nov 21 12:05:23 1999 by pk // aaz@ruxy.org.ru
 * Base version of this file was taken from by Eugene Crosser's ifcico 
 * $Id: execsh.c,v 1.8 2003/06/18 20:30:25 raorn Exp $
 ******************************************************************/
#include "headers.h"

int execsh(char *cmd)
{
	int pid,status,rc;

	if ((pid=fork()) == 0) {
		to_dev_null();
		rc=execl(SHELL,"sh","-c",cmd,NULL);
		exit(-1);
	}
	if(pid<0) {
		write_log("can't fork(): %s",strerror(errno));
		return -1;
	}
	do {
		rc=waitpid(pid, &status, 0);
	} while ((rc == -1) && (errno == EINTR));
	if(rc<0) {
		write_log("error in waitpid(): %s",strerror(errno));
		return -1;
	}
	return WEXITSTATUS(status);
}

int execnowait(char *cmd,char *p1,char *p2,char *p3)
{
	int pid,rc;

	if ((pid=fork()) == 0) {
		to_dev_null();
		setsid();
		rc=execl(cmd,cmd,p1,p2,p3,NULL);
		if(rc<0) write_log("can't exec %s: %s", cmd, strerror(errno));
		exit(-1);
	}
	if(pid<0) {
		write_log("can't fork(): %s",strerror(errno));
		return -1;
	}
	return 0;
}
