#!/usr/bin/env perl
# input: srilm
# optional env: order (checks just that order).  nodup: skip dup check.  print: print events (without prob/bo)
# output: die on duplicates ngram events (no output, no exit code = no duplicates)

use warnings;
my $order=$ENV{order};
my $print=$ENV{print};
my $dup=!$ENV{nodup};

my %ctx;
my $N=0;
while(<>) {
    if (/^\\(\d+)-grams:\s*$/o) {
        $N=$1;
        print STDERR "starting $N-grams...\n";
    } elsif (/^\\end\\$/) {
        $N=0;
        %ctx=();
    } elsif ($N==0 || ($order&&$order!=$N) || /^\s*$/ ) {
    } else {
        my @w=split;
        my $ctx=join(' ',@w[1..$N]);
        if ($dup) {
            die "DUPLICATE ($ARGV): $ctx :\n$_ " if exists $ctx{$ctx};
            $ctx{$ctx}=1;
        }
        print $ctx,"\n" if $print;
    }
}
