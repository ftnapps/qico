/*
 * $Id: modem.h,v 1.1 2005/05/16 20:33:46 mitry Exp $
 *
 * $Log: modem.h,v $
 * Revision 1.1  2005/05/16 20:33:46  mitry
 * Code cleaning
 *
 */

#ifndef __MODEM_H__
#define __MODEM_H__

#define MODEM_OK                0
#define MODEM_PORTLOCKED       -1
#define MODEM_PORTACCESSDENIED -2

#define MC_OK            0
#define MC_FAIL          1
#define MC_ERROR         2
#define MC_BUSY          3
#define MC_NODIAL        4
#define MC_RING          5
#define MC_BAD           6
#define MC_CANCEL        7

int	modem_hangup(void);
int	modem_stat_collect(void);
int	modem_dial(char *, char *);
void	modem_done(void);

#endif /* __MODEM_H__ */
