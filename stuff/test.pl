# test and info perl script for qicosi.
# $Id: test.pl,v 1.8 2004/06/22 14:26:21 sisoft Exp $

# available qico functions:
#  sub wlog([level,]string): write string to log.
#  sub setflag(num,bool): set user perl flag num to bool.
#  sub qexpr(string): test qico expression (as in config),
#                     returns bool result, or -1 if bad expression.

# available hooks:

# called for initialization.
# $conf: config file name.
# %conf: hash of config values.
# $daemon: 1 if script loaded into daemon, 0 else.
# $init: 1 on init, and 0 after reload configs or daemon forking.
# return: none.
sub on_init {
    my $re=$init?"":"re";
    wlog("normally ${re}loaded, daemon=$daemon, ver=$version");
    wlog("from $conf:");
    wlog(0," main addr: $conf{address}[0], sysop: $conf{sysop}");
    wlog(1,"first password line: $conf{password}{adr}[0] '$conf{password}{str}[0]'");
}

# called before exit, for deinitialization.
# $emerg: 0=normal exit, 1=emergency exit.
# return: none.
sub on_exit {
    wlog("on_exit() (emerg=$emerg)");
}

# called before write to log.
# $_: full log line.
# return: true if log string ($_) changed, else false.
sub on_log {
    my $rc=0;
    if(/poll /) {
    	s/poll/big hello/;
	$rc=1;
    }
    setflag(0,1) if(/\.log/);
    open F,">>/tmp/qlog";
    print F $_."\n" unless /DBG_/;
    close F;
    return $rc;
}

# called before dialing or connecting (if IP), after exec runonsession.
# $addr: calling address.
# $site: phone or host of calling system.
# $tcpip: 1 if call over tcp/ip, 0 else.
# $binkp: 1 if call via Binkp protocol, 0 else.
# $port: modem port (ex.: ttyS1) or "ip" if ip call.
# return: combination of S_* constants. aborting call, if return!=$S_OK.
sub on_call {
    wlog("on_call() to $addr by $site, ip=$tcpip, bink=$binkp, port=$port.");
    return $S_BUSY if($addr eq '2:5050/125.522');
    return $S_OK;
}

# called after handshake, before start transfer.
# @addrs: list of our akas.
# @akas: list of remote akas.
# $start: unix time of session start.
# %info: hash of strings: sysop, station, mailer, place, flags, wtime, password,
#        and integers: time, speed, connect.
# $flags: protocol and session flags.
# %flags: hash of booleans: in, out, tcp, secure, listed.
# %queue: hash of integers: mail, files, num.
# @queue: list of outgoing files, format such as in *.?lo.
# return: $S_OK for continue session, any other S_* for abort with this status.
sub on_session {
    wlog("on_session() time=$start, dir=$flags{out}, sec=$flags{secure}, lst=$flags{listed}, tcp=$flags{tcp}.");
    wlog(" $info{sysop}, sysop of <$info{station}>, using $info{mailer}");
    wlog(" live in '$info{place}' and work in '$info{wtime}' (now time=$info{time})");
    wlog(" he akas: @akas, our akas: @addrs");
    wlog(" remote has flags '$info{flags}' and system flags '$flags'");
    wlog(" need to sent $queue{mail} bytes of mail and $queue{files} bytes of files ($queue{num} files)");
    return $S_NODIAL if(qexpr("!time 23-7")==1);
    return $S_OK;
}

# called after session.
# $r_bytes: bytes received.
# $r_files: files received.
# $s_bytes: bytes sended.
# $s_files: files sended.
# $result: session result, set of S_* constants.
# $sesstime: session time in seconds.
# return: none.
sub end_session {
    wlog("end_session() (rb,rf,sb,sf,rc,time)=($r_bytes,$r_files,$s_bytes,$s_files,$result,$sesstime).");
}

# called before file receiving.
# %recv: hash of values: name, size, time.
# return: one of $F_* constants.
sub on_recv {
    wlog("on_recv() name=$recv{name}, size=$recv{size}, time=$recv{time}.");
    return $F_OK;
}

# called afrer receiving file, before writing.
# $state: receiving status (one of F_* constants).
# return: empty string for default actions, undef for kill file, or new file name.
sub end_recv {
    my $s=qw/F_OK F_CONT F_SKIP F_ERR F_SUSPEND/[$state];
    wlog("end_recv() file=$recv{file}, state=$s.");
    return "";
}

# called before file sending.
# %send: hash of values: file, name, size, time.
# return: empty string for send, undef for skip, or new file name.
sub on_send {
    wlog("on_send() file=$send{file}, name=$send{name}, size=$send{size}, time=$send{time}.");
    return "";
}

# called after file sending.
# $state: sending status (one of F_* constants).
# return: none.
sub end_send {
    my $s=qw/F_OK F_CONT F_SKIP F_ERR F_SUSPEND/[$state];
    wlog("end_send() file=$send{file}, state=$s.");
}
