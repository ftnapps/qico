/*
 * $Id: qipc.h,v 1.3 2005/05/16 11:17:30 mitry Exp $
 *
 * $Log: qipc.h,v $
 * Revision 1.3  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.2  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 */

#ifndef __QIPC_H__
#define __QIPC_H__

void	qsendpkt(char, const char *, const char *, size_t);
size_t	qrecvpkt(char *);
void	vlogs(const char *);
void	vlog(const char *, ...);
void	sendrpkt(char, int, const char *, ...);
void	sline(const char *, ...);
void	title(const char *, ...);
void	qemsisend(const ninfo_t *);
void	qpreset(int);
void	qpqueue(const ftnaddr_t *, int, int, int, int);
void	qpproto(char, const pfile_t *);
void	qpmydata(void);

#define QLNAME		log_tty ? ( is_ip ? ip_id : log_tty ) : "master"
#define qpfsend()	qpproto( QC_SENDD, &sendf )
#define qpfrecv()	qpproto( QC_RECVD, &recvf )
#define qereset()	qsendpkt( QC_EMSID, QLNAME, "", 0 )
#define qqreset()	qsendpkt( QC_QUEUE, QLNAME, "", 0 )
#define vidle()		qsendpkt( QC_LIDLE, QLNAME, "", 0 )
#define qlerase()	qsendpkt( QC_ERASE, QLNAME, "", 0 )
#define qlcerase()	qsendpkt( QC_CERASE,QLNAME, "", 0 )
#define qchat(x)	qsendpkt( QC_CHAT, QLNAME, (x), strlen((x))+1 )

#endif
