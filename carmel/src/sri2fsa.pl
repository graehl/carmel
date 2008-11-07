#!/usr/bin/perl -w


# translate input sri text language model to carmel wfsa

# example usage:  

# EOS=0 ./sri2fsa.pl ../sample/3gram.sri | carmel -gIs 10
# EOS=1 ./sri2fsa.pl ../sample/3gram.sri | carmel -gIs 10

#The so-called ARPA (or Doug Paul) format for N-gram backoff models starts with a header, introduced by the keyword \data\, listing the number of N-grams of each length. Following that, N-grams are listed one per line, grouped into sections by length, each section starting with the keyword \N-grams:, where  N is the length of the N-grams to follow. Each N-gram line starts with the logarithm (base 10) of conditional probability  p of that N-gram, followed by the words w1...wN making up the N-gram. These are optionally followed by the logarithm (base 10) of the backoff weight for the N-gram. The keyword \end\ concludes the model representation.

#Backoff weights are required only for those N-grams that form a prefix of longer N-grams in the model. The highest-order N-grams in particular will not need backoff weights (they would be useless).

#Since log(0) (minus infinity) has no portable representation, such values are mapped to a large negative number. However, the designated dummy value (-99 in SRILM) is interpreted as log(0) when read back from file into memory.

use strict;

## OPTIONS:

my $eos=$ENV{EOS}; # </s> vs. *e* at end
my $lock_bo=$ENV{LOCK_BACKOFF};

## END OPTIONS

my $bo_suffix=$lock_bo ? '!' : '';

my $loop=$ENV{LOOP}; #TODO: probability of another sentence would need to be given or
                     # else inconsistent ... wfsa would accept a sequence of sentences

my $eos_word="</s>";
my $eos_state=$eos_word; # replace dest w/ this carmel state, if last word is
                         # $eos_word 
my $carmel_eos_word=$eos ? '"</s>"' : "*e*" ;

my $sos_word="<s>";
my $sos_state=$sos_word;

my $no_context_state='""';

my $start_state=$sos_state; # could start in $no_context_state instead
my $final_state=$eos_state;



# X = quote and spec and esc are all to be c-string-style escaped by esc X

# use for all single words as tokens in FSA
sub escape_for_carmel
{
    my ($s)=@_;
    $s =~ s/([\"])/\$1/og;
    return qq{"$s"};
}


sub escape_state
{
    my ($s)=@_;
    if ($s eq '' || $s =~ /^["*]/o || $s =~ /[ ()]/o) {
        return escape_for_carmel($s);
    }
    return $s;
}


# sequence of a_b_c ... if b = e.g. ~a_, then replace b with ~~a~_ (i.e. ~ is
# like \ in c-escapes
sub escape_for_seq {
    my ($s)=@_;
    $s =~ s/([~_])/~$1/og;
    return $s;
}

sub escaped_to_state
{
    return escape_state(join('_',@_));
}

sub words_to_state {
    escaped_to_state(map(escape_for_seq($_),@_));
}



sub ngram_to_fsa_arc {
    my ($p,$bo,@words)=@_;
    my $last_word=$words[$#words];
    my $word_sym=escape_for_carmel($last_word);
    my @escs=map(escape_for_seq($_),@words);
    my $whole=escaped_to_state(@escs);
    my $laste=pop @escs;
    my $source=escaped_to_state(@escs); # source must exist and be reachable
                                         # since it's a prefix of us and we
                                         # exist, so it must have had a BO
    push @escs,$laste;
    shift @escs;
    my $bostate=escaped_to_state(@escs);
    my $dest;
    if ($last_word eq $eos_word) {
        $dest=$eos_state;
        $word_sym=$carmel_eos_word;
    } elsif (defined($bo)) {
        $dest=$whole;
        print "($dest $bostate 10^$bo$bo_suffix)\n";
    } else {
        $dest=$bostate;
    }
    

    print "($source $dest $word_sym 10^$p)\n";
}


sub read_srilm {
    my ($process_ngram)=@_;
    local $_;
    my $N;
    my $maxN;
    while(<>) {
        if (/^\\(\d+)-grams:$/) {
            $N=$1;
#            &debug("expecting $N-grams: $. $_");
            print "\n";
        } elsif (defined($N)) {
            @_=split;
            next unless scalar @_ > $N;
            my $p=shift;
            my $bo=pop if scalar @_ > $N;
            $process_ngram->($p,$bo,@_)
        }
    }
}

print qq{$final_state\n($start_state)\n};

read_srilm(\&ngram_to_fsa_arc);

exit;

