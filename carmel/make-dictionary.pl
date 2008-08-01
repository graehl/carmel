#!/usr/bin/perl

#outputs an FSA of character-recognizer for lines on STDIN (newline excluded)

my $end="END";
my $start=0;

print "$end\n";

my $s=1;

sub quote_char {
    my ($c)=@_;
    $c='\"' if $c eq '"';
    return qq{"$c"};
}

while(<>) {
    my $p=$start;
    chomp;
    my @c=split //,$_;
    for (0..$#c) {
        my $d=($_==$#c)?$end:$s++;
        print "($p $d ",&quote_char($c[$_]),")\n";
        $p=$d;
    }
}

