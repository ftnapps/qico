#include <sys/types.h>
#include <sys/unistd.h>

pid_t getsid(pid_t pid)
{
	return getpid();
}