#!/bin/sh
if [ -n "$1" ]; then
perl -e '@file=<>;print "\xEF\xBB\xBF";print(@file);' < "$1" > "$1".tmp
mv "$1".tmp "$1"
else
perl -e '@file=<>;print "\xEF\xBB\xBF";print(@file);'
fi

