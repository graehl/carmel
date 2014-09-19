#!/usr/bin/perl -w
use strict;
use utf8;
die 'why-empty.pl filea fileb' unless scalar @ARGV == 2;
my $a = shift;
my $b = shift;
sub opened {
    my $f;
    open($f, $_[0]) or die "open $_[0]";
    $f
}
my $af = &opened($a);
my $bf = &opened($b);
my @a = <$af>;
my @b = <$bf>;
my $an = scalar @a;
my $bn = scalar @b;
sub countwords {
#    my @a = split ' ',$_[0];
#    scalar @a;
    return length shift;
}
my @difflines;
die "#lines differ: $an != $bn for why-empty.pl $a $b" unless $an == $bn;
for (0.. ($an - 1)) {
    my $al = $a[$_];
    chomp $al;
    my $bl = $b[$_];
    chomp $bl;
    my $ac = &countwords($al);
    my $bc = &countwords($bl);
    if (($ac == 0) != ($bc == 0)) {
        print "$_: $ac $bc ||| $al ||| $bl\n";
        push @difflines, $_+1;
    }
}
my $ndiff = scalar @difflines;
die "$ndiff differently empty lines. line-numbers: ".join(' ',@difflines) if $ndiff;
