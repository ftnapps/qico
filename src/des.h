/**********************************************************
 * File: des.h
 * Created at Sat Jun 30 23:28:31 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: des.h,v 1.1 2001/07/05 19:55:12 lev Exp $
 **********************************************************/
#ifndef __DES_H__
#define __DES_H__

/* Key and encriptyiion/decryption context */
typedef struct _DESCONTEXT {
	UINT32 key[32];
} descontext_t;


void des_init_key(descontext_t *cx,CHAR *key);

void des_encrypt(descontext_t *cx,UINT32 *out,UINT32 *in);
void des_decrypt(descontext_t *cx,UINT32 *out,UINT32 *in);

void des_cbc_encrypt(descontext_t *cx,CHAR *iv,CHAR *dest,CHAR *src,UINT32 len);
void des_cbc_decrypt(descontext_t *cx,CHAR *iv,CHAR *dest,CHAR *src,UINT32 len);

#endif/*__DES_H__*/
