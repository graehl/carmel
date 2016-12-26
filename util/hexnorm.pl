#!/usr/bin/perl -i~
my %id;
my $idre = qr/(?:0x|thread:)([0-9a-f]+)/;
my $opre = '#x';
my $nid = 0;
sub getid {
    my ($x) = @_;
    $opre.(exists $id{$x} ? $id{$x} : ($id{$x} = $nid++));
}
while(<>) {
    s/$idre/getid($1)/eg;
    print;
}
