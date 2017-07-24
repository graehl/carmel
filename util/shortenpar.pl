#!/usr/bin/perl -w
use strict;
use Getopt::Long;
use List::Util qw(sum);
use POSIX;
sub openz {
    my ($file)=@_;
    my $fh;
    die "no input file specified" unless $file ne '';
    if ($file =~ /\.gz$/) {
        open $fh,"-|","gunzip","-f","-c",$file or die "gunzip -c $file: $!";
    } elsif ( $file =~ /\.bz2$/) {
        open $fh,"-|","bunzip2","-c",$file or die "bunzip2 -c $file: $!";
    } else {
        open $fh,'<',$file or die "read $file: $!";
    }
    $fh;
}
sub openz_out {
    my ($file)=@_;
    return \*STDOUT if ($file eq '-');
    return \*STDERR if ($file eq '-2');
    my $fh;
    die "no input file specified" unless $file ne '';
    open $fh,'>',$file or die "read $file: $!";
    $fh;
}
my $minlen = 0;
my $HUGE = 1e20;
my $maxlen = $HUGE;
my $lr = 0;
my $ur = 0;
my $plus = 10;
my $chars = 0;
my $outext;
my $pkeep = 1;
my $pevery = -1;
my $rejects;
my $histogram;
my $verbose = 1;
my $all;
GetOptions("min:i" => \$minlen,
           "max:i" => \$maxlen,
           "p:s" => \$pkeep,
           "pevery:s" => \$pevery,
           "lr:s" => \$lr,
           "ur:s" => \$ur,
           "plus:i" => \$plus,
           "outext:s" => \$outext,
           "chars" => \$chars,
           "rejects:s" => \$rejects,
           "histogram" => \$histogram,
           "verbose:i" => \$verbose,
           "all" => \$all,
    ) || die;
$all = $all || $lr > $ur;
$ur = $ur ? $ur : $lr > 0 && $lr < 1 ? 1/$lr : $HUGE;

my @f = map {&openz($_) }@ARGV;
my @o = map {my $fh;
             open $fh,">","$_.$outext" or die "open for output: $_.$outext: $!";
             $fh } @ARGV if defined($outext);
sub max {
    my $max=shift;
    $max < $_ and $max = $_ for @_;
    return $max;
}
sub min {
    my $min=shift;
    $min > $_ and $min = $_ for @_;
    return $min;
}
my $n = 0;
$pkeep = 1/$pevery if $pevery > 0;
$pkeep = 1 if $pkeep < 0;
my $rfile = openz_out($rejects) if $rejects;

my $hu = $ur;
my $hl = $lr;
if ($hl >= $hu) {
    $hu = 2.5;
    $hl = 0;
}
my $hsize = $hu - $hl;
my $nbins = 50;
my $binsize = $hsize / $nbins;
my @bins = (0) x ($nbins + 2);
my $numlen = 6;
my $numformat = "%.".($numlen-2)."f";
my $padinf = ' ' x ($numlen - 4);
my $margin = 80;
$margin -= $numlen;
sub histout {
    my $sum = sum(@bins);
    my $max = max(@bins);
    return unless $max > 0;
    for (0..$nbins+1) {
        if (!$_) {
            print '-INF', $padinf;
        } elsif ($_ > $nbins) {
            print '+INF', $padinf;
        } else {
            print sprintf($numformat, $lr + $binsize * ($_ - 1));
        }
        my $morelen = max(0, int($margin * $bins[$_] / $max));
        print '=' x $morelen, "\n";
    }
}
sub histrecord {
    my ($x) = @_;
    ++$bins[min($#bins, max(0, 1 + floor(($x - $hl) / $binsize)))];
}
for(;;) {
    ++$n;
    my @l = map { scalar <$_> } @f;
    last if !defined($l[0]);
    for (@l) { die unless defined $_; }
    die unless scalar @l;
    my @lens = $chars ? map { use utf8; max(0,length($_) - 1) } @l : map { scalar(split) } @l;
    my $l0 = max(.00001, $lens[0] + $plus);
    # (b + p) / (a + p) > l
    # (b + p) > (a + p) * l
    # b > (a + p) * l - p
    use POSIX;
    my $l = floor($l0 * $lr - $plus);
    my $u = ceil($l0 * $ur - $plus);
    my $len = max(@lens);
    my $mlen = min(@lens);
    if ($histogram) {
        histrecord(($lens[$_] + $plus) / $l0) for (1..$#lens);
    }
    if (($all || List::Util::all { $l <= $_ && $_ <= $u } @lens) && $len <= $maxlen && $mlen >= $minlen) {
        next unless rand() < $pkeep;
        for (0..$#o) {
            my $fh = $o[$_];
            print $fh $l[$_];
        }
    } else {
        print STDERR "#$n\t$lens[0]\t",$all ? "" : "[$l,$u]\t",join " ",@lens,"\n" if $verbose;
        if ($rejects) {
            print $rfile "$n\n";
            print $rfile $_ for (@l);
        }
    }
}
histout() if ($histogram);
