#include <string.h>

char *basename (const char *filename)
{
  char *p = strrchr (filename, '/');
  return p ? p + 1 : (char *) filename;
}
