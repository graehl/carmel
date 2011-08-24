#!/usr/bin/env perl
#
# Author: graehl

use strict;
use warnings;
use Getopt::Long;

### script info ##################################################
use FindBin;
use lib $FindBin::RealBin;
my $BLOBS;

BEGIN {
    $ENV{BLOBS}='/home/nlg-01/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    push @INC,$libgraehl if -d $libgraehl;
}

require "libgraehl.pl";

### arguments ####################################################

my ($fieldnames,$concat,$pass,$num,$list,$paste)=('',1);

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
my $doavg;
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
my $re;
my $prec=7;
my $dostddev;
my $sentslope;
my $nofailed=1;

my @options=(
             "extract val with fieldname={{{val}}} or fieldname=val",
             ["fieldnames-to-files=s"=>\$fieldnames,"comma separated list of fieldname[:outfile], with no outfile -> STDOUT, indicating where each value is written, e.g. 'hyp:hyp-file,sent:sent-file,lm-cost'look for fieldname=s (multiple names may be given, separated by comma, e.g. 'hyp,tree' (trailing/leading commas ignored)"],
    ["regexp-fieldname=s"=>\$re,"for avgs only"],
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
    ["prec=i"=>\$prec,"digits precision for avgs"],
    ["avg!"=>\$doavg,"compute average vals for regexp-fieldname"],
    ["stddev!"=>\$dostddev,"also compute stddev over all vals"],
    ["sentslope!"=>\$sentslope,"compute beta of least squares regression across repeats for same sentence, in order for same sent. can supply two 1best files"],
    ["nofailed!"=>\$nofailed,"skip failed-parse=1 lines"],
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
my %hypuniq;
my $nuniq;
my $lastsent;
my $sentno=0;
my %sums;
my %sumsq;
my %nonzero;
my %fsentvals; # {fname}{sent}[occurence#]=x
my %nsent;
my $Ngradient=0;

while (<>) {
    my $nbest=$1 if /\bnbest=(\d+)\b/;
    my $sent=$1 if /\bsent=(\d+)\b/;
    my $inbest; # position in occurences of htis sent @ any nbest not skipped
    if (/^\s*$/ || $nofailed && /\bfailed-parse=1\b/ || defined($maxnbest) && defined($nbest) && $nbest > $maxnbest) {
        ++$Nskip;
        next;
    }
    if (defined($sent)) {
        no warnings 'uninitialized';
        $inbest=$nsent{$sent}++;
        $Ngradient++ if ($inbest==1);
    }
    my $braces=0;
    my $anyfound=0;
    my $line=$_;
    my %found=();
    my $any;
    while ($line =~ /\b([\w\-][^=]*)=({{{(.*?)}}}|([^{]\S*))/go) {
        my $key=$1;
        my $look=$lookfor{$key};
        next unless $re || $look;
        $braces = 1 if defined $3;
#        my $val=($braces ? $3 : $4);
        my $val=defined $3 ? $3 : $4;
        $found{$key}=$val if $look;
        next unless ($re && $key =~ /^$re$/o);
        no warnings 'uninitialized';
        my $l=exists $fsentvals{$key}{$sent} ? $fsentvals{$key}{$sent} : ($fsentvals{$key}{$sent}=[]);
        at_grow_default($l,$inbest,0,$val) if ($sentslope && defined $sent);
        $sums{$key}+=$val if $doavg;
        $sumsq{$key}+=$val*$val if $doavg;
        $nonzero{$key}+=1 if $doavg && $val!=0;
        $any=1;
    }
    my $sr=\$found{$sentfield};
    if (defined $$sr) {
        if (!defined($lastsent) || $lastsent ne $$sr) { # new sent
            ++$sentno;
            if (!defined($lastsent) && defined($renumbersent)) {
                $sentno=$renumbersent;
                info("Renumbered new sentence $lastsent to $sentno");
            }
            %hypuniq=();
            $nuniq=0;
            $lastsent=$$sr;
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
if ($doavg && $N) {
    &info("Averages (N=$N):\n");
    for (keys %sums) {
        my $s=$sums{$_};
        my $sq=exists $sumsq{$_} ? $sumsq{$_} : 0;
        my $nz=exists $nonzero{$_} ? $nonzero{$_} : 0;
        print "$_=",real_prec($s/$N,$prec);
        print " stddev=",real_prec(sqrt(variance($s,$sq,$N)),$prec) if $dostddev;
        print " nonzero=$nz/$N\n";
    }
}
if ($sentslope && $N) {
    print "Feature gradients (avg over N=$Ngradient sents with >1 nbest out of $N):\n";
    for my $f (keys %fsentvals) {
        my $sv=$fsentvals{$f};
        my $n=0;
        my $sum=0;
        for my $sent (keys %$sv) {
            my $vl=$sv->{$sent};
            if (scalar @$vl > 1) {
                my ($slope,$intercept)=linear_regress($vl);
                $sum+=$slope;
                ++$n;
            } else {
#                count_info_gen("No gradient for sent=[$sent] (only 1 nbest)");
            }
                #TODO: incremental regression w/ linear_regress_counts - don't remember specific values
        }
        if ($n>0) {
            my $avg=$n ? $sum/$n : 0;
            $avg=real_prec($avg,$prec);
            print "$f delta=$avg N=$n\n";
        } else {
#            warn("No gradient for $f (no sentence with >1 nbest)");
            count_info("only 1 nbest for feature - no gradient");
        }
    }
}
