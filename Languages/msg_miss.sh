#!/bin/sh

function del_bom
{
	perl -e '@file=<>;$file[0] =~ s/^\xEF\xBB\xBF//;print(@file);'
}

function add_bom
{
	perl -e '@file=<>;print "\xEF\xBB\xBF";print(@file);'
}

for L in *.lang ; do
	LM=`basename $L .lang`_miss.lang
	echo $L $LM
	del_bom < $L | msgattrib --untranslated | add_bom > $LM
done

