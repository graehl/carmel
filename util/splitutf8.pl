#!/usr/bin/perl -CSDA
use utf8;
use 5.014;

my $space = $ENV{chars} ? '' : ' ';

while(<>) {
    chomp;
    my @f=split $space,$_;
    my $n = scalar @f;
    my $mid = int(($n + 1) / 2);
    my @right = splice @f, $mid;
    print STDERR join($space, @right),"\n";
    print join($space, @f),"\n";
}
