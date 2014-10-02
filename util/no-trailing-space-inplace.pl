#!/usr/bin/perl -i~
while(<>) {
    chomp;
    s/\s+$//;
    print $_,"\n";
}
