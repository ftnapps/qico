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