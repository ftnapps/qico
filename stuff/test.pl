# test and info perl script for qicosi.

# available qico functions:
#  sub wlog(string): write string to log.

# default commands (outside any subs), executig before on_load().
$|=1;

# called for initialization.
# %conf: hash of config values.
# $daemon: true, if script loaded into daemon, not in line.
# $init: true on init, and false after reload configs or daemon forking.
sub on_load {
    my $re=$init?"":"re";
    wlog("normally ${re}loaded, daemon=$daemon");
    wlog("main addr: $conf{address}[0], sysop: $conf{sysop}");
    wlog("first password: $conf{password}{adr}[0] '$conf{password}{str}[0]'");
}

# called before exit, for deinitialization.
# $emerg: 0=normal exit, 1=emergency exit.
sub on_exit {
    wlog("try to exit (emerg=$emerg)");
}

# called before write to log.
# $_: full log line.
# return: 1 if log string changed, 0 else.
sub on_log {
    my $rc=0;
    if(/poll /) {
    	s/poll/big hello/;
	$rc=1;
    }
    open F,">>/tmp/qlog";
    print F $_."\n" unless /DBG_/;
    close F;
    return $rc;
}

# called
# 
sub on_call {
    wlog("on_call to $addr by $phone, ip=$tcpip, bink=$binkp.");
    return 0;
}
