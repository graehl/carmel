#!/usr/bin/env perl
#input: one per line ghkm-format trees ... (NN dog) (-LRB- () (-RRB- ))
#output: one per line yield ... dog ( )
my $DEBUG=$ENV{DEBUG};
while(<>) {
    my $sp='';
    while (/\(([^() ]+) ([^ ]+)\)( |$)/g) {
        my ($pos,$lex)=($1,$2);
        print STDERR "($pos $lex) " if $DEBUG;
        print "$sp$lex";
        $sp=' ';
    }
    print STDERR "\n" if $DEBUG;
    print "\n";
}
