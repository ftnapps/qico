/**********************************************************
 * File: crc.h
 * Created at Sun Mar 25 15:32:03 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: crc.h,v 1.1 2001/03/25 15:38:21 lev Exp $
 **********************************************************/
#ifndef __CRC_H__
#define __CRC_H__

extern UINT32 crc32_tab[256];			/* CRC polynomial 0xedb88320 -- CCITT CRC32. */
extern UINT16 crc16usd_tab[256];		/* CRC polynomial 0x1021 -- CCITT upside-down CRC16. EMSI, ZModem, Janus. */
extern UINT16 crc16prp_tab[256];		/* CRC polynomial 0x8408 -- CCITT proper CRC16. Hydra. */

/* CRC-32 CCITT. Used by Hydra, ZModem, Janus */
#define CRC32_INIT					(0xffffffffl)
#define CRC32_TEST					(0xdebb20e3l)
#define CRC32_UPDATE(b,crc)			((crc32_tab[((crc) ^ (b)) & 0xff] ^ (((crc) >> 8) & 0x00ffffffl)) & 0xffffffffl)
#define CRC32_FINISH(crc)			((~(crc)) & 0xffffffffl)

/* CRC-16 CCITT upside-down. Used by ZModem, Janus and EMSI */
#define CRC16USD_INIT				(0x0000)
#define CTC16USD_TEST				(0x0000)
#define CRC16USD_UPDATE(b,crc)		((crc16usd_tab[(((crc) >> 8)  ^ (b)) & 0xff] ^ (((crc) & 0x00ff) << 8)) & 0xffff)
#define CRC16USD_FINISH(crc)		(crc)

/* CRC-16 CCITT (X.25). Used by Hydra file transfer protocol */
#define CRC16PRP_INIT				(0xffff)
#define CTC16PRP_TEST				(0xf0b8)
#define CRC16PRP_UPDATE(b,crc)		((crc16prp_tab[((crc) ^ (b)) & 0xff] ^ ((((crc) >> 8)) & 0x00ff)) & 0xffff)
#define CRC16PRP_FINISH(crc)		((~(crc)) & 0xffff)


extern UINT32 crc32s(char *str);
extern UINT32 crc32(char *data, int size);

extern UINT16 crc16usds(char *str);
extern UINT16 crc16usd(char *data, int size);

extern UINT16 crc16prps(char *str);
extern UINT16 crc16prp(char *data, int size);

#endif
