#!/usr/bin/perl -i~

my $id = '[A-Za-z_][0-9A-Za-z_]*';
while(<>) {
    s/(struct|class)\s+($id)({)/$1 $2 $3/;
    s/(namespace)\s+($id)({)/$1 $2 $3/;
    print;
}
