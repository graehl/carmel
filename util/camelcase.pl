#!/usr/bin/perl -i~ -p -w
use strict;
sub ucfirst {
    local($_)=@_;
    uc(substr($_,0,1)).substr($_, 1);
}
my @whitelistpre = (
    'EVP_',
    'zhash_',
    'cson_',
    );
my @whitelistpost = (
    '_t',
    '_type',
    );
my @whitelist = (
    'unordered_map',
    'const_iterator',
    'size_t',
    'time_t',
    );
my %whitelistset;
$whitelistset{$_}=1 for (@whitelist);
my %transform = (
    'hstore_query' => 'HstoreQuery',
    'hstore_query_fn' => 'HstoreQueryFn',
    'bytes_builder' => 'BytesBuilder',
    );
my $UNDERSCORE='<([UNDER_S_CORE])>';
sub whitelist {
    local($_)=@_;
    &debug("whitelist?('$_')");
    my $out;
    $out = $_ if exists $whitelistset{$_};
    $out = $transform{$_} if exists $transform{$_};
    $out = $_ unless /[a-z]/;
    for my $pre (@whitelistpre) {
        $out = $_ if /^\Q$pre\E/;
    }
    for my $post (@whitelistpost) {
        $out = $_ if /\Q$post\E$/;
    }
    if (defined($out)) {
        $out =~ s/_/$UNDERSCORE/g;
        &debug("whitelist?('$_')=>$out");
    }
    defined($out) ? $out : $_;
}
sub debug {
    print STDERR 'DBG: ',join(' ',@_), '\n' if $ENV{DEBUG};
}
if (/^\s*\#/) {
    &debug("preprocessor directive: $_");
} else {
    s/\buint(64|32|16|8)_t\b/UI$1/g;
    s/\bint(64|32|16|8)_t\b/I$1/g;
    s/([a-zA-Z][[a-zA-Z0-9]*_[a-zA-Z0-9_]*)/&whitelist($1)/eg;
    &debug("$1_$2(") if s/([a-z][a-zA-Z0-9]*)_([a-z0-9_]*[a-z][a-z0-9_]*)\(/&ucfirst($1).&ucfirst($2).'('/e;
    while (s/([a-zA-Z][a-zA-Z0-9]*)_([a-zA-Z0-9_]*[a-z][a-zA-Z0-9_]*)/$1 . &ucfirst($2)/e) {
        &debug("$1_$2");
    }
    s/\Q$UNDERSCORE\E/_/g;
    s/\bhrev\b/Hrev/g;
    s/\bhstore\b/Hstore/g;
    s/\bbytes\b/Bytes/g;
}
