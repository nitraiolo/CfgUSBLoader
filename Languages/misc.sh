#!/bin/sh
for L in *.lang ; do
	echo $L
	./msgreplace.pl "Press " "\b[AB12]\b" "%s" < $L > $L.new
	mv $L.new $L
	./msgreplace.pl "Press " "[-+] " "%s " < $L > $L.new
	mv $L.new $L
	./msgreplace.pl "Press " "Home" "%s" < $L > $L.new
	mv $L.new $L
	./msgreplace.pl "Press " "LEFT/RIGHT" "%s/%s" < $L > $L.new
	mv $L.new $L
done

