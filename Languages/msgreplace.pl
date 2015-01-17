#!/usr/bin/perl

#print scalar(@ARGV), ":", @ARGV, "\n";

if (scalar(@ARGV) != 3) {
	print "Usage: $0 msgid_text search replace\n";
	exit;
}

$msgid = $ARGV[0];
$search = $ARGV[1];
$replace = $ARGV[2];
shift @ARGV;
shift @ARGV;
shift @ARGV;

while (<>) {
	if (/^msgid/) {
		#print "MSGID: $_";
		$read_id = $_;
		while (<>) {
			if (/^msgstr/) { last; }
			$read_id .= $_;
		}
		# msgstr
		$read_str = $_;
		while (<>) {
			if (!/^"/) { last; }
			$read_str .= $_;
		}
		if ($read_id =~ /$msgid/) {
			$read_id =~ s/$search/$replace/g;
			$read_str =~ s/$search/$replace/g;
		}
		print $read_id;
		print $read_str;
	}
	print;
}

