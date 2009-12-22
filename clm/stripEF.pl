#!/usr/bin/env perl
# input: sorted (ngram_count -sort) srilm cross language ngram
#  that has two types of tokens: foreign which end in F suffix, english ending in E suffix.
#  (except for SRI-introduced <s>,</s>,<unk>)
# output: srilm with fewer ngram events than original (header isn't updated so you should re-count if that matters)
# probabilities are always p(f|e-context), so I expect duplicate entries in \N-gram: section
# -99 ctxE wordE -1.3
# -2.4 ctxE wordF
# note: wordF may not occur if the foreign text had no instances of the english word in it
# whenever those consecutive pairs occur, they're collapsed to:
# -2.4 ctx word -1.3
# all tokens have the E and F suffixes stripped.  (ctx may be 0 or more words)

use warnings;

my ($bo,$boline,$N1)=("","",0);
sub print1 {
    ++$N1;
    print @_;
}
while(<>) {
    print;
    if (/^\\1-grams:$/o) {
        while(<>) {
            if (/(\S+)\s+(.*?)(\S+)(F|E(\s+[-\d.e]+)?)\s*$/o) {
                my ($p,$ctx,$w,$fe,$ebo)=($1,$2,$3,$4,$5);
                $ctx =~ s/(\S+)[EF](\s+)/$1$2/go;
                $ev="$ctx$w";
                if ($p==-99) {
                    die "F word $w w/ 0 prob: $_" if $fe eq "F";
                    $bo=defined $ebo ? $ebo : "";
                    $boline="$p\t$ev$bo\n";
                    $lastev=$ev;
                } else {
                    if ($boline && $lastev ne $ev) {
                        print($boline);
                        $bo="";
                    }
                    print("$p\t$ev$bo\n");
                    $boline=$bo="";
                }
            } else {
                die "unknown ngram line: $_" unless (/^(.*<(\/s|s|unk)>)?$/ || /^\\(\d+-grams:)|end\\$/);
                print($_);
            }
        }
        last;
    }
}
