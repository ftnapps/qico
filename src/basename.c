#include <string.h>
#include <config.h>

#ifdef HAVE_BASENAME
#	define BASENAME_NAME	q_basename
#else
#	define BASENAME_NAME	basename
#endif

char *BASENAME_NAME(const char *filename)
{
  char *p = strrchr (filename, '/');
  return p ? p + 1 : (char *) filename;
}
