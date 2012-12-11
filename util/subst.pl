#!/usr/bin/env perl
#
# Author: graehl

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
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
   $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    push @INC,$libgraehl if -d $libgraehl;
}

require "libgraehl.pl";

### arguments ####################################################

my $ttable;
my $isregexp;
my $sep="\t";
my $substflags="";
my $parallel;
my $inplace;
my $reverse;
my $wholeword;
my $dryrun;
my $firstonly;
my $verbose;
my $substre;
my @substs;
my $abspath=1;

my @options=(
"Global or regexp search and replace from a translation file (list of tab-separated source/replacement pairs)",
["abspath!"=>\$abspath,"for inplace, modify pointed to file by absolute path (don't remove symlink)"],
["translations-file=s"=>\$ttable,"list of tab-separated source/replacement pairs"],
["reverse!"=>\$reverse,"reverse: replace second column in translations-file with first column"],
["inplace!"=>\$inplace,"in-place edit (note: cannot handle compressed inputs)"],
["eregexp!"=>\$isregexp,"treat source as regexp"],
["substregexp!"=>\$substre,"treat ttable lines as arbitrary s/whatever/to/g lines to be eval"],
["wholeword!"=>\$wholeword,"match only at word boundaries (\\b)"],
["dryrun!"=>\$dryrun,"show substituted lines on STDOUT (no inplace)"],
["firstonly!"=>\$firstonly,"don't process subsequent translations after the first matching per line"],
["verbose!"=>\$verbose,"show each applied substitution"],
#["substflags=s"=>\$substflags,"flags for s///flags, e.g. e for expression"],
#["parallel!"=>\$parallel,"perform at most one replacement per matched section"],
);


my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

my @rewrites;
my $fh=openz($ttable);
while(<$fh>) {
    chomp;
    if ($substre) {
        push @substs,[eval "sub { $_ }",$_];
    } else {
        my ($find,$replace)=split /$sep/;
        next unless $find;
        unless ($replace) {
            next if $reverse;
            $replace="";
        }
        ($find,$replace)=($replace,$find) if $reverse;
        &debug($find,$replace);
        my $retext=$isregexp ? $find : qq{\\Q$find\\E};
#        my $replacetext=$isregexp ? $replace : qq{\\Q*replace\\E};
        $retext=qq{\\b$retext\\b} if $wholeword;
        &debug($retext);
        push @rewrites,[eval("qr{$retext}"),$replace,"s{$find}{$replace}"];
    }
}

if ($inplace) {
    my %modify_files;
    my $hadargs=scalar @ARGV;
    file: for my $file (@ARGV) {
        open LOOKFOR,'<',$file or die "$file: ".`ls -l $file`;
        while(my $line=<LOOKFOR>) {
            for my $sd (@rewrites) {
                my $source=$sd->[0];
                if ($line =~ /$source/) {
                    $modify_files{$file}=1;
                    &debug("translation matched $file: $source");
                    next file;
                }
            }
            for my $sdesc (@substs) {
                my ($s,$desc)=@{$sdesc};
                $_=$line;
                if ($s->()) {
                    $modify_files{$file}=1;
                    &debug("substition matched $file: $desc");
                    next file;
                }
                $_=$line;
                eval $s;
            }
        }
        close LOOKFOR;
    }
@ARGV=keys %modify_files;
@ARGV=uniq(map { abspath($_) } @ARGV) if $abspath;
count_info("modifying $_") for (@ARGV);
    &debug(@ARGV);
    if ($hadargs && scalar @ARGV == 0) {
        fatal("None of the input files match any patterns in $ttable for in-place edit - no change");
    }
    $^I = "~" unless $dryrun;
} else {
    &argvz;
}


sub count_subst {
    my ($desc,$pre,$n)=@_;
    if ($n) {
        my $rec="$ARGV: $desc";
        count_info($rec,$n);
        count_info("BEFORE: $pre",$n);
        info("$rec\n\tBEFORE: $pre") if $verbose;
        print if $dryrun;
    }
}

while(<>) {
    my $pre=$_;
    chomp($pre);
    for my $sdd (@rewrites) {
        my ($source,$dest,$desc)=@{$sdd};
        my $n=s/$source/$dest/g;
#        &debug($desc,$n,$_);
        count_subst($desc,$pre,$n);
        last if $firstonly && $n;
    }
    for my $sd (@substs) {
        my ($s,$desc)=@{$sd};
        my $n=$s->();
        count_subst($desc,$pre,$n);
        last if $firstonly && $n;
    }
    print unless $dryrun;
}
info_summary();
