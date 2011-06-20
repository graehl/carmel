#!/usr/bin/env perl
use strict;
use warnings;
use Getopt::Long;

### script info ##################################################
use File::Basename;
my $scriptdir; # location of script
my $scriptname; # filename of script
my $BLOBS;

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
   push @INC, $scriptdir; # now you can say: require "blah.pl" from the current directory
   #  $ENV{BLOBS}='/home/nlg-01/blobs' unless exists $ENV{BLOBS};
   # $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
   #  $BLOBS=$ENV{BLOBS};
   #  my $libgraehl="$BLOBS/libgraehl/unstable";
   #  push @INC,$libgraehl if -d $libgraehl;
}

require "libgraehl.pl";

### arguments ####################################################

my $template_string='{sOmEnUm}';
my $raw_out;
my $sum_out=\*STDOUT;
my $sums;
my $avgonly;
my $grep;
my $N;
my $prec=8;
my $fullout;
my @options=(
"Any lines containing integer or floating point numbers are normalized and averages over each unique line (after normalization) are reported",
#             ["template-string=s"=>\$template_string,"Unique string that won't occur anywhere in the input"],
             ["raw-numbers-out=s"=>\$raw_out,"Write any recognized numbers to this file"],
             ["sums!"=>\$sums,"Compute sums"],
    ["avgonly!"=>\$avgonly,"Output average only"],
    ["fullout=s"=>\$fullout,"Output full bounds here even if avgonly (filename)"],
             ["grep=s"=>\$grep,"Regexp filtering input lines"],
             ["n=i"=>\$N,"Only record first n (matching) lines"],
    ["prec=i"=>\$prec,"digits of precision"],
            );


my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
&set_number_summary_prec($prec);
info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);


### main program ################################################

&argvz;

my $loose_num_match=qr/((?:[+\-]|\b)[0123456789]+(?:[.][0123456789]*(?:[eE][0123456789\-+]*)?)?)\b/;

my $RAW=openz_out($raw_out) if $raw_out;

my $l=0;
while(<>) {
    next if ($grep && ! /$grep/o);
    ++$l;
    if ($N and $l>$N) {
#        while(<>) {}
        last;
    } else {
        if ($raw_out) {
            while (/$loose_num_match/g) {
                print $RAW $1,"\t" if $raw_out;
            }
            print $RAW "\n";
        }
        log_numbers($_);
    }
}


&print_number_summary($sum_out,$sums,1,$avgonly);
if ($fullout) {
    $fullout=openz_out($fullout);
    &print_number_summary($fullout,0,1);
}
