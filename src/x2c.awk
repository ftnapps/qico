# $Id: x2c.awk,v 1.1.1.1 2003/07/12 21:27:19 sisoft Exp $
BEGIN {
 print "#include \"headers.h\""
 print
 print "cfgstr_t configtab[CFG_NNN+1]={" 
}
/^[^\#]/ {
 print "{\"" tolower($1) "\", " $2 ", " $4 ", 0, NULL, " $5 "},"
}
END {
 print "{NULL,0,0,0,NULL,NULL}};"
}