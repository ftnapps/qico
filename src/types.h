/**********************************************************
 * common types
 * $Id: types.h,v 1.1.1.1 2003/07/12 21:27:18 sisoft Exp $
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

#ifdef WORDS_BIGENDIAN
/* We are on non-Intel-like processor */
	/* Host to Intel */
#	define H2I16(x)	((((x) & 0x00FF) << 8) | (((x) >> 8) & 0x00FF))
#	define H2I32(x)	((H2I16((x) & 0x0000FFFF) << 16) | (H2I16(((x) >> 16) & 0x0000FFFF)))
	/* Intel to host */
#	define I2H16(x)	((((x) & 0x00FF) << 8) | (((x) >> 8) & 0x00FF))
#	define I2H32(x)	((I2H16((x) & 0x0000FFFF) << 16) | (I2H16(((x) >> 16) & 0x0000FFFF)))
#else 
/* We are on Intel-like processor */
	/* Host to Intel */
#	define H2I16(x)	(x)
#	define H2I32(x)	(x)
	/* Intel to host */
#	define I2H16(x)	(x)
#	define I2H32(x)	(x)
#endif

#endif
