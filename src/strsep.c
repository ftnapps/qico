#include <string.h>

char *strsep(char **str, const char *delim)
{
    char *save = *str;
    if(*str == NULL)
	return NULL;
    *str = *str + strcspn(*str, delim);
    if(**str == 0)
	*str = NULL;
    else{
	**str = 0;
	(*str)++;
    }
    return save;
}
