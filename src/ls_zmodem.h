/**********************************************************
 * File: ls_zmodem.h
 * Created at Sun Oct 29 18:51:46 2000 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: ls_zmodem.h,v 1.1 2000/11/02 07:00:20 lev Exp $
 **********************************************************/
#ifndef _LS_ZMODEM_H_
#define _LS_ZMODEM_H_
/*

   ZModem file transfer protocol. Written from scratches.
   Support CRC16, CRC32, RLE Encoding, variable header, ZedZap (big blocks).
   Global variables, common functions.

*/

#define LSZINIT_CRC16 0x0000
#define LSZTEST_CRC16 0x0000

#define LSZINIT_CRC32 0xFFFFFFFF
#define LSZTEST_CRC32 0xDEBB20E3

#define LS_UPDATE_CRC16(b,crc) (crc16tab[(((crc) >> 8) & 255)] ^ ((crc) << 8) ^ (b))
#define LS_UPDATE_CRC32(b,crc) (crc32tab[((crc) ^ (b)) & 0xff] ^ ((b) >> 8))

#define LS_UPDATE_CRC(b,crc) (ls_CRC32?(LS_UPDATE_CRC32(b,crc)):(LS_UPDATE_CRC16(b,crc)))

#endif/*_LS_ZMODEM_H_*/
