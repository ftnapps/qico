/**********************************************************
 * File: qcc.c
 * Created at Sun Aug  8 16:23:15 1999 by pk // aaz@ruxy.org.ru
 * qico control center
 * $Id: qcc.c,v 1.19.4.1 2003/01/29 07:42:17 cyrilm Exp $
 **********************************************************/
#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include <signal.h>
#ifdef HAVE_NCURSES_H
# include <ncurses.h>
#else
# ifdef HAVE_CURSES_H
#  include <curses.h>
# endif
#endif
#ifndef GWINSZ_IN_SYS_IOCTL
# include <termios.h>
#endif
#include "qcconst.h"
#include "ver.h"
#include "byteop.h"

#define xfree(p) do { if(p) free(p); p = NULL; } while(0)

#define SAFE(s) s?s:nothing
#define MH 10
#define MAX_SLOTS 9

typedef struct {
	char tty[8];
	int  session;
	
	char *header;
	char *status;

	char *name;
	char *city;
	char *sysop;
	char *flags;
	char *phone;
	char *addrs;
	
	int  speed;
	int  options;
	time_t start;
	
	pfile_t s,r;
	WINDOW *wlog;
} slot_t;

typedef struct _qslot_t {
	int n;
	char *addr;
	int mail, files, flags, try;
	struct _qslot_t *next, *prev;
} qslot_t;

void sigwinch(int s);
char *nothing="";
char *helpl[]={
/* 	           "F1","Help ", */
/* 			   "F2","Send ", */
/* 			   "F3","Freq ", */
/* 			   "F4","Edit ", */
/* 			   "F5","Poll ", */
/* 			   "F8","Kill ", */
	           "0-9,Tab", " Switch line ",
			   "Up","/",
			   "Down"," Scroll queue ",
			   "Q","uit ",
			   NULL, NULL
};

slot_t *slots[MAX_SLOTS];
qslot_t *queue;
int currslot, allslots, q_pos, q_first, q_max, sizechanged=0;

char *m_header=NULL;
char *m_status=NULL;

WINDOW *wlog, *wmain, *whdr, *wstat;

int qipc_msg;

#ifndef CURS_HAVE_MVVLINE
void mvvline (int y,int x,int ch,int n)
{
	move (y,x);
	vline (ch,n);
}

void mvhline (int y,int x,int ch,int n)
{
	move (y,x);
	hline (ch,n);
}

void mvwhline (WINDOW *win,int y,int x,int ch,int n)
{
	wmove (win,y,x);
	wvline (win,ch,n);
}
#endif

void draw_screen() {
	int i,k=1;
	
	attrset(COLOR_PAIR(2));
	bkgd(COLOR_PAIR(2)|' ');
	mvvline(1, 0, ACS_VLINE, LINES-3);
	mvvline(1, COLS-1, ACS_VLINE, LINES-3);
	mvhline(MH+1, 1, ACS_HLINE, COLS-2);
	mvaddch(MH+1, 0, ACS_LTEE);mvaddch(MH+1, COLS-1, ACS_RTEE);
	mvaddch(0, 0, ACS_ULCORNER);mvaddch(0, 1, ACS_HLINE);
	mvaddch(0, COLS-1, ACS_URCORNER);mvaddch(0, COLS-2, ACS_HLINE);
	mvaddch(LINES-2, 1, '[');mvaddch(LINES-2, 0, ACS_LLCORNER);
	mvaddch(LINES-2, COLS-2, ']');mvaddch(LINES-2, COLS-1, ACS_LRCORNER);

	attron(COLOR_PAIR(8));mvhline(LINES-1,0,' ',COLS);
	for(i=0;helpl[i];i++) {
		if(i%2) attron(COLOR_PAIR(8));
		else attron(COLOR_PAIR(9));
		mvaddstr(LINES-1,k,helpl[i]);
		k+=strlen(helpl[i]);
	}
	attron(COLOR_PAIR(10));
	mvprintw(LINES-1,COLS-3-2-strlen(version), "qcc-%s", version);
	refresh();

	wmain=newwin(MH, COLS-2, 1, 1);
	wbkgd(wmain, COLOR_PAIR(6)|' ');
	
 	wlog=newwin(LINES-MH-4, COLS-2, MH+2, 1);
	wbkgd(wlog, COLOR_PAIR(7)|' ');
	scrollok(wlog, TRUE);	

	wstat=newwin(1, COLS-4, LINES-2, 2);
	wbkgd(wstat, COLOR_PAIR(6)|' ');

	whdr=newwin(1, COLS-4, 0, 2);
	wbkgd(whdr, COLOR_PAIR(13)|A_BOLD|' ');
	
	wrefresh(wmain);
	wrefresh(wstat);
	wrefresh(whdr);
 	wrefresh(wlog);
}	

void initscreen()
{
	initscr();start_color();
	cbreak();noecho();nonl();
	nodelay(stdscr, TRUE);keypad(stdscr, TRUE);leaveok(stdscr, FALSE);

	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_CYAN, COLOR_BLACK);
	init_pair(4, COLOR_RED, COLOR_BLACK);
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(6, COLOR_YELLOW, COLOR_BLACK);
	init_pair(7, COLOR_WHITE, COLOR_BLACK);
	
	init_pair(8, COLOR_BLACK, COLOR_WHITE);
	init_pair(9, COLOR_RED, COLOR_WHITE);
	init_pair(10, COLOR_BLUE, COLOR_WHITE);
	
	init_pair(12, COLOR_YELLOW, COLOR_BLUE);
	init_pair(13, COLOR_WHITE, COLOR_BLUE);
	init_pair(14, COLOR_CYAN, COLOR_BLUE);
	init_pair(15, COLOR_GREEN, COLOR_BLUE);
	
	init_pair(16, COLOR_BLACK, COLOR_CYAN);

	draw_screen();
	
 	signal(SIGWINCH, sigwinch);
}

void donescreen()
{
	int i;
	for(i=0;i<allslots;i++) delwin(slots[i]->wlog);
	delwin(wlog);
	delwin(whdr);
	delwin(wstat);
	delwin(wmain);
	clear();
	refresh();
	endwin();
}

void freshhdr()
{
	werase(whdr);
	wattron(whdr, COLOR_PAIR(14));
	mvwprintw(whdr,0,0,"[%d/%d] ", currslot+1, allslots);	
	wattron(whdr, COLOR_PAIR(13));
	waddstr(whdr, currslot>=0 ? slots[currslot]->tty : "master");
	waddch(whdr, ' ');
	wattron(whdr, COLOR_PAIR(12));
	waddstr(whdr, currslot>=0 ? slots[currslot]->header : m_header);
}

void freshstatus()
{
	werase(wstat);
	waddstr(wstat,currslot>=0 ? slots[currslot]->status : m_status);
}

char *timestr(time_t tim)
{
	static char ts[10];
	long int hr;
	if(tim<0) tim=0;
	hr=tim/3600;
	snprintf(ts, 10, "%02ld:%02ld:%02ld", hr, tim/60-hr*60, tim%60);
	return ts;
}

void bar(int o, int t, int l)
{
	int i, k, x;
	k=t/l;x=0;
	for(i=0;i<l;i++) {
		waddch(wmain, (k<o-x+1)?ACS_BLOCK:ACS_CKBOARD);
		x+=k;
	}
}

#define SIZES(x) (((x)<1024)?(x):(((x)<1048576)?((x)/1024):((x)/1048576)))
#define SIZEC(x) (((x)<1024)?'b':(((x)<1048576)?'k':'M'))

void freshpfile(int b, int e, pfile_t *s, int act)
{
	char bf[20];

	if(s->cps<1) return;
	wattrset(wmain, (act?COLOR_PAIR(4):COLOR_PAIR(2))|A_BOLD);
	mvwprintw(wmain, 4, b, "%s file %d of %d",
			  act?"Sending":"Receiving",s->nf, s->allf);
	wattrset(wmain, COLOR_PAIR(6)|A_BOLD);
	mvwaddnstr(wmain, 5, b, s->fname, e-b);
	wattrset(wmain, COLOR_PAIR(7)|A_BOLD);
	snprintf(bf, 20, " %d CPS", s->cps);
	mvwaddstr(wmain, 4, e-strlen(bf), bf);
	wattroff(wmain, A_BOLD);
	mvwprintw(wmain, 6, b, "Current: %d of %d bytes",
			  s->foff, s->ftot);	
	wattron(wmain, A_BOLD);
	wmove(wmain, 7, b);bar(s->foff, s->ftot, e-b-9);waddch(wmain, ' ');
	waddstr(wmain, timestr((s->ftot-s->foff)/s->cps));
	wattrset(wmain, COLOR_PAIR(1));
	mvwprintw(wmain, 8, b, "Total: %d%c of %d%c",
			  SIZES(s->toff+s->foff), SIZEC(s->toff+s->foff),
			  SIZES(s->ttot), SIZEC(s->ttot));	
	wattron(wmain, A_BOLD);
	wmove(wmain, 9, b);bar(s->toff+s->foff, s->ttot, e-b-9);waddch(wmain, ' ');
	waddstr(wmain, timestr((s->ttot-s->toff-s->foff)/s->cps));
}

void freshslot()
{
	werase(wmain);
	if(!slots[currslot]->session) return;
	wattrset(wmain, COLOR_PAIR(3)|A_BOLD);
	mvwprintw(wmain, 0, 1, "%s // %s",
			  slots[currslot]->name,SAFE(slots[currslot]->city));
	wattroff(wmain, A_BOLD);
	mvwprintw(wmain, 0, COLS-3-8-strlen(SAFE(slots[currslot]->sysop)), " Sysop: %s",
			  slots[currslot]->sysop);
	mvwprintw(wmain, 1, 1, "[%d] %s",
			  slots[currslot]->speed,SAFE(slots[currslot]->flags));
	mvwprintw(wmain, 1, COLS-3-8-strlen(SAFE(slots[currslot]->phone)), " Phone: %s",
			  slots[currslot]->phone);
	wattron(wmain, COLOR_PAIR(2)|A_BOLD);
	mvwaddstr(wmain, 2, 1, "AKA: ");
	wattroff(wmain, A_BOLD);
	waddnstr(wmain, SAFE(slots[currslot]->addrs), COLS-10-6);
	wattrset(wmain, COLOR_PAIR(4));mvwaddch(wmain, 2, COLS-3-11,' ');
	mvwaddstr(wmain, 2, COLS-3-10,slots[currslot]->options&O_PWD?"[PWD]":"");
	wattrset(wmain, COLOR_PAIR(6));
	mvwaddstr(wmain, 2, COLS-3-5,slots[currslot]->options&O_LST?"[LST]":"");

	wattrset(wmain, COLOR_PAIR(2));
	mvwhline(wmain, 3, 0, ACS_HLINE, COLS-2);wattron(wmain, A_BOLD);
	mvwprintw(wmain, 3, COLS/2-5, "[%s]",
			  timestr(time(NULL)-slots[currslot]->start)); 
	freshpfile(1, (COLS-2)/2-1, &slots[currslot]->r, 0);
	freshpfile((COLS-2)/2+1, COLS-3, &slots[currslot]->s, 1);
}

char *sscat(char *s, int size)
{
	if(size<1024) {
		sprintf(s+strlen(s), " %4d", size);
		return s;
	}
	if(size<1024*1024) {
		sprintf(s+strlen(s), "%4dK", size/1024);
		return s;
	}
	sprintf(s+strlen(s), "%4dM", size/(1024*1024));
	return s;
}

void mylog(char *str, ...)
{
	char s[MAX_STRING];
	va_list args;

	wattron(wlog, A_BOLD);
	va_start(args, str);
	vsprintf(s, str, args);
	va_end(args);
	waddnstr(wlog, s, COLS-3);
	waddch(wlog, '\n');
	wattroff(wlog, A_BOLD);
	if(currslot<0) wrefresh(wlog);
}

qslot_t *addqueue(qslot_t **l)
{
	qslot_t **t, *p=NULL;
	for(t=l;*t;t=&((*t)->next)) p=*t;
	*t=(qslot_t *)malloc(sizeof(qslot_t));
	bzero(*t,sizeof(qslot_t));
	(*t)->prev=p;
	(*t)->n=p?p->n+1:1;
	return *t;
}

void killqueue(qslot_t **l)
{
	qslot_t *t;
	while(*l) {
		t=(*l)->next;
		xfree((*l)->addr);
		xfree(*l);
		*l=t;
	}
}


char qflgs[Q_MAXBIT]=Q_CHARS;
int  qclrs[Q_MAXBIT]=Q_COLORS;
void freshqueue()
{
	int i,k,l;
	char str[MAX_STRING];
	qslot_t *q;
	
	werase(wmain);
	wattrset(wmain,COLOR_PAIR(6)|A_BOLD);
	mvwaddstr(wmain,0,0,"* Address");
	mvwaddstr(wmain,0,COLS-20-Q_MAXBIT,"Mail  Files  Try  Flags");

	for(q=queue;q && q->n<q_first;q=q->next);
	for(i=0;q && i<MH-1;i++,q=q->next) {
		wattrset(wmain,COLOR_PAIR(3)|(q->flags&Q_DIAL?A_BOLD:0));
		if(q_pos==q->n) wattrset(wmain,COLOR_PAIR(16));
		mvwaddstr(wmain, i+1, 0, "  ");
		waddstr(wmain, q->addr);
		for(k=0;k<COLS-23-Q_MAXBIT-strlen(q->addr);k++)	waddch(wmain, ' ');
		str[0]=0;
		sscat(str, q->mail);strcat(str, "  ");
		sscat(str, q->files);
		sprintf(str+strlen(str), "  %3d  ", q->try);
		waddstr(wmain, str);
		l=1;
		for(k=0;k<Q_MAXBIT;k++) {
			if(q_pos!=q->n) wattron(wmain, COLOR_PAIR(qclrs[k]));
			waddch(wmain, (q->flags&l) ? qflgs[k] : '.');
			l=l<<1;
		}
	}
}
void freshall()
{
	freshhdr();wnoutrefresh(whdr);
	freshstatus();wnoutrefresh(wstat);
	if(currslot>=0)
		freshslot();
	else
		freshqueue();
	wnoutrefresh(wmain);
	touchwin((currslot<0)?wlog:slots[currslot]->wlog);
	wnoutrefresh((currslot<0)?wlog:slots[currslot]->wlog);
	doupdate();
}

void sigwinch(int s)
{
	sizechanged=1;
 	signal(SIGWINCH, sigwinch);
}

int findslot(char *slt)
{
	int i;
	for(i=0;i<allslots;i++)
		if(!strcmp(slots[i]->tty, slt)) return i;
	return -1;
}

int createslot(char *slt)
{
	slots[allslots]=malloc(sizeof(slot_t));
	bzero(slots[allslots], sizeof(slot_t));
	strcpy(slots[allslots]->tty, slt);
	slots[allslots]->wlog=newwin(LINES-MH-4, COLS-2, MH+2, 1);
	scrollok(slots[allslots]->wlog, TRUE);	
	return allslots++;
}

void logit(char *str, WINDOW *w)
{
	waddnstr(w, str, COLS-3);
	waddch(w, '\n');
}


char *strefresh(char **what, char *to)
{
	if(*what!=NULL) free(*what);
	*what=malloc(strlen(to)+1);
	if(*what) strcpy(*what, to);
	return *what;
}

#define xfree(p) do { if(p) free(p); p = NULL; } while(0)

void usage(char *ex)
{
	printf("qcc-%s copyright (c) pavel kurnosoff, 1999-2000\n"
		   "usage: %s [options]\n"
		   "-h           this help screen\n"
		   "\n", version, ex);
}

int main(int argc, char **argv)
{
	int quitflag=0, len, ch, rc, type, pid;
	fd_set rfds;
	struct timeval tv;
	char buf[MSG_BUFFER], *data, c, *p;
	time_t tim;
	struct tm *tt;
	key_t qipc_key;
	int lastfirst=1, lastpos=1;
#ifdef CURS_HAVE_WRESIZE		
	struct winsize size;
#endif

 	while((c=getopt(argc, argv, "h"))!=EOF) {
		switch(c) {
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 0;
		}
	}
		
	if((qipc_key=ftok(QIPC_KEY,QC_MSGQ))<0) {
		fprintf(stderr, "can't create ipc key\n");
		return 0;
	}
	if((qipc_msg=msgget(qipc_key, 0666 | IPC_CREAT | IPC_EXCL))<0) {
		fprintf(stderr, "can't create message queue (may be there's another qcc is running?)\n");
		return 0;
	}

	initscreen();
	currslot=-1;
	allslots=0;
	freshhdr();wrefresh(whdr);
	bzero(&slots, sizeof(slots));
	freshall();
	while(!quitflag) {
#ifdef CURS_HAVE_WRESIZE		
		if (sizechanged) {
			if (ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0) {
				int i;
				resizeterm(size.ws_row, size.ws_col);
				draw_screen();
				for(i=0;i<allslots;i++)	wresize(slots[i]->wlog,LINES-MH-4, COLS-2);
				freshall();
			}
			sizechanged=0;
 		} 
#endif
		tim=time(NULL);
		tt=localtime(&tim);wattron(whdr, COLOR_PAIR(15));
		mvwprintw(whdr, 0, COLS-13, "%02d:%02d:%02d",
				  tt->tm_hour, tt->tm_min, tt->tm_sec);
		wnoutrefresh(whdr);wnoutrefresh(wstat);
		doupdate();
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		tv.tv_sec=0;
		tv.tv_usec=5000;
		rc=select(1, &rfds, NULL, NULL, &tv);
		bzero(buf, MSG_BUFFER);
		rc=msgrcv(qipc_msg, buf, MSG_BUFFER-1, 1, IPC_NOWAIT);
		if(rc>=13) {
			len=FETCH32(buf+4);
			pid=FETCH32(buf+8);
			type=buf[12];
			data=strchr(buf+13,0)+1;
			data[len]=0;
			if(strcmp(buf+13, "master")) {
				rc=findslot(buf+13);
				if(type==QC_ERASE) {
					if(allslots>0) {
						if(currslot==allslots-1)
							currslot--;
						allslots--;
						delwin(slots[rc]->wlog);
						free(slots[rc]);
						slots[rc]=slots[rc+1];
						freshall();								
					}
				} else if(rc<0) {
					rc=createslot(buf+13);
					freshhdr();wrefresh(whdr);
				}
				if(type==QC_SLINE) {
					strefresh(&slots[rc]->status, data);
					freshstatus();wrefresh(wstat);
				}
				if(type==QC_LOGIT) {
					logit(data, slots[rc]->wlog);
					if(currslot==rc) {
						wrefresh(slots[rc]->wlog);
					}
				}
				if(type==QC_TITLE) {
					strefresh(&slots[rc]->header, data);
					if(currslot==rc) {
						freshhdr();wrefresh(whdr);
					}
				}
				if(type==QC_EMSID) {
					if(!len) {
						xfree(slots[rc]->name);
						xfree(slots[rc]->sysop);
						xfree(slots[rc]->city);
						xfree(slots[rc]->flags);
						xfree(slots[rc]->phone);
						xfree(slots[rc]->addrs);
						slots[rc]->session=0;
					} else {
						p=data;
						slots[rc]->speed=FETCH16(p);INC16(p);
						slots[rc]->options=FETCH32(p);INC32(p);
						slots[rc]->start=FETCH32(p);INC32(p);
						strefresh(&slots[rc]->name, p);p+=strlen(p)+1;
						strefresh(&slots[rc]->sysop, p);p+=strlen(p)+1;
						strefresh(&slots[rc]->city, p);p+=strlen(p)+1;
						strefresh(&slots[rc]->flags, p);p+=strlen(p)+1;
						strefresh(&slots[rc]->phone, p);p+=strlen(p)+1;
						strefresh(&slots[rc]->addrs, p);
						slots[rc]->session=1;
					}
					if(currslot==rc) {								
						freshslot();wrefresh(wmain);
					}
				}
				if(type==QC_LIDLE) {
					if(&slots[rc]->session) {
						slots[rc]->session=0;
						bzero(&slots[rc]->r, sizeof(pfile_t));
						bzero(&slots[rc]->s, sizeof(pfile_t));
						if(currslot==rc) {
							freshslot();wrefresh(wmain);
						}
					}
				}
				if(type==QC_SENDD) {
					if(!len) {
						bzero(&slots[rc]->s, sizeof(pfile_t));
					} else {
						char *p=data;
						
						slots[rc]->session=1;
						xfree(slots[rc]->s.fname);

						slots[rc]->s.foff=FETCH32(p);INC32(p);
						slots[rc]->s.ftot=FETCH32(p);INC32(p);
						slots[rc]->s.toff=FETCH32(p);INC32(p);
						slots[rc]->s.ttot=FETCH32(p);INC32(p);
						slots[rc]->s.nf=FETCH16(p);INC16(p);
						slots[rc]->s.allf=FETCH16(p);INC16(p);
						slots[rc]->s.cps=FETCH32(p);INC32(p);
						slots[rc]->s.soff=FETCH32(p);INC32(p);
						slots[rc]->s.stot=FETCH32(p);INC32(p);
						slots[rc]->s.start=FETCH32(p);INC32(p);
						slots[rc]->s.mtime=FETCH32(p);INC32(p);
						
						slots[rc]->s.fname=strdup(p);
					}
					if(currslot==rc) {								
						freshslot();wrefresh(wmain);
					}
				}
				if(type==QC_RECVD) {
					if(!len) {
						bzero(&slots[rc]->r, sizeof(pfile_t));
					} else {
						char *p=data;
						
						slots[rc]->session=1;
						xfree(slots[rc]->r.fname);

						slots[rc]->r.foff=FETCH32(p);INC32(p);
						slots[rc]->r.ftot=FETCH32(p);INC32(p);
						slots[rc]->r.toff=FETCH32(p);INC32(p);
						slots[rc]->r.ttot=FETCH32(p);INC32(p);
						slots[rc]->r.nf=FETCH16(p);INC16(p);
						slots[rc]->r.allf=FETCH16(p);INC16(p);
						slots[rc]->r.cps=FETCH32(p);INC32(p);
						slots[rc]->r.soff=FETCH32(p);INC32(p);
						slots[rc]->r.stot=FETCH32(p);INC32(p);
						slots[rc]->r.start=FETCH32(p);INC32(p);
						slots[rc]->r.mtime=FETCH32(p);INC32(p);
						
						slots[rc]->r.fname=strdup(p);
					}
					if(currslot==rc) {								
						freshslot();wrefresh(wmain);
					}
				}
						
			} else {
				if(type==QC_SLINE) {
					strefresh(&m_status, data);
					if(currslot<0) {
						freshstatus();wrefresh(wstat);
					}
				}
				if(type==QC_TITLE) {
					strefresh(&m_header, data);
					if(currslot<0) {
						freshhdr();wrefresh(whdr);
					}
				}
				if(type==QC_LOGIT) {
					logit(data, wlog);
					if(currslot<0) {
						wrefresh(wlog);
					}
				}
				if(type==QC_QUEUE) {
					if(!len) {
						lastfirst=q_first;
						lastpos=q_pos;
						killqueue(&queue);
						q_max=0;
						q_first=1;
						q_pos=1;
					} else {
						char *p=data;
						qslot_t *q=addqueue(&queue);
						q_max=q->n;
						
						q->mail=FETCH32(p);INC32(p);
						q->files=FETCH32(p);INC32(p);
						q->flags=FETCH32(p);INC32(p);
						q->try=FETCH16(p);INC16(p);
						q->addr=strdup(p);

						if(lastfirst>=q->n) q_first=q->n;
						if(lastpos>=q->n) q_pos=q->n;
					}
					if(currslot<0) {								
						freshqueue();wrefresh(wmain);
					}
				}
			}
		}
		ch=getch();
		if(ch>='0' && ch<='9') {
			ch=ch-'0'-1;
			if(ch<allslots) {
				currslot=ch;
				freshall();
			}
		} else {
			switch(ch) {
			case '\t':
				if(allslots) {
					currslot++;
					if(currslot==allslots) currslot=-1;
					freshall();
				}
				break;
			case 'q':
			case KEY_F(10):
				quitflag=1;break;
			}
			if(currslot<0) switch(ch) {
			case KEY_UP:
				if(q_pos>1) {
					q_pos--;
					if(q_pos<q_first) q_first--;
				}				
				freshqueue();wrefresh(wmain);
				break;
			case KEY_DOWN:
				if(q_pos<q_max) {
					q_pos++;
					if(q_pos>=q_first+MH-1) q_first++;
				}
				freshqueue();wrefresh(wmain);
				break;
			}
		}
	}
	donescreen();
	
	msgctl(qipc_msg, IPC_RMID, 0);
	return 0;
}
