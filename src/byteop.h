/**********************************************************
 * operations with bytes
 * $Id: byteop.h,v 1.4 2004/02/13 22:29:01 sisoft Exp $
 **********************************************************/
#ifndef __BYTEOP_H__
#define __BYTEOP_H__

#define FETCH32(p) (((byte*)(p))[0]+(((byte*)(p))[1]<<8)+ \
                   (((byte*)(p))[2]<<16)+(((byte*)(p))[3]<<24))
#define STORE32(p,i) ((byte*)(p))[0]=i;((byte*)(p))[1]=i>>8; \
                       ((byte*)(p))[2]=i>>16;((byte*)(p))[3]=i>>24;
#define INC32(p) p=(byte*)(p)+4

#define FETCH16(p) (((byte*)(p))[0]+(((byte*)(p))[1]<<8))
#define STORE16(p,i) ((byte*)(p))[0]=i;((byte*)(p))[1]=i>>8;
#define INC16(p) p=(byte*)(p)+2

#endif
