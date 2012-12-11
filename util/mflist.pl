#!/usr/bin/perl
my $d="\t";
while(<>) {
    last if /Most Recent Quarter Data/;
}
$l=10;
while(<>) {
if (/<td align="left">([^<]+)<\/td>/) { $n=$1; $l=0; } else {
    if (/<td align="center">([^<]+)<\/td>/) {
        if ($l==0) {
            $t=$1;
        } elsif ($l==1) {
            $c=$1;
            $c=~s/,//g;$c=int($c+.499);
            print "$t$d$c$d$n\n";
        }
    } else {
        $l=10;
}
    ++$l;
} }
