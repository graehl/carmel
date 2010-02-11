#!/usr/bin/env perl
# input: sorted (ngram_count -sort) srilm cross language ngram
#  that has two types of tokens: foreign which end in F suffix, english ending in E suffix.
#  (except for SRI-introduced <s>,</s>,<unk>)
# output: srilm with fewer ngram events than original (header is updated for new number of N-grams as required by quantize.py and pagh/biglm training)
# probabilities are always p(f|e-context), so I expect duplicate entries in \N-gram: section
# -99 ctxE wordE -1.3
# -2.4 ctxE wordF
# note: wordF may not occur if the foreign text had no instances of the english word in it
# whenever those consecutive pairs occur, they're collapsed to:
# -2.4 ctx word -1.3
# all tokens have the E and F suffixes stripped.  (ctx may be 0 or more words)
#
# recent bugfix:
# -99 ctxE wordE -1.3
# -99 ctx2E word2E -1.3
# needs to output both of these
use warnings;
use File::Temp;
my $TEMP=$ENV{TEMP};
$TEMP='.' unless $TEMP;
my ($fh, $tmpname) = File::Temp::tempfile("$TEMP/stripEF.srilm.XXXXXX");
print STDERR "using temporary $tmpname\n";

my ($bo,$boline,$N1)=("","",0);
my $N=0;
my @ngrams=(0);

sub print1 {
    ++$ngrams[$N];
    print $fh @_;
}

while(<>) {
    if (/^\\(\d+)-grams:\s*$/o) {
        $N=$1;
        push @ngrams,0;
        print $fh $_;
        print STDERR "starting $N-grams...\n";
    } elsif ($N==0) {
    } elsif (/(\S+)\s+(.*?)(\S+)(F|E(\s+[-\d.e]+)?)\s*$/o) {
        my ($p,$ctx,$w,$fe,$ebo)=($1,$2,$3,$4,$5);
        $ctx =~ s/(\S+)[EF](\s+)/$1$2/go;
        $ev="$ctx$w";
        if ($p==-99) {
            die "F word $w w/ 0 prob: $_" if $fe eq "F";
            $bo=defined $ebo ? $ebo : "";
            print1($boline) if ($boline);
            $boline="$p\t$ev$bo\n";
            $lastev=$ev;
        } else {
            if ($boline && $lastev ne $ev) {
                print1($boline);
                $bo="";
            }
            print1("$p\t$ev$bo\n");
            $boline=$bo="";
        }
    } elsif (/^(.*<(\/s|s|unk)>)$/o) {
        print1($_);
    } else {
        die "unknown ngram line: $_" unless /^$/o || /^\\(\d+-grams:|end)\\$/o;
        print $fh $_;
    }
}
print1($boline) if ($boline);

print "\n\\data\\\n";
for (1..$#ngrams) {
    print "ngram $_=$ngrams[$_]\n";
}
print "\n";

close $fh;

system "cat",$tmpname;

unlink $tmpname;
