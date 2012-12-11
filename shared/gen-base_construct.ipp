#!/usr/bin/perl
use warnings;
#usage: $0 4 10
# outputs 2^5 variants of const/non
# and 10 const-only

my $cn=shift || 4;
my $n=shift || 16;


print "
/*

intended to forward constructor args to parent

will be unnecessary in C++11

no include guard (intentional - include it inside your class that defines these:

typedef A self_type;
typedef B base_type;
void init();

generated from $0

*/

self_type() : base_type() { init(); }

";

$.=' ';
$,=' ';
sub ctor {
    print "template <";
    my @n=(1..scalar @_);
    print join(', ',(map {"class T$_"} @n));
    print ">";
    print " self_type(";
    print join(', ',(map {"T$_".($_[$_-1]?" const& ":"& ")."t$_"} @n));
    print ")\n : base_type(";
    print join(', ',(map {"t$_"} @n));
    print ") {init();}\n";
}
my $i;
sub rcn {
    my @cn=@_;
    die unless $i;
    if (scalar @cn==$i-1) {
        &ctor(@cn,0);
        &ctor(@cn,1);
    } else {
        &rcn(@cn,0);
        &rcn(@cn,1);
    }
}

for (1..$n) {
    $i=$_;
# print STDERR "$i $cn $n @cn\n";
    if ($i<=$cn) {
        &rcn();
    } else {

        &ctor((0) x $i);
    }
}
