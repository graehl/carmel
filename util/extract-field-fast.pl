#!/usr/bin/env perl
#
# Author: graehl

use strict;
use warnings;
use Getopt::Long;

my $blobbase="/home/hpc-22/dmarcu/nlg/blobs";

### script info ##################################################
use FindBin;
use lib $FindBin::RealBin;
my $BLOBS;

BEGIN {
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    push @INC,$libgraehl if -d $libgraehl;
}

require "libgraehl.pl";

### arguments ####################################################

my ($fieldnames,$concat,$pass,$num,$list,$paste)=qw(hyp 1);

my $concatsep=' ';
my $addnum;
my $printfieldname;
my $nofieldnameinfiles;
my $infile_name;
my $outfile_name;
my $fenc='raw';
my $tenc='raw';
my $numsum=0;
my $numdefault=0;
my $doavg=1;
my $doprint=1;
my $printzeros=1;
my $missingas;
my $omitblank=1;
my $renumbersent;
my $sentfield="sent";
my $hypfield="hyp";
my $nbestfield="nbest";
my $maxuniquehyp;
my $maxnbest;

my @options=(
             "extract val with fieldname={{{val}}} or fieldname=val",
             ["fieldnames-to-files=s"=>\$fieldnames,"comma separated list of fieldname[:outfile], with no outfile -> STDOUT, indicating where each value is written, e.g. 'hyp:hyp-file,sent:sent-file,lm-cost'look for fieldname=s (multiple names may be given, separated by comma, e.g. 'hyp,tree' (trailing/leading commas ignored)"],
             ["print-fieldnames!"=>\$printfieldname,"don't remove the fieldname= part"],
             ["show-zeros!"=>\$printzeros,"show 0 values (note: unless printing fieldname, ALWAYS show 0"],
             ["skip-lines-notfound!"=>\$omitblank,"Don't print anything if none of the fields were found on that line"],
             ["missing-default-as=s"=>\$missingas,"for absent values, pretend the value was this"],
             ["only-stdout-fieldnames!"=>\$nofieldnameinfiles,"keep the fieldname= part only for fieldnames without an outfile"],
             ["outfile=s" => \$outfile_name,"Write output here (can be .gz)"],
             ["infile=s" => \$infile_name,"Take <file> as input (as well as the rest of ARGV)"],
             ["renumber-sents-from=i"=>\$renumbersent,"Special case for the 'sent' fieldname - every time it changes, give it the next highest number, starting from i"],
             ["sent-fieldname=s"=>\$sentfield,"(used by renumber-sents-from)"],
             ["max-nbest=i"=>\$maxnbest,"Special case for 'nbest' fieldname - skip line if value is > i"],
             ["unique-hyps=i"=>\$maxuniquehyp,"Special case for 'hyp' fieldname - skip line if hyp has already been seen for this sent-fieldname"],
            );


my $cmdline=&escaped_cmdline;
my ($usagep,@opts)=getoptions_usage(@options);
info("COMMAND LINE:");
info($cmdline);
show_opts(@opts);
$missingas=0 unless $printfieldname;

my @f=split /,/,$fieldnames;
my @fields;
my %lookfor;
my %fhs;
for (@f) {
    my $name=$_;
    my $file='STDOUT';
    my $fh;
    my $printfield=$printfieldname || $nofieldnameinfiles;
    if (/(.*):(.*)/) {
        $name=$1;
        $file=$2;
        if (exists $fhs{$file}) {
            &debug("output $file already exists");
            $fh=$fhs{$file};
        } else {
            &debug("creating output $file");
            open $fh,'>',$file or die ">$file: $?";
            $fhs{$file}=$fh;
        }
        $printfield=0 if $nofieldnameinfiles;
    } else {
        $fhs{$file}=$fh=\*STDOUT;
    }
    info("looking for field: $name with output -> $file");
    push @fields,[$name,$fh,$printfield];
    $lookfor{$name}=1;
}

my @allfhs=values %fhs;

&debug("fields",@fields);

my $enc=set_inenc($fenc);
use open IN => $enc;

if ($infile_name) {
    info("Adding infile = $infile_name");
    unshift @ARGV,$infile_name;
}
outz_stdout($outfile_name);
set_outenc($tenc);

&argvz;

my $N=0;
my $Nskip=0;
my %sums;
my %hypuniq;
my $nuniq;
my $lastsent;
my $sentno=0;

while (<>) {
    if (defined $maxnbest && /\bnbest=(\d+)\b/ && $1 > $maxnbest) {
        ++$Nskip;
        next;
    }
    my $braces=0;
    my $anyfound=0;
    my $line=$_;
    my %found=();
    while ($line =~ /\b([\w\-]+)=({{{(.*?)}}}|([^{]\S*))/go) {
        my $key=$1;
        next unless $lookfor{$key};
        $braces = 1 if defined $3;
#        my $val=($braces ? $3 : $4);
        $found{$key}=defined $3 ? $3 : $4;
#        &debug($1,$2,$3,$4);
    }
    my $sr=\$found{$sentfield};
    if (defined $$sr) {
        if (!defined($lastsent) || $lastsent ne $$sr) { # new sent
            ++$sentno;
            if (!defined($lastsent) && defined($renumbersent)) {
                $sentno=$renumbersent;
            }
            %hypuniq=();
            $nuniq=0;
            $lastsent=$$sr;
            info("Renumbered new sentence $lastsent to $sentno");
        } else { # same sent
        }
        if (defined $maxuniquehyp) {
            next if ($nuniq > $maxuniquehyp);
            if (defined $found{$hypfield}) {
                if (exists $hypuniq{$found{$hypfield}}) {
                    #hyp already existed
                    &debug("repeated hyp: $N");
                    ++$Nskip;
                    next;
                } else {
                    &debug("new hyp: $N");
                    $hypuniq{$found{$hypfield}}=1;
                    ++$nuniq;
                }
            }
        }
        $$sr=$sentno if (defined $renumbersent);
    }
    for (@fields) {
        my ($name,$fh,$printfield)=@$_;
        my $val=$found{$name};
        if (!defined($val)) {
            if (defined($missingas)) {
                $val=$missingas;
            } else {
                next;
            }
        }
        if ($val || $printzeros || !$printfield) {
            print $fh "$name=" if $printfield;
            print $fh "$val ";
        }
    }
    print $_ "\n" for @allfhs;
    ++$N;
}

&info("output $N lines with the requested fields");
&info("skipped $Nskip lines") if $Nskip;
