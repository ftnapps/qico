/**********************************************************
 * File: types.h
 * Created at Tue Feb 13 22:23:31 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: types.h,v 1.1 2001/02/15 20:31:45 lev Exp $
 **********************************************************/
#ifndef __TYPES_H__
#define __TYPES_H__
#include <config.h>

#if SIZEOF_CHAR==1
typedef signed char		SINT8;
typedef signed char		INT8;
typedef signed char		BYTE;
typedef unsigned char	UINT8;
typedef unsigned char	CHAR;
#else
#	error "There is no 8-bit integer type in your compiler, sorry"
#endif

#if SIZEOF_SHORT==2
typedef signed short	SINT16;
typedef signed short	INT16;
typedef unsigned short	UINT16;
typedef unsigned short	WORD;
#else
#	if SIZEOF_INT==2
typedef signed int		SINT16;
typedef signed int		INT16;
typedef unsigned int	UINT16;
typedef unsigned int	WORD;
#	else
#		error "There is no 16-bit integer type in your compiler, sorry"
#	endif
#endif

#if SIZEOF_LONG==4
typedef signed long		SINT32;
typedef signed long		INT32;
typedef unsigned long	UINT32;
typedef unsigned long	DWORD;
#else
#	if SIZEOF_INT==4
typedef signed int		SINT32;
typedef signed int		INT32;
typedef unsigned int	UINT32;
typedef unsigned int	DWORD;
#	else
#		error "There is no 32-bit integer type in your compiler, sorry"
#	endif
#endif

#endif
