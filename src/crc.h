/**********************************************************
 * operations with CRC
 **********************************************************/
/*
 * $Id: crc.h,v 1.6 2005/05/06 20:26:57 mitry Exp $
 *
 * $Log: crc.h,v $
 * Revision 1.6  2005/05/06 20:26:57  mitry
 * Misc changes
 *
 * Revision 1.5  2005/03/29 20:37:08  mitry
 * Remove commented out code
 *
 */

#ifndef __CRC_H__
#define __CRC_H__

extern UINT32 crc32_tab[256];		/* CRC polynomial 0xedb88320 -- CCITT CRC32. */
extern UINT16 crc16usd_tab[256];	/* CRC polynomial 0x1021 -- CCITT upside-down CRC16. EMSI, ZModem, Janus. */
extern UINT16 crc16prp_tab[256];	/* CRC polynomial 0x8408 -- CCITT proper CRC16. Hydra. */

/* CRC-32 CCITT. Used by Hydra, ZModem, Janus */
#define CRC32_INIT		(0xffffffffl)
#define CRC32_TEST		(0xdebb20e3l)
#define CRC32_UPDATE(b,crc)	((crc32_tab[((crc) ^ (b)) & 0xff] ^ (((crc) >> 8) & 0x00ffffffl)) & 0xffffffffl)
#define CRC32_FINISH(crc)	((~(crc)) & 0xffffffffl)

#define CRC32_CR(c,b)	(crc32_tab[((int)(c) ^ (b)) & 0xff] ^ ((c) >> 8))

/* CRC-16 CCITT upside-down. Used by ZModem, Janus and EMSI */
#define CRC16USD_INIT		(0x0000)
#define CRC16USD_TEST		(0x0000)
#define CRC16USD_UPDATE(b,crc)	((crc16usd_tab[(((crc) >> 8)  ^ (b)) & 0xff] ^ (((crc) & 0x00ff) << 8)) & 0xffff)
#define CRC16USD_FINISH(crc)	(crc)

/* CRC-16 CCITT (X.25). Used by Hydra file transfer protocol */
#define CRC16PRP_INIT		(0xffff)
#define CRC16PRP_TEST		(0xf0b8)
#define CRC16PRP_UPDATE(b,crc)	((crc16prp_tab[((crc) ^ (b)) & 0xff] ^ ((((crc) >> 8)) & 0x00ff)) & 0xffff)
#define CRC16PRP_FINISH(crc)	((~(crc)) & 0xffff)


/* ------------------------------------------------------------------------- */
/* CRC-32								     */
/* ------------------------------------------------------------------------- */
UINT32 crc32s(void *);
UINT32 crc32block(void *, size_t);

/* ------------------------------------------------------------------------- */
/* CRC-16 upside-down, transmitted high-byte first			     */
/* ------------------------------------------------------------------------- */
UINT16 crc16usds(void *);
UINT16 crc16usd(void *, size_t);

/* ------------------------------------------------------------------------- */
/* CRC-16 proper, used for both CCITT and the one used by ARC		     */
/* ------------------------------------------------------------------------- */
UINT16 crc16prps(void *);
UINT16 crc16prp(void *, size_t);

int update_keys(unsigned long *, int);
void init_keys(unsigned long *, char *);
int decrypt_byte(unsigned long *);
void decrypt_buf(char *, size_t, unsigned long *);
void encrypt_buf(char *, size_t, unsigned long *);

int base64(void *, size_t, char *);

#endif
