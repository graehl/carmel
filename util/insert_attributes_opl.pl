#!/bin/sh
#! -*- perl -*-
 eval 'exec perl -w -x $0 ${1+"$@"} ;'
  if 0;
#
# Author: graehl
# Created: Tue Aug 24 16:42:04 PDT 2004
#
# see usage (@end)
#

use strict;
use Getopt::Long;
use Pod::Usage;
use File::Basename;

### script info ##################################################
my $scriptdir; # location of script
my $scriptname; # filename of script

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
   push @INC, $scriptdir; # now you can say: require "blah.pl" from the current directory
}

#use WebDebug qw(:all);

### arguments ####################################################
&usage() if (($#ARGV+1) < 1); # die if too few args

my $id_regexp='id=(\d+)';

my $infile_name; # name of input file
my $fieldname;
my $oplfile;
my $base_index=1;
my $outfile=undef;

GetOptions("help" => \&usage,
           "idregexp:s" => \$id_regexp,
           "fieldname=s" => \$fieldname, # = required, : optional
           "oplfile=s" => \$oplfile,
           "base-index:i" => \$base_index,
#           "outfile:s" => \$outfile,
); # Note: GetOptions also allows short options (eg: -h)


sub usage() {
    pod2usage(-exitstatus => 0, -verbose => 2);
}

&usage unless defined($fieldname);

### main program ################################################

open OPL,$oplfile || die;
my $i_delta=-$base_index;

my @opls=<OPL>;
close OPL;

sub attrescape {
    #add appropriate escaping later
    my ($val)=@_;
    return $val;
}

my $line=0;
while(<>) {
    ++$line;
    if (/$id_regexp/) {
        my $index=$1+$i_delta;
        my $val=$opls[$index] if exists $opls[$index];
        if (defined($val)) {
            chomp $val;
            chomp;
            print "$_ $fieldname=",attrescape($val),"\n";
        } else {
            print STDERR "warning: no value defined on line $line of input, with id=$1, coming from line ",$1+$i_delta+1," of $oplfile\n";
            print;
        }
    }
}

__END__

=head1 NAME

insert_attributes_opl.pl

=head1 SYNOPSIS

insert_attributes_opl.pl [options] --fieldname emprob --oplfile em.weights rules_file

     Options:
       --help          
       --idregexp regexp
       --fieldname name
       --oplfile filename
       --base-index integer

=head1 OPTIONS

=over 8

=item B<-help>

    Print a brief help message (and that's all).

=item B<--idregexp 'regexp'>

    (default is "id=(\d+)").  $1 is the id checked against the user input

=item B<--fieldname name>

    name=blah is appended to the output where blah is the ith line of the opl
    file, and the input had id=i

=item B<--oplfile filename>

    attributes are pulled one per line from filename

=item B<--base-index integer>

    sets the id=index corresponding to the first line of oplfile (default=1).
    
=back

=head1 DESCRIPTION

    Given a one per line (opl) value file, appends fieldname=ith line of opl
    whenever idregexp=i is found in the input(s)

=cut
