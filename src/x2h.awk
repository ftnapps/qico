# $Id: x2h.awk,v 1.1.1.1 2003/07/12 21:27:19 sisoft Exp $
BEGIN {
 print "#ifndef __QCONF_H__"
 print "#define __QCONF_H__"
 print ""
 i = 0
}
/^[^\#]/ {
 print "#define CFG_" toupper($1) "\t\t" i 
 i++;
}
END {
 print ""
 print "#define CFG_NNN\t\t" i 
 print "extern cfgstr_t configtab[CFG_NNN+1];"
 print "#endif"
}