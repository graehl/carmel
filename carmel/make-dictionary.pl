#!/usr/bin/perl

use Getopt::Long;

#outputs an FSA of character-recognizer for lines on STDIN (newline excluded)

my $end="END";
my $start=0;

print "$end\n";

my $s=1;
my $random=0;
my $weighted=0;

GetOptions("random!"=>\$random
,"weighted!"=>\$weighted
) || die;


sub quote_char {
    my ($c)=@_;
    $c='\"' if $c eq '"';
    return qq{"$c"};
}

my $num_match=qr/(?:[+\-]|\b)[0123456789]+(?:[.][0123456789]*(?:[eE][0123456789\-+]*)?)?/;

while(<>) {
    my $w=1;
    if ($weighted) {
        s/((?:e\^|10\^)?$num_match(?:ln|log)?)\s+// || die "no weight found for line $_ with --weighted";
        $w=$1;
    }
    $w=1-rand(1) if $random;
    my $p=$start;
    chomp;
    my @c=split //,$_;
    for (0..$#c) {
        my $d=($_==$#c)?$end:$s++;
        print "($p $d ",&quote_char($c[$_]);
        print " $w" if $w ne '1' && $_==0;
        print ")\n";
        $p=$d;
    }
}

