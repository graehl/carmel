#!/usr/bin/perl -w
use strict;
use utf8;
binmode STDIN, ':utf8';
binmode STDOUT, ':utf8';

my $default_precision=$ENV{DIGITS}||5;

sub real_prec {
    my ($n,$prec)=@_;
    $prec=$default_precision unless defined $prec;
    sprintf("%.${prec}g",$n);
}

my $num_match=qr/[+\-]?(?:\.\d+|\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?)/;

while(<>) {
    s/($num_match)/real_prec($1)/eg;
    print;
}
