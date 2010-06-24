#!/usr/bin/env perl

my $member='(?:\w+\.)';
while(<>) { 
#    print "$1=$2;\n" if (\&$member(\w+)\)\-\>default_value\(([^,]*)(\s*,"[^"]*")?\),\s*("|$)/);
    print "$1=false;\n" if (/bool_switch\(\&$member(\w+)\)/);
}
