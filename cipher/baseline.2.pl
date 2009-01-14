#!/usr/bin/perl -w
use strict;

open A,"test.freq" or die;
open B,"train.freq" or die;

my $N=0;
my $right=0;
my $Ndict=0;
my $rightdict=0;

while(<A>) {
    my $a=$_;
    my $b=<B>;
    last unless defined $b;
    my ($na,$wa)=split ' ',$a;
    my ($nb,$wb)=split ' ',$b;
    $Ndict++;
    $N+=$na;
    if ($wa eq $wb) {
        print STDERR $a;
        $right+=$na;
        $rightdict++;
    }
}

print "per-word ($rightdict correct out of $Ndict unique test words) accuracy: ",$rightdict/$Ndict,"\n";
print "per-running-text (out of $N running test words) accuracy: ",$right/$N,"\n";

