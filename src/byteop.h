/**********************************************************
 * operations with bytes
 * $Id: byteop.h,v 1.3 2004/02/05 19:51:16 sisoft Exp $
 **********************************************************/
#ifndef __BYTEOP_H__
#define __BYTEOP_H__

#define FETCH32(p) (((unsigned char *)(p))[0]+(((unsigned char *)(p))[1]<<8)+ \
                   (((unsigned char *)(p))[2]<<16)+(((unsigned char *)(p))[3]<<24))
#define STORE32(p,i) ((unsigned char *)(p))[0]=i;((unsigned char *)(p))[1]=i>>8; \
                       ((unsigned char *)(p))[2]=i>>16;((unsigned char *)(p))[3]=i>>24;
#define INC32(p) p=(unsigned char *)(p)+4

#define FETCH16(p) (((unsigned char *)(p))[0]+(((unsigned char *)(p))[1]<<8))
#define STORE16(p,i) ((unsigned char *)(p))[0]=i;((unsigned char *)(p))[1]=i>>8;
#define INC16(p) p=(unsigned char *)(p)+2


#endif
