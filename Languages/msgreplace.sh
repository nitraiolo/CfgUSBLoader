#!/bin/sh
if [ $# != 3 ]; then
	echo "usage: $0 msgid_text search replace"
	exit
fi
for L in *.lang ; do
	echo $L
	./msgreplace.pl "$@" < $L > $L.new
	mv $L.new $L
done

