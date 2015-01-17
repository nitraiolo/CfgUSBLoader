#!/bin/sh
perl -e '@file=<>;$file[0] =~ s/^\xEF\xBB\xBF//;print(@file);'
