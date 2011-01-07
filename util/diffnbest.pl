#!/usr/bin/env perl

use warnings;
use strict;

my $QUEUE='isi';
my $JOBSDIR=$ENV{HOME}.'/isd/jobs'; # $curdir/jobs
$JOBSDIR=$ENV{HOME}.'/jobs' unless -d $JOBSDIR;
my $ntop=12;
my $ntopskip=4;

use Getopt::Long qw(:config require_order);
use Pod::Usage;
use File::Basename;

### script info ##################################################
my $scriptdir;                  # location of script
my $scriptname;                 # filename of script
my $BLOBS;

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
   $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
   $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
   $BLOBS=$ENV{BLOBS};
   my $libgraehl="$BLOBS/libgraehl/unstable";
   unshift @INC,$libgraehl if -d $libgraehl;
   unshift @INC, $scriptdir;
}

require "libgraehl.pl";

#package Nbest;
### TODO: 
### RECOVERED Hypothesis: sent=3 hyp={{{...
#instead use extract-hypothesis.pl

my $EPSILON=exists $ENV{EPSILON} ? $ENV{EPSILON} : .001;
#NOTE: EPSILON=.0001 gives false positive changed totalcosts!


### arguments ####################################################

my $origcmd=&cmdline;
print STDERR "CMDLINE:\n$origcmd\n";

my ($file,$allchanges,$topn,$sentence,$rulesfile,$maxsents);
my $brief=0;
my $quiet=0;
my $ignoresent=0;
my $dumprules=0;
my $pretty=0;
my $ugly=0;
my $onlycomparesame=0;
my $corpusdir; # f.plain e.0 e.1 ats-hyp
my $fieldname='totalcost';
my $getopl=0;
my $topruleid;

GetOptions("help" => \&usage,
           "ignoresent!",\$ignoresent,
           "allchanges!",\$allchanges,
           "n=i",\$topn,
           "sentence=i",\$sentence,
           "brief!",\$brief,
           "quiet!",\$quiet,
           "rules:s",\$rulesfile,
           "xrs-file:s",\$rulesfile,
           "dumpallrules!",\$dumprules,
           "maxsents=i",\$maxsents,
           "prettyprintnbests!",\$pretty,
           "uglyprintnbests!",\$ugly,
           "onlycomparesame!",\$onlycomparesame,
           "corpusdir=s",\$corpusdir,
           "fieldname=s",\$fieldname,
           "getopl!",\$getopl,
           "epsilon=s",\$EPSILON,
           "topruleid=i",\$topruleid,
) or &usage;

my $anyorder=0;
unless ($fieldname eq 'totalcost') {
    $anyorder=$onlycomparesame=1;
}
my $justprint=$ugly || $pretty || $getopl;
&usage() if ((scalar @ARGV) < ($dumprules || $justprint ? 1: 2)); # die if too few args

&argvz;

my $reference;
$reference=shift unless scalar @ARGV == 1;

my $new='-';
$new=$ARGV[0] if defined $ARGV[0];

print STDERR "Only comparing first $maxsents sentences ...\n" if defined($maxsents);
print STDERR "Only comparing sentence #$sentence ...\n" if defined($sentence);

print STDERR "Restricting comparison to top $topn best per sentence ...\n" if defined($topn);

sub usage() {
    pod2usage(-exitstatus => 0, -verbose => 2);
    exit(1);
}

#my $capture_3brackets='{{{((?:[^}]|}[^}]|}}[^}])*)}}}';
my $capture_3brackets='{{{(.*?)}}}';
my $capture_natural='(\d+)';
my $capture_int='(-?\d+)';
my $capture_cost='((?:e\^)?-?\d+\S*)';
my $nsent;
my $lastsent;

my $nerror=0;
my $nwarn=0;

my %rules;
my %allrules;
my %nevent;

sub nbest_event {
    my ($type,$event,@rest)=@_;
    print "$type($event): ",@rest,"\n"  unless $quiet;
    ++$nevent{$event};
    count_info_gen("$type :$event");
}

sub nbest_error {
    ++$nerror;
    nbest_event("Error",@_);
}

sub nbest_warn {
    ++$nwarn;
    nbest_event("Warning",@_);
}

sub reset_parse_nbest {
    $nsent=0;
    $lastsent=undef;
}

my @sentids; # first entry undef, indexed 1...$nsent

sub parse_nbest {
    my ($line)=@_;
    if ($line =~ /^NBEST sent=$capture_natural nbest=$capture_natural/) {
        my $self={};
        my @k=qw(line sent nbest);
        $self->{line}=$_;
        $self->{sent}=$1;
        $self->{nbest}=$2;
        $self->{totalcost}=$1 if /totalcost=$capture_cost/;
        $self->{hyp}=$1 if /(?:hyp|estring)=$capture_3brackets/;
        $self->{deriv}=normalize_deriv($1) if /derivation=$capture_3brackets/;
        die "Couldn't find cost field $fieldname in $line" unless $line =~ /\b\Q$fieldname\E=$capture_cost/;
        $$self{$fieldname} = $1;
        my $realsent=$$self{sent};
        ++$nsent if (!defined $lastsent or $realsent != $lastsent);
        if (defined $maxsents) {
            if ($nsent > $maxsents) {
                --$nsent;
                return 'done';
            }
            return 'done' if (defined ($topn) && $nsent == $maxsents && $$self{nbest} > $topn);
        }
        $$self{sent}=$nsent if ($ignoresent);
        my $sentid=$$self{sent};
        my $oldsentid=$sentids[$nsent];
        $sentids[$nsent]=$sentid;
        $lastsent=$realsent;
        return 'done' unless  (!defined $sentence || $sentence==$sentid);
        if (defined $oldsentid && $sentid != $oldsentid) {
            die "Conflicting sent=$sentid compared to sent=$oldsentid - run with -ignoresent\n";
        }
#        &debug($nsent,$$self{sent},$ignoresent);
#        bless $self;
        return $self
    } else {
        nbest_error("unrecognized NBEST format", $line)   if ($line =~ /^NBEST/);
        return undef;
    }
    return undef;
}

sub string_nbest {
    my ($self,$forcebrief) = @_;
    my $localbrief = $forcebrief || $brief;
    return "[sent=$$self{sent} n=$$self{nbest} $fieldname=$$self{$fieldname}" .
      ($localbrief ? '' : " hyp=$$self{hyp} deriv=$$self{deriv}") .
        ']';
}

sub read_old {
    return defined ($_=<REF>);
}

sub read_new {
    return defined ($_=<>);
}

sub normalize_deriv {
    local($_)=@_;
    my $o=$_;
#    $d=~y/()/  /;
    #necessary to compare       veeryyy old nbest lists
    s/ +/ /g;
    s/^ //;
    s/ $//;
    s/\[\d+\,\d+\]//g;
    if (defined $topruleid) {
        s/^\($topruleid // && s/\)$//;
    }
    &debug("Normalize deriv:\n",$o,"\n",$_,"\n") if ($_ ne $o);
    return $_;
}

sub delta {
    my ($a,$b)=@_;
    abs($$a{$fieldname}-$$b{$fieldname});
}
sub same_cost {
    return &delta < $EPSILON;
}

sub better_cost {
    my ($a,$b)=@_;
    return $$a{$fieldname} + $EPSILON < $$b{$fieldname}
}

sub sort_by_cost {
    my ($l)=@_;
    @$l=map { $_->[0] }  # restore original values
	     sort { $a->[1] <=> $b->[1] }  # sort
	     map { [$_, $_->{$fieldname}] }   # transform: value, sortkey
         @$l;
}

sub corpusprelude {
    my ($sent)=@_;
    return unless defined $corpusdir;
    print "\n";
    my @fields=(['ref0','e.0'],['ref1','e.1'],['ats','ats-hyp']);
    for my $f (@fields) {
        my $file="$corpusdir/$$f[1]";
        &debug($sent,$$f[0],$$f[1],$file);
        if (-f $file) {
            my $line=nthline($sent,$file);
            print "$$f[0]: $line" if defined $line;
        }
    }
}

# writes to storebyderiv hash<normalize_deriv(deriv)> of (ref)nbest hash
sub get_nbests {
    my ($reader,$storebyderiv,$storenpersent)=@_;
    my $nowarn=defined $storenpersent;
    my $lastnbest=undef;
    my $nbests={};
    my $nn=0;
    my $thisn;
    &reset_parse_nbest;
    my %derivs;
    my $lastsent;
    my $lastpass;
    my     $newsent;
    while (&$reader) {
        my $passno=(s/^\(pass (\d+)\)\s*//) ? $1 : 0;
        if (defined(my $nbest=parse_nbest($_))) {
            last if $nbest eq 'done';
            ++$nn;
            my $sent=$nbest->{sent};
            next unless !defined $sentence or $sent == $sentence;
            if (!defined $lastpass || $lastpass != $passno) {
                $lastnbest=undef;
            }
            if (!defined $lastsent || $sent!=$lastsent) {
                $lastnbest=undef;
                $newsent=1;
            } else {
                $newsent=0;
            }
            $lastpass=$passno;
            if (defined $lastnbest) {
                if (!$anyorder && &better_cost($nbest,$lastnbest)) {
                    nbest_warn("out of order nbest","$fieldname improved from ",string_nbest($lastnbest)," to ",string_nbest($nbest)) unless $nowarn;
                }
                ++$thisn;
            } else {
                if (defined $lastsent) {
                    $$storenpersent[$lastsent]=( ((defined $topn) and $thisn > $topn) ? $topn : $thisn) if (!$passno && defined $lastsent and defined $storenpersent);
                }
                $thisn=1;
            }
            if ((defined $topn) and $thisn > $topn) {
            } else {
                if ($dumprules) {
                    my $rules=&normalize_deriv($$nbest{deriv});
                    my @rules=split ' ',$rules;
                    ++$allrules{$_} for @rules;
                }

                &debug($sent,$thisn,$topn);
                my $nderiv=normalize_deriv($$nbest{deriv});
                if (!$passno) {
                    if (exists $$storebyderiv{$nderiv}) {
                        nbest_error("derivation repeated in nbests",string_nbest($nbest)," - same deriv as ",string_nbest($$storebyderiv{$nderiv})) unless $nowarn;
                    }
                    $$storebyderiv{$nderiv}=$nbest;
                }
                if ($ugly) {
                    print $$nbest{line};
                }
                my $hyp=$$nbest{hyp};
                if ($pretty) {
                    my $line=$$nbest{line};
                    corpusprelude($$nbest{sent}) if $newsent;
                    print "#$$nbest{sent}";
                    print     " n=$$nbest{nbest}";
                    print   " pass=$passno" if $passno;
                    print " \"$hyp\" totalcost=$$nbest{totalcost}";
                    print " tm=$1" if ($line =~ /sbtm-cost=$capture_cost/ && $1 > 0);
                    print " sblm=$1" if ($line =~ /sblm-cost=$capture_cost/ && $1 > 0);
                    print " lm=$1" if ($line =~ /lm-cost=$capture_cost/ && $1 > 0);
                    print " len=$1" if ($line =~ /text-length=$capture_cost/);
                    print "\n";
                }
                if ($getopl && $$nbest{nbest} == 0 && !$passno) {
                    $hyp =~ s/\(\w+ \@UNKNOWN\@\)/ /g;
                    $hyp =~ s/ +/ /g;
                    print "$hyp\n";
                }
            }
            $lastsent=$sent;
            $lastnbest=$nbest;

        }
    }
    $$storenpersent[$lastsent]=(((defined $topn) and $thisn > $topn) ? $topn : $thisn) if (defined $lastsent and defined $storenpersent);
    return $nn;
}

sub push_multihash {
    my ($hash,$key,$val)=@_;
    if (exists $$hash{$key}) {
        my $lref=$$hash{$key};
        push @$lref,$val;
    } else {
        $$hash{$key}=[$val];
    }
}

sub debug_nbest {
    my ($nbest)=@_;
    return $$nbest{$fieldname};
}

sub check_nbests {
    my ($worse,$better,$betterns)=@_;
    my @wdiff;
    my @bdiff;
    my @wdiffs;
    my @bdiffs;
    my $top_rank_worsened=999999999;
    for (1..$nsent) {
        my $sentid=$sentids[$_];
        $bdiffs[$sentid]=[];
        $wdiffs[$sentid]=[];
    }
    my $nsame=0;
    my ($nref,$sumref,$nnew,$sumnew)=(0,0,0,0);
    while (my ($deriv,$nbest) = each %$worse) {
        ++$nref;
        $sumref += $$nbest{$fieldname};
        if (exists $$better{$deriv}) {
            my $bnbest=$$better{$deriv};
            &debug("same",$deriv,string_nbest($nbest));
            ++$nsame;
            my $delta=delta($nbest,$bnbest);
            nbest_error("changed $fieldname [delta=$delta] for derivation","new $fieldname: ",string_nbest($bnbest,1),", reference: ",string_nbest($nbest)," deriv=[$$bnbest{deriv}]") unless same_cost($nbest,$bnbest);
        } else {
 #           push @wdiff,$nbest;
            push @{$wdiffs[$$nbest{sent}]},$nbest;
            &debug(@{$wdiffs[$$nbest{sent}]});
            my $rank=$$nbest{nbest};
            $top_rank_worsened = $rank if ($rank < $top_rank_worsened); 
        }
    }
    print "$nsame identical nbest entries total.\n" unless $quiet;
    while (my ($deriv,$nbest) = each %$better) {
        ++$nnew;
        $sumnew += $$nbest{$fieldname};
        unless (exists $$worse{$deriv}) {
#            push @bdiff,$nbest;
            push @{$bdiffs[$$nbest{sent}]},$nbest;
#            &debug($$nbest{sent},@{$bdiffs[$$nbest{sent}]});
        }
    }
#    sort_by_cost(\@wdiff);
#    sort_by_cost(\@bdiff);
    
    my ($totalworse,$totalbetter,$totalindet)=(0,0,0);
    for (1..$nsent) {
        my $sentid=$sentids[$_];
        die unless defined $sentid;
        @wdiff=();
        @bdiff=();
        @wdiff=@{$wdiffs[$sentid]} if defined $wdiffs[$sentid];
        @bdiff=@{$bdiffs[$sentid]} if defined $bdiffs[$sentid];
        sort_by_cost(\@wdiff);
        sort_by_cost(\@bdiff);
        &debug("REF",$_,$sentid,map {debug_nbest($_)} @wdiff);
        &debug("\n\n\n");
        &debug("NEW",$_,$sentid,map {debug_nbest($_)} @bdiff);
        my ($bnd,$wnd)=(scalar @bdiff,scalar @wdiff);
        if ($bnd < $wnd) {
            nbest_warn("incomplete nbest list","new nbest list has fewer entries than reference ($bnd different in new, $wnd different in reference)");
        }
        my $i=0;
        my $nbetter=0;
        my $nworse=0;
        my $nindet=0;
        for my $b (@bdiff) {
            my $w=$wdiff[$i];
            last unless defined $w;
#            &debug("diff",string_nbest($b),string_nbest($w));
            if (better_cost($b,$w)) {
                nbest_warn("new nbest better","\n NEW=",string_nbest($b),"\n REF=",string_nbest($w)) if ($allchanges);
                ++$nbetter;
            } elsif (better_cost($w,$b)) {
                nbest_warn("new nbest worse","\n NEW=",string_nbest($b), "\n REF=",string_nbest($w),"\n - reference deriv={{{$$w{deriv}}}}") unless $onlycomparesame;
#, "; new:\nderiv={{{$$b{deriv}}}}");
                my $rules=normalize_deriv($$w{deriv});
                my @rules=split ' ',$rules;
#                &debug(@rules);
                ++$rules{$_} for @rules;
                ++$nworse;
            } else {
                nbest_warn("indeterminate change","\n NEW=",string_nbest($b),"\n REF=",string_nbest($w)) if ($allchanges);
                ++$nindet;
            }
            ++$i;
        }
        $totalworse+=$nworse;
        $totalindet+=$nindet;
        $totalbetter+=$nbetter;
        unless ($quiet) {
        print "\nFor sentence $sentid";
#        print "($$betterns[$sentid] nbests)" if defined $betterns;
        print ": $nworse worse, $nbetter better, $nindet indeterminate";
        print ", ",$$betterns[$sentid]-($bnd)," identical" if defined $betterns;
        print " (indeterminate means equal within epsilon=$EPSILON).\n";
    }
    }
    &print_used_rules;
    print "\n" unless $quiet;
    print "Total: $nsame identical, $totalworse worse, $totalbetter better, $totalindet indeterminate (epsilon=$EPSILON).\n";
    print " (all of the top ",$top_rank_worsened," stayed the same or got cheaper)\n" if ($totalworse);
    print " ($nref total ref nbests (from $reference) with avg. $fieldname of ",$sumref/$nref,")\n" if $nref;
    print " ($nnew total new nbests (from $new) with avg. $fieldname of ",$sumnew/$nnew,")\n" if $nnew;
    print "\n" unless $quiet;
    print "$nerror errors and $nwarn warnings:\n";
    while(my ($type,$cnt)=each %nevent) {
        print "\t$cnt \"$type\"\n";
    }
    &all_summary;
}


sub print_used_rules {
    if (defined($rulesfile)) {
        print "\nRules used by missing (better) reference derivations:\n";
        my $fh=openz($rulesfile);
        while (<$fh>) {
            if (/id=(\d+)/) {
                if (exists $rules{$1}) {
                    print;
                }
                if ($dumprules && exists $allrules{$1}) {
                    print "::XRS-RULE(xrs-line:$.): $_";
                }

            }
        }
        close $fh;
    }

}
#&debug(%new_nbests);
&debug('');
&debug('reading old ...');
my %old_nbests;
open REF, $reference or die $! if defined $reference;
my $nold=get_nbests(\&read_old,\%old_nbests) if (defined $reference);
my %new_nbests;
my @npersent;
my $nnew=get_nbests(\&read_new,\%new_nbests,\@npersent);
&print_used_rules unless defined $reference;

check_nbests(\%old_nbests,\%new_nbests,\@npersent) if (defined $reference);

__END__

=head1 NAME

  diffnbest.pl

=head1 SYNOPSIS

  diffnbest.pl [options] [reference_nbest] new_nbest

=head1 OPTIONS

=over 8

=item B<-brief>

  show only sent, nbest and cost for differences.

=item B<-quiet>

  only show summary (no errors or warnings)

=item B<-n N>

  compare only top N (even if more are found)

=item B<-maxsents M>

  compare only the first M sentences (even if more are found)

=item B<-rulesfile filename>

  for missing derivations, print the used rules (may be a .gz)

=item B<-sentence N> (default 1)

  only compare for NBEST sent=N

=item B<-ignoresent>

  ignore sentence ids, assuming the sentences in reference and new are in the
  same order but with different ids

=item B<-allchanges>

  even if the differences are all improvements, show them.

=item B<-onlycomparesame>

  don't examine different derivations

=item B<-dumpallrules>

  print ::XRS_RULE ... items for all used rules (in the top -n)

=item B<-prettyprintnbests>

  print nbests (prettily)

=item B<-uglyprintnbests>

  print nbests (ugly, same format as input)

=item B<-getopl>

  print opl (one per line) first best hypothesis only, with UNKNOWN words
  removed, for BLEU scoring

=item B<-corpusdir>

  directory to find e.0, e.1, ats.hyp files in

=item B<-topruleid>

  any derivation with topruleid at the root will have that rule stripped off

=item B<-fieldname name>

  compare name=110 as cost (default name=totalcost)

    =item B<-epsilon <epsilon>> absolute cost difference within which costs are
considered equal

=back

=head1 DESCRIPTION

  Verifies that a new nbest list is an improvement (or identical to) the reference.

  Looks at differences in derivations and scores between a file and old files
  ... assumes sent=N are comparable unless -ignoresent.  For those derivations
  that are in both, the scores must match, or else the difference is shown.  For
  those derivations that are new, compare their scores in order, and if they're
  all improvements, nothing is wrong, otherwise (if the old ones are better for
  any at the same rank) show the difference.

  Note that this will mask some missing entries if there are enough improvements
  at the top of the list.

  Also detects out of order nbest lists.

  If you happen to have the same derivation as a translation for different
  sentences, it's assumed to be an error for now.

  Supplying only one nbest file will allow options like --dumpallrules and
  --prettyprintnbests to operate.

=cut
