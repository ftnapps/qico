/**********************************************************
 * File: qcc.c
 * Created at Sun Aug  8 16:23:15 1999 by pk // aaz@ruxy.org.ru
 * qico control center
 * $Id: qcc.c,v 1.2 2000/07/18 12:56:18 lev Exp $
 **********************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include <signal.h>
#ifdef FREE_BSD
 #include <ncurses/curses.h>
#else
 #include <curses.h>
#endif
#include "qcconst.h"
#include "ver.h"

#define MAX_QUEUE 256

extern unsigned short crc16(char *str, int l);
void sigwinch(int s);
/*
#ifdef FREE_BSD
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
*/

#define MH 10
#define MAX_SLOTS 9

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

typedef struct {
	char tty[8];
	int  session;
	char header[MAX_STRING];
	char status[MAX_STRING];
	pemsi_t e;
	pfile_t s,r;
	WINDOW *wlog;
} slot_t;

slot_t *slots[MAX_SLOTS];
pque_t queue[MAX_QUEUE];
int currslot, allslots, q_size;
time_t online;

char m_header[MAX_STRING]="";
char m_status[MAX_STRING]="";

WINDOW *wlog, *wmain, *whdr, *wstat;

int sock, saddrlen, caddrlen, qpos=0;
struct sockaddr_un server, client;

void initscreen()
{
	int i,k=1;
	
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
	mvprintw(LINES-1,COLS-strlen(progname)-2-strlen(version), "%s-%s",
			 progname, version);
	refresh();

	wmain=newwin(MH, COLS-2, 1, 1);
	wbkgd(wmain, COLOR_PAIR(6)|' ');
	
	wlog=newwin(LINES-MH-4, COLS-2, MH+2, 1);
/*	wbkgd(wlog, COLOR_PAIR(7)|' ');*/
	scrollok(wlog, TRUE);	

	wstat=newwin(1, COLS-4, LINES-2, 2);
	wbkgd(wstat, COLOR_PAIR(6)|' ');

	whdr=newwin(1, COLS-4, 0, 2);
	wbkgd(whdr, COLOR_PAIR(13)|A_BOLD|' ');
	
	wrefresh(wmain);
	wrefresh(wstat);
	wrefresh(whdr);
	wrefresh(wlog);

/* 	signal(SIGWINCH, sigwinch); */
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
	printf("%s-%s: have a nice unix! c u l8r!\n", progname, version);
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
	sprintf(ts, "%02ld:%02ld:%02ld", hr, tim/60-hr*60, tim%60);
	return ts;
}

void bar(int o, int t, int l)
{
	int i, k, x;
	k=t/l;x=0;
	for(i=0;i<l;i++) {
		waddch(wmain, (x<o)?ACS_BLOCK:ACS_CKBOARD);
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
			  act?"Sending":"Receiveing",s->nf, s->allf);
	wattrset(wmain, COLOR_PAIR(6)|A_BOLD);
	mvwaddnstr(wmain, 5, b, s->fname, e-b);
	wattrset(wmain, COLOR_PAIR(7)|A_BOLD);
	sprintf(bf, " %d CPS", s->cps);
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

void freshproto()
{
	werase(wmain);
	if(!slots[currslot]->session) return;
	wattrset(wmain, COLOR_PAIR(3)|A_BOLD);
	mvwprintw(wmain, 0, 1, "%s // %s",
			  slots[currslot]->e.name,slots[currslot]->e.city);
	wattroff(wmain, A_BOLD);
	mvwprintw(wmain, 0, COLS-3-8-strlen(slots[currslot]->e.sysop), " Sysop: %s",
			  slots[currslot]->e.sysop);
	mvwprintw(wmain, 1, 1, "[%d] %s",
			  slots[currslot]->e.speed,slots[currslot]->e.flags);
	mvwprintw(wmain, 1, COLS-3-8-strlen(slots[currslot]->e.phone), " Phone: %s",
			  slots[currslot]->e.phone);
	wattron(wmain, COLOR_PAIR(2)|A_BOLD);
	mvwaddstr(wmain, 2, 1, "AKA: ");
	wattroff(wmain, A_BOLD);
	waddnstr(wmain, slots[currslot]->e.addrs, COLS-10-6);
	wattrset(wmain, COLOR_PAIR(4));mvwaddch(wmain, 2, COLS-3-11,' ');
	mvwaddstr(wmain, 2, COLS-3-10,slots[currslot]->e.secure?"[PWD]":"");
	wattrset(wmain, COLOR_PAIR(6));
	mvwaddstr(wmain, 2, COLS-3-5,slots[currslot]->e.listed?"[LST]":"");

	wattrset(wmain, COLOR_PAIR(2));
	mvwhline(wmain, 3, 0, ACS_HLINE, COLS-2);wattron(wmain, A_BOLD);
	mvwprintw(wmain, 3, COLS/2-5, "[%s]", timestr(time(NULL)-online));
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


char qflgs[5]="NIHUW";
int  qclrs[5]={7,2,3,4,6};
void freshqueue()
{
	int i,k,l;
	char str[MAX_STRING];
	
	werase(wmain);
	wattrset(wmain,COLOR_PAIR(6)|A_BOLD);
	mvwaddstr(wmain,0,0,"* Address");
	mvwaddstr(wmain,0,COLS-25,"Mail  Files  Try  Flags");
	for(i=0;i<q_size && i<MH-1;i++) {
		wattrset(wmain,COLOR_PAIR(3)|(queue[i+qpos].flags&Q_DIAL?A_BOLD:0));
		mvwaddstr(wmain, i+1, 2, queue[i+qpos].addr);
		str[0]=0;
		sscat(str, queue[i+qpos].mail);strcat(str, "  ");
		sscat(str, queue[i+qpos].files);
		sprintf(str+strlen(str), "  %3d  ", queue[i+qpos].try);
		mvwaddstr(wmain, i+1, COLS-7-strlen(str), str);
		l=1;
		for(k=0;k<4;k++) {
			wattron(wmain, COLOR_PAIR(qclrs[k]));
			waddch(wmain, (queue[i+qpos].flags&l) ? qflgs[k] : '.');
			l=l<<1;
		}
		wattron(wmain, COLOR_PAIR(qclrs[4]));
		waddch(wmain, (queue[i+qpos].flags&Q_ANYWAIT) ? qflgs[4] : '.');
	}
}
void freshall()
{
	freshhdr();wnoutrefresh(whdr);
	freshstatus();wnoutrefresh(wstat);
	if(currslot>=0)
		freshproto();
	else
		freshqueue();
	wnoutrefresh(wmain);
	wnoutrefresh((currslot<0)?wlog:slots[currslot]->wlog);
	doupdate();
}

void sigwinch(int s)
{
/* 	struct winsize size; */
/* 	if (ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0) { */
/* 		wrefresh(curscr);wrefresh(stdscr); */
/* 		donescreen(); */
/* 		initscreen(); */
/* 		refresh(); */
/* 		freshall(); */
/* 	} */
/* 	signal(SIGWINCH, sigwinch); */
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

int main(int argc, char **argv)
{
	int quitflag=0, len, crc, ch, rc;
	fd_set rfds;
	struct timeval tv;
	char buf[MSG_BUFFER], tty[8], *p;
	time_t tim;
	struct tm *tt;
	
	if((sock=socket(AF_UNIX,SOCK_DGRAM,0))<0) {
		perror("can't create socket!");return 1;
	}
	unlink(Q_SOCKET);
	bzero(&server, sizeof(server));
	server.sun_family=AF_UNIX;
	strcpy(server.sun_path, Q_SOCKET);
	saddrlen = strlen(server.sun_path) + 1 + sizeof(server.sun_family);
	if(bind(sock, (struct sockaddr*)&server, saddrlen)<0) {
		perror("can't bind server socket!");return 1;
	}
	chmod(Q_SOCKET, 0777);
	initscreen();
	currslot=-1;
	allslots=0;
	freshhdr();wrefresh(whdr);
	bzero(&slots, sizeof(slots));bzero(&queue, sizeof(queue));
	q_size=0;/*strcpy(queue[0].addr, "2:5030/532.28");queue[0].flags=14;*/
	freshall();online=time(NULL);
	while(!quitflag) {
		tim=time(NULL);
		tt=localtime(&tim);wattron(whdr, COLOR_PAIR(15));
		mvwprintw(whdr, 0, COLS-13, "%02d:%02d:%02d",
				  tt->tm_hour, tt->tm_min, tt->tm_sec);
		wnoutrefresh(whdr);wnoutrefresh(wstat);
		doupdate();
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		FD_SET(sock, &rfds);
		tv.tv_sec=1;
		tv.tv_usec=0;
		rc=select(sock+1, &rfds, NULL, NULL, &tv);
		if(rc>0 && FD_ISSET(sock, &rfds)) {
			caddrlen=sizeof(client);bzero(buf, MSG_BUFFER);
			rc=recvfrom(sock, buf, MSG_BUFFER, 0,
						(struct sockaddr *)&client, &caddrlen);
			if(rc>=27 && !memcmp(buf, QC_SIGN, 6)) {
				sscanf(buf+6, "%04x", &len);
				sscanf(buf+10,"%04x", &crc);
				if(rc>=len+27 && crc==crc16(buf+27, len)) {
					memcpy(tty, buf+19, 8);
					p=strchr(tty, ' ');if(p) *p=0;else tty[7]=0;
					if(strcmp(tty, "master")) {
						rc=findslot(tty);
						if(!memcmp(buf+14, QC_ERASE, 5)) {
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
							rc=createslot(tty);
							freshhdr();wrefresh(whdr);
						}
						if(!memcmp(buf+14, QC_SLINE, 5)) {
							strncpy(slots[rc]->status, buf+27, MAX_STRING);
							freshstatus();wrefresh(wstat);
						}
						if(!memcmp(buf+14, QC_LOGIT, 5)) {
							logit(buf+27, slots[rc]->wlog);
							if(currslot==rc) {
								wrefresh(slots[rc]->wlog);
							}
						}
						if(!memcmp(buf+14, QC_TITLE, 5)) {
							strncpy(slots[rc]->header, buf+27, MAX_STRING);
							if(currslot==rc) {
								freshhdr();wrefresh(whdr);
							}
						}
						if(!memcmp(buf+14, QC_EMSID, 5)) {
							online=time(NULL);
							if(!len) {
								bzero(&slots[rc]->e, sizeof(pemsi_t));
								slots[rc]->session=0;
							} else {
								memcpy(&slots[rc]->e, buf+27,
									   sizeof(pemsi_t));
								slots[rc]->session=1;
							}
							if(currslot==rc) {								
								freshproto();wrefresh(wmain);
							}
						}
						if(!memcmp(buf+14, QC_LIDLE, 5)) {
							if(&slots[rc]->session) {
								slots[rc]->session=0;
								bzero(&slots[rc]->e, sizeof(pemsi_t));
								bzero(&slots[rc]->r, sizeof(pfile_t));
								bzero(&slots[rc]->s, sizeof(pfile_t));
								if(currslot==rc) {
									freshproto();wrefresh(wmain);
								}
							}
						}
						if(!memcmp(buf+14, QC_SENDD, 5)) {
							if(!len) {
								bzero(&slots[rc]->s, sizeof(pfile_t));
							} else {
								slots[rc]->session=1;
								memcpy(&slots[rc]->s, buf+27,
									   sizeof(pfile_t));
							}
							if(currslot==rc) {								
								freshproto();wrefresh(wmain);
							}
						}
						if(!memcmp(buf+14, QC_RECVD, 5)) {
							if(!len) {
								bzero(&slots[rc]->r, sizeof(pfile_t));
							} else {
								slots[rc]->session=1;
								memcpy(&slots[rc]->r, buf+27,
									   sizeof(pfile_t));
							}
							if(currslot==rc) {								
								freshproto();wrefresh(wmain);
							}
						}
						
					} else {
						if(!memcmp(buf+14, QC_SLINE, 5)) {
							strncpy(m_status, buf+27, MAX_STRING);
							if(currslot<0) {
								freshstatus();wrefresh(wstat);
							}
						}
						if(!memcmp(buf+14, QC_TITLE, 5)) {
							strncpy(m_header, buf+27, MAX_STRING);
							if(currslot<0) {
								freshhdr();wrefresh(whdr);
							}
						}
						if(!memcmp(buf+14, QC_LOGIT, 5)) {
							logit(buf+27, wlog);
							if(currslot<0) {
								wrefresh(wlog);
							}
						}
						if(!memcmp(buf+14, QC_QUEUE, 5)) {
							if(!len) {
								bzero(&queue, sizeof(pque_t));
								q_size=0;qpos=0;
							} else {
								memcpy(&queue[q_size], buf+27,
									   sizeof(pque_t));
								q_size++;
							}
							if(currslot<0) {								
								freshqueue();wrefresh(wmain);
							}
						}
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
		} else switch(ch) {
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
		case KEY_UP:
			if(currslot<0) {
				if(qpos>0) qpos--;
				freshqueue();wrefresh(wmain);
			}
			break;
		case KEY_DOWN:
			if(currslot<0) {
				if(qpos+MH<=q_size) qpos++;
				freshqueue();wrefresh(wmain);
			}
			break;
		}
	}
	donescreen();
	close(sock);
	p = (char *) malloc(strlen(Q_SOCKET) + 1);
	strcpy(p,Q_SOCKET);
	remove (p);
	free(p);
	unlink(Q_SOCKET);
	
	return 0;
}
