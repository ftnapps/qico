/**********************************************************
 * operations with CRC
 * $Id: crc.h,v 1.4 2004/02/09 01:05:33 sisoft Exp $
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

#define CRC32_CR(c,b)	(crc32_tab[((int)(c) ^ (b)) & 0xff] ^ ((c) >> 8))

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

extern int update_keys(unsigned long keys[3],int c);
extern void init_keys(unsigned long keys[3],char *passwd);
extern int decrypt_byte(unsigned long keys[3]);
extern void decrypt_buf(char *buf,unsigned bufsize,unsigned long keys[3]);
extern void encrypt_buf(char *buf,unsigned bufsize,unsigned long keys[3]);

extern int base64(char *data,int size,char *p);

#endif
