# $Id: x2c.awk,v 1.3 2004/01/10 09:24:40 sisoft Exp $
BEGIN {
    C="qconf.c"; H="qconf.h"
    print "#include \"headers.h\"\n" >C
    print "cfgstr_t configtab[CFG_NNN+1]= {" >C
    print "#ifndef __QCONF_H__" >H
    print "#define __QCONF_H__\n" >H
    i=0
}

/^[^\#]/ {
    print "\t{\"" tolower($1) "\", " $2 ", " $4 ", 0, NULL, " $5 "}," >C
    print "#define CFG_" toupper($1) "\t\t" i >H
    i++
}

END {
    print "{NULL,0,0,0,NULL,NULL}};" >C
    print "\n#define CFG_NNN\t\t" i >H
    print "extern cfgstr_t configtab[CFG_NNN+1];" >H
    print "#endif" >H
}
