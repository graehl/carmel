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
    $ENV{BLOBS}='/home/nlg-01/blobs' unless exists $ENV{BLOBS};
   $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    unshift @INC,$libgraehl if -d $libgraehl;
    unshift @INC, $scriptdir;
}

require "libgraehl.pl";

### arguments ####################################################

my ($fieldnames,$concat,$pass,$num,$list,$paste)=qw(hyp 1);

my $concatsep=' ';
my $addnum;
my $printfieldname;
my $infile_name;
my $outfile_name;
my $fenc='raw';
my $tenc='raw';
my $numsum=0;
my $numdefault=0;
my $doavg=0;
my $doprint=1;
my $whole_lines;
my $normalize_cost;

my @options=(
             "extract val with fieldname={{{val}}} or fieldname=val",
             ["fieldnames=s"=>\$fieldnames,"look for fieldname=s (multiple names may be given, separated by comma, e.g. 'hyp,tree' (empty fields are ignored)"],
             ["print-fieldname!"=>\$printfieldname,"don't remove the fieldname= part"],
             ["number-lines!"=>\$num,"prepend original line number before extracted fields"],
             ["paste-with-separator=s"=>\$paste,"append the original line to the extracted field, separated with s (normal double-quote escapes e.g. \\n)"],
             ["show-filename!"=>\$list,"prefix each found value with its filename"],
             ["concatenate!"=>\$concat,"if the attribute occurs multiply on a line, concatenate the values with spaces inserted between"],
             ["separator-for-concatenate=s"=>\$concatsep,"Use this string between concatenated fields"],
             ["add-numeric-for-concatenate!"=>\$addnum,"When multiple occurences of a numeric attribute are found, add them all instead"],
             ["min-avg-max"=>\$numsum,"Compute min/avg/max of fields (if they're numeric)"],
             ["avg"=>\$doavg,"Compute avg of fields"],
             ["default-for-avg"=>\$numdefault,"If an field is missing but others are found, assume this (for avg)"],
             ["do-print!" => \$doprint, "If --do-print, output values, if --nodo-print, just show summaries (and --pass-through lines)"],
             ["pass-through!"=>\$pass,"pass through to output any line that doesn't match fieldname=... (if you specify -paste, the separator will be inserted) - ignores --nodo-print"],
             ["whole-lines=s" => \$whole_lines,"any lines with matching --fieldnames are listed to this file intheir entirety. like --paste-with-separator but to different file"],
             ["outfile=s" => \$outfile_name,"Write output here (can be .gz)"],
             ["infile=s" => \$infile_name,"Take <file> as input (as well as the rest of ARGV)"],
    ["normalize-cost!" => \$normalize_cost,"accept N, 10^-N, e^-(2.3...*N) as the same"],
            );


my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);

if ($paste) {
    $paste=double_quote_interpolate($paste);
}

my @fields=split /,/,$fieldnames;
&debug("fields",@fields);

my $enc=set_inenc($fenc);
use open IN => $enc;

if ($infile_name) {
    info("Adding infile = $infile_name");
    unshift @ARGV,$infile_name;
}
outz_stdout($outfile_name);
set_outenc($tenc);
my $wholeto;
$wholeto=openz_out($whole_lines) if $whole_lines;

&argvz;

my $capture_3brackets='{{{(.*?)}}}';
my $capture_natural='(\d+)';
my $capture_int='(+?-?\d+)';
my $capture_cost='([-+e0-9]\S*)';

my $N=0;
my %sums;
my %sumsq;
my %nonzero;

while (<>) {
    &debug($_);
    my $braces=0;
    my $anyfound=0;
    my $line=$_;
    my %found=();
    for my $field (@fields) {
        next unless $field;
        &debug("trying $field on $line");
        my $whole=undef;
        while ($line =~ /\b(\Q$field\E)=({{{(.*?)}}}|([^{]\S*))/g) {
            my $whole_num=(!defined($whole) || is_numeric($whole));
            #    if ($_ =~ /$re/) {
            &debug($1,$2,$3,$4);
            $braces = 1 if defined $3;
            my $val=($braces ? $3 : $4);
            &debug($val,to_cost($val)) if $normalize_cost;
            $val=to_cost($val) if $normalize_cost;
            if ($addnum && is_numeric($val) && $whole_num) {
                no warnings 'uninitialized';
                $whole+=$val;
            } else {
                $whole=(defined $whole ? $whole.$concatsep.$val : $val);
            }
        }
        if (defined $whole) {
            if ($doprint) {
                if ($anyfound) {
                    print  ' ';
                } elsif ($list || $num) {
                    my ($file,$line)=&last_file_line;
                    print "$file:" if $list;
                    print "$line:" if $num;
                    print ' ';
                }
                print "$field=" if ($printfieldname);
                if ($braces && $printfieldname) {
                    print "{{{$whole}}}";
                } else {
                    print $whole;
                }
            }
            $anyfound=1;
            $found{$field}=$whole if $numsum;
            no warnings 'uninitialized';
            $sums{$field}+=$whole if $doavg;
            $sumsq{$field}+=$whole*$whole if $doavg;
            $nonzero{$field}+=1 if $doavg && $whole!=0;
        }
    }
    if ($anyfound) {
        if ($numsum) {
            log_numbers(join ' ',map {"$_=".hash_lookup_default(\%found,$_,$numdefault)} @fields);
        }
        ++$N;
        if ($doprint) {
            if ($paste) {
                print $paste,$_;    # has newline
            } else {
                print "\n";
            }
            print $wholeto $_ if ($wholeto);
        }
    } elsif ($pass) {
        print $paste if $paste;
        print;
    }
}

&info("found $N lines with the requested fields");
if ($doavg && $N) {
    print "Averages (N=$N):\n";
    for (@fields) {
        my $s=exists $sums{$_} ? $sums{$_} : 0;
        my $sq=exists $sumsq{$_} ? $sumsq{$_} : 0;
        my $nz=exists $nonzero{$_} ? $nonzero{$_} : 0;
        print "$_=",$s/$N," stddev=",sqrt(variance($s,$sq,$N))," N=$N N_nonzero=$nz\n";
    }
}
&print_number_summary if $numsum;
