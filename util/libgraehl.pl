#!/usr/bin/env perl
###USAGE: begin your perl script with:

# use warnings;
# use strict;
# use Getopt::Long;
# use File::Basename;
# my $scriptdir; # location of script
# my $scriptname; # filename of script
# my $BLOBS;
# BEGIN {
#    $scriptdir = &File::Basename::dirname($0);
#    ($scriptname) = &File::Basename::fileparse($0);
#    push @INC, $scriptdir;
#     $ENV{BLOBS}='/home/nlg-01/blobs' unless exists $ENV{BLOBS};
#     $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
#     $BLOBS=$ENV{BLOBS};
#     my $libgraehl="$BLOBS/libgraehl/v2";
#     push @INC,$libgraehl if -d $libgraehl;
# }
# require "libgraehl.pl";



my @ORIG_ARGV=@ARGV;

#note: perl 5.12 (5.10?) has a nice module IPC::Cmd standard, which does what
#exec_filter etc. do

# $fh=openz@($filename)
# &argvz; <>;
# exec_filter('cat',"a\nb") eq "a\nb"
# &cleanup would kill the current exec_filter process - naturally this assumes only one runs at a time per process (the global filehandles FW/FR require this as well)

use warnings;
use strict;

use Carp;

sub printw {
    print join(' ',@_),"\n";
}
sub printj {
    print join(' ',@_);
}

sub warn_summary {
    &all_summary();
    &Carp::cluck
}

sub die_summary {
    &all_summary();
    &Carp::confess
}

sub set_die_warn {
    $SIG{__DIE__}=$_[0];
    &set_system_warn_fn($SIG{__WARN__}=$_[1]);
}

sub use_carp_nosummary {
    set_die_warn(\&Carp::confess,\&Carp::cluck)
}

sub use_carp {
    set_die_warn(\&die_summary,\&warn_summary);
}

use File::Temp qw/ tempfile tempdir/;

my $DEBUG=$ENV{DEBUG};

# args: list of fns. return [list of fhs]
sub openzs {
    [map { &openz($_) } @_]
}

# takes [list of fhs], returns (next line from each), or () if all files are
# done. optional arg dies if any file is done before the others are
sub reads {
    my ($fhs,$die)=@_;
    my $ndone=0;
    local $_;
    my @r;
    for (@$fhs) {
        my $l=<$_>;
        push @r,$l;
        if (!defined($l)) {
            ++$ndone;
        }
    }
    return () if ($ndone==scalar @$fhs);
    croak "files had different number of lines (one but not all ended early)" if ($die && $ndone>0);
    @r
}

# to be evaled, because PadWalker module isn't available: http://stackoverflow.com/questions/3729734/how-can-i-print-the-name-of-a-given-perl-variable-not-its-value
sub vars {
    q{
    my @varvals;
    for my $v (@vars) {
        push @varvals,$v,eval "\$$v";
    }
@varvals
     }
}
sub opthash2vars {
    #in caller, eval &opthash2vars; you have a "@vars" and "$opthash={...}" there.
    q{
    for my $v (@vars) {
        #no strict 'refs';
        if (exists $opthash->{$v}) {
           #$$v=$opthash->{$v}; info(qq{$v=$opthash->{$v}});
eval "\\$$v=\$opthash->{$v}; ";
#info(qq{$v=$opthash->{$v}});
       }
    }
}
}
sub debug_vars {
    #USAGE: eval &debug_vars;

    my $varscode=&vars;

#   return $DEBUG ? '()  ' :   $varscode;

q{
my $r=[];
    if ($DEBUG)
    {
        for my $varname (@vars)
        {
            push @$r,$varname,eval "\$$varname";
        }
    }
    @$r
}
}

sub tex2pdf {
    #tex = file.tex or file (.tex added) - creates file.ps and file.pdf - note: outdir may cause failures if .ps includes .eps relative to .tex dir; symlink the required files to outdir first.
    #xetex=1 -> use xelatex which goes straight to pdf already. note: landscape only applies to regular latex dvi2ps stage and you should specify page type in your .tex anyway. if 'latex' bin is specified, it must point to xelatex; give latexbin dir and it will use latexbin/xelatex.
    # although xetex (and maybe latex) has the possibility of putting work files in cwd that isn't input file, we intentionally cd to input file so relative includes etc. work. so outdir specified -> move pdf there after. xetex has --aux-directory=dir and --output-directory=dir. regular latex hasn't. but dvips can have a target in outdir.
    # set all the options in $opthash->{name} instead of in long positional arg list:
    my ($tex,$opthash,$landscape,$nobatchmode,$latn,$cplatex,$nodelete,$latexbin,$latex,$outdir,$pdfonly,$xetex,$warn,$quiet,$rmoldpdf)=@_;
    local $_;
    my @vars=qw(tex outdir landscape nobatchmode latn cplatex nodelete latexbin latex pdfonly xetex warn quiet rmoldpdf);
    my $eopt=&opthash2vars;
    #    &debug($eopt);
    eval $eopt;

    my $x=$DEBUG;

#    unless ($quiet) {
#    &debug($edv);
        my $edv=&debug_vars;
        my @vals;
        @vals=  eval $edv;
        &debug( \@vals);
#    }

    my $latoutdir='';
    if ($outdir)
    {
        $outdir=abspath($outdir);
        mkdir_check($outdir);
        $latoutdir="-output-directory=$outdir";
        $outdir.='/';
    } else
    {
        $outdir='';
    }
    $latn=2 unless $latn;
    $latexbin='/home/nlg-03/mt-apps/texlive/2010/bin/x86_64-linux' unless $latexbin && -d $latexbin;
    $latexbin='/usr/bin' unless $latexbin && -d $latexbin;
    $latex=&which_dir($xetex?'xelatex':'latex',$latexbin,$quiet) unless $latex && -x $latex;
    my $dvips=&which_dir('dvips',$latexbin,$quiet); #'/usr/bin'
    my $ps2pdf=&which_dir('ps2pdf','/usr/bin',$quiet);
    $cplatex=undef unless $cplatex && -f $cplatex;
    require_file($tex);
    $tex =~ s/\.tex$//;
    my $papertype=$landscape?'-t landscape':'';
    my ($texdir,$texf)=dir_file_noslash($tex);
    my $cp=$cplatex?"cp $cplatex .\n":"";
    my $outpdf="$outdir$texf.pdf";
    my $rmpdf=$rmoldpdf ? "rm -f '$outpdf'\n":'';
    my $rm=($cplatex&&!$nodelete)?"rm -f $cplatex\n":"";
    my $input="'".($nobatchmode?'':'\batchmode\input ').$texf.".tex'";
    &debug($tex,$input);
    my $latexinputs = "$latex $latoutdir $input \n" x $latn;
    my $texfrm="rm -f '$texf'.{aux,dvi,bbl,blg,log,out,toc}\n";
    my $texfrm2=$nodelete?'':$texfrm;
    my $allauxrm = $pdfonly ? "rm -f *.aux\n" : ''; # because files like
                                                          # pst-qtree.may be
                                                          # created also
    my $mvout=$outdir?"mv '$texf.pdf' '$outpdf'\n":'';
    my $psrm = $pdfonly ? "rm -f '$texf'.ps\n" : '';
    $psrm='' if $xetex;
    my $pdfcmds=$xetex?'':qq{
$dvips $papertype $texf
$ps2pdf $texf.ps $texf.pdf
};
    &debug($texdir,$texfrm,$rmpdf,$cp,$latexinputs);
    my $script=qq{
set -e
set -x
cd '$texdir'
$texfrm$rmpdf$cp$latexinputs$pdfcmds$mvout$rm$texfrm2$allauxrm$psrm};
#    $warn ? system_warn($script) : system_force($script);
    system_opt({warn=>$warn,quiet=>$quiet,stdout=>$opthash->{stdout},stderr=>$opthash->{stderr}},"($script)"); # group shell commands in subshell so redir applies to all
}

my $escapechar=',';
my $is_bad_file=qr.[$escapechar/ \t\n\\><|&;"'`~*?{}$!()].;
my %bad2name=(
    "\0"=>'0',
    $escapechar=>'e',
    '/'=>'s',
    "\t"=>'t',
    "\n"=>'n',
    "\\"=>'b',
    '>'=>'g',
    '<'=>'l',
    '|'=>'i',
    '&'=>'a',
    ';'=>'S',
    '"'=>'Q',
    "'"=>'q',
    '`'=>'B',
    '~'=>'T',
    '*'=>'A',
    '?'=>'m',
    '{'=>'L',
    '}'=>'R',
    '$'=>'D',
    '!'=>'C',
    '('=>'p',
    ')'=>'P',
    );
my %name2bad;
for (keys %bad2name) {
    croak "duplicate: $name2bad{$_}" if exists $name2bad{$_};
    $name2bad{$bad2name{$_}}=$_;
}
sub escapepath {
    local ($_)=@_;
    s/^\./_./; # avoid producing . or ..
    s/($is_bad_file)/$escapechar.$bad2name{$1}/goe;
    $_
}
sub unescapepath {
    local ($_)=@_;
    s/^_\././;
    s/$escapechar(.)/$name2bad{$1}/goe;
    $_
}
sub pad0 {
    my ($key,$pad)=@_;
    $pad ? sprintf("%0${pad}s",$key) : $key
}

sub key2path {
# returns ("a/b/c/d/e","../../../../.." (inverse path),a,b,c,d,e) where all but the last component has $every chars. empty key returns empty string. escapepath on $key unless $noescape (i.e. no "." "/" etc). left-0pad to $pad chars optionally (after escape)
    my ($key,$every,$pad,$noescape)=@_;
    return "" unless $key;
    $key=escapepath($key) unless $noescape;
    $key=pad0($key,$pad);
    $every=2 unless $every;
#    my @vars=qw(key every pad noescape);
#    my $ev=&debug_vars;my $x=$DEBUG;eval $ev;
    &debug("key=$key every=$every pad=$pad noescape=$noescape");


    my $n=length $key;
    my @r;
    my @v;
    for(my $i=0;$i<$n;$i+=$every) {
        &debug(substr($key,$i,$every));
        push @r,substr($key,$i,$every);
        push @v,'..';
    }
    (join('/',@r), join('/',@v),@r);
}
sub mkdir_isnew {
    my ($d)=@_;
    return 0 if -d $d;
    &mkdir_force($d);
    1
}


use DB_File;
    use Fcntl;
sub make_db_hash {
    my ($file,$hash)=@_;
    tie %{$hash},'DB_File',$file,O_RDWR|O_CREAT,0664,$DB_HASH || croak "creating DB_File $file";
}

sub count_ws {
    local ($_)=@_;
    my $n=0;
    ++$n while (/\s+/og);
    $n;
}
#fully qualified domain name
sub fqdn {
    superchomped(`hostname -f`)
}

#hpc-login-1 and compute nodes aren't accessible from outside; use hpc-login2 for them
sub fqdn_hpc {
    my $hostname=superchomped(`hostname`);
    ($hostname =~ /^hpc[^ .]*$/) ? "hpc-login2.usc.edu" : &fqdn;
}

sub ping_process {
    kill 0, $_[0];
}

sub become_daemon {
    my ($outfile)=@_;
    my $nullf='/dev/null';
    $outfile=$nullf unless $outfile;
    exit if fork();
    croak "can't detach" if POSIX::setsid()<0;
    local $SIG{HUP}='IGNORE';
    my $pid=fork();
    if ($pid) {
        wait;
        exit;
    } else {
        my $mfh=POSIX::sysconf( &POSIX::_SC_OPEN_MAX );
        $mfh=64 unless $mfh && $mfh>0;
        foreach ( 0 .. $mfh ) { POSIX::close( $_ ) }
        open STDIN,"+>$outfile";
        open STDOUT,"+>&STDIN";
        open STDERR,"+>&STDIN";
    }
}

sub system_output {
    my ($cmd)=@_;
    &debug($cmd);
    (ref $cmd eq 'ARRAY') ? open(SYSOUTP,'-|',@{$cmd}) : open(SYSOUTP,$cmd." |") || croak "system $cmd failed";
    my $out=do { local $/; <SYSOUTP> };
    close SYSOUTP;
    ($?>>8,$out)
}

sub optarg {
    my ($k,$v)=@_;
    &debug("optarg",$k,$v);
    defined($v) ? ($k,$v) : ()
}
sub optarg_str {
    join(' ',&optarg);
}
sub optflag {
    my ($k,$v)=@_;
    &debug("optflag",$k,$v);
    $v ? ($k) : ()
}

sub ref2str {
    my ($x)=@_;
    (ref $x eq 'ARRAY') ? join(' ',@{$x}) : $x;
}

sub system_output_list {
    my ($cmd,$outlist)=@_;
    &debug("system_output_list",ref2str($cmd),$outlist);
    $outlist = [] unless defined($outlist);
    (ref $cmd eq 'ARRAY') ? open(SYSOUTP,'-|',@{$cmd}) : open(SYSOUTP,$cmd." |") || croak "system $cmd failed";
    @$outlist = <SYSOUTP>;
    my $ce=close SYSOUTP;
    (!$ce || $?>>8,$outlist)
}

sub system_output_list_force {
    my $c=$_[0];
    my ($e,$l)=&system_output_list;
    croak "system ".ref2str($c).": exit code $e" if $e;
    $l
}

sub system_force {
    croak "system ".join(' ',@_).": exit code ".($?>>8) if system(@_);
    0
}
my $system_warn_fn=\&warning;
sub set_system_warn_fn {
    $system_warn_fn=$_[0];
}
sub system_warn {
    my $ret=system(@_);
    if ($ret) {
        my $code=$?>>8;
        $system_warn_fn->("system ".join(' ',@_).": exit code $code");
        return $ret;
    }
    0
}

sub system_suffix {
    #quote shell args if @rest is more than 1 word, to put redirect suffix e.g. 2>&1 > /dev/null on. if $suffix is empty, then use regular system
    my ($suffix,@rest)=@_;
    $suffix?system((scalar @rest > 1 ? escaped_shell_args_str(@rest): $rest[0]).' '.$suffix):system(@rest)
}

sub system_force_suffix {
    my ($suffix,@rest)=@_;
    $suffix='' unless $suffix;
    croak "system ".join(' ',@rest).' '.$suffix.": exit code ".($?>>8) if system_suffix(@_);
    0
}

sub system_warn_suffix {
    my $ret=system_suffix(@_);
    if ($ret) {
        my $code=$?>>8;
        my ($suffix,@rest)=@_;
        $suffix='' unless $suffix;
        $system_warn_fn->("system ".join(' ',@rest).' '.$suffix.": exit code $code");
        return $ret;
    }
    0
}

sub system_opt {
    my ($opthash,@rest)=@_;
    &debug("system_opt",@_);
    my $quiet=$opthash->{quiet};
    my $stdoutto=$opthash->{stdout};
    my $stderrto=$opthash->{stderr};
    $stdoutto=$stderrto='/dev/null' if $quiet;
    my $suffix='';
    $suffix.=" >$stdoutto" if $stdoutto;
    $suffix.=" 2>$stderrto" if $stderrto;
    &debug("system_opt suffix rest",$suffix,@rest);
    $opthash->{warn} ? system_warn_suffix($suffix,@rest) : system_force_suffix($suffix,@rest);
}

# return hash of tcp ports that have listeners using netstat
sub used_tcp_ports {
    local $_;
    my @cmd=(&which_prog('netstat','/bin/netstat',1),'-n','--tcp','--protocol=inet');
#'--listen',
    #can't listen also if TIME_WAIT connects are there
    my ($ec,$o)=system_output_list(\@cmd);
    croak "error $ec with ".join(' ',@cmd) unless $ec==0;
    my $r={};
    for (@$o) {
        $r->{$1}=1 if /^tcp[^:]+:(\d+)\s+/;
    }
    $r
}

sub user_base_port {
    ($< % 7919) + 2000
}

sub get_tcp_port {
    my $p=shift;
    $p=&user_base_port unless defined($p);
    my $used=shift;
    $used=&used_tcp_ports unless $used;
    ++$p while(exists $used->{$p});
    $p
}

our $debug_fh=\*STDERR;
sub set_debug_fh {
    my $oldfh=$debug_fh;
    $debug_fh=$_[0] if defined $_[0];
    return $oldfh;
}

require 'dumpvar.pl';

my %mathsym=(
    '<=','leq',
    '>=','geq',
    '<','<',
    '>','>',
    '!=','neq',
    );
sub escapere {
    local ($_)=@_;
    s/(\p{IsPunct})/\\$1/g;
    $_
}
my $mathsyms=join('|',map {escapere($_)} keys(%mathsym));
my $mathsymre=qr/$mathsyms/;
sub subst_mathsym {
    my ($k)=@_;
    exists $mathsym{$k} ? "\\ensuremath{\\$mathsym{$k}}" : $k
}
sub escape_mathsym {
    my ($k)=@_;
    $k=~s/($mathsymre)/subst_mathsym($1)/oeg;
    $k
}
sub escape_latex {
    local ($_)=@_;
    # make latex safe
    s/([\$_%\{\}&])/\\$1/g;
    s/\^/\\\^\{\}/g;
    $_
}
sub pretty_latex {
    local ($_)=@_;
    $_=escape_latex($_);
    # bold any triple prestarred nonterm -- for highlighting
    s/(\(\s*\S*)\*\*\*(\S+)/$1\\textbf{$2}/g;
    escape_mathsym($_)
}


sub debugging {
 return $DEBUG;
}

sub set_debugging {
    $DEBUG=$_[0];
}

sub uniq_ref {
    my ($l,$seen,$luniq)=@_; # appends to @$luniq and adds counts to %$seen if elements in @$l weren't in seen already
    $seen={} unless defined $seen;
    $luniq=[] unless defined $luniq;
    no warnings 'uninitialized';
    no warnings 'numeric';
    for (@{$l}) {
        push @{$luniq},$_ if (!$seen->{$_}++);
    }
    ($luniq,$seen) # ref to list of uniques, seen with counts in orig.
}

sub uniq {
    my %seen;
    my @r;
    for (@_) {
        if (!$seen{$_}) {
            $seen{$_}=1;
            push @r,$_;
        }
    }
    @r
}

sub debug {
    if ($DEBUG) {
        no warnings 'uninitialized';
        my ($package, $filename, $line) = caller;
        $filename = $1 if $filename =~ m|/([^/]+)$|;
        my $dbg=((!defined($package) || $package eq 'main') ? '' : "[$package]")."$filename($line): ";
        my $oldfh=select($debug_fh);
        my $first=1;
        for (@_) {
            if ($first) {
                $first=0;
                print $dbg;
            } else {
                print '; ';
            }
            if (ref($_)) {
                print ref($_),': ';
                dumpValue($_);
                $first=1;
            } else {
                print $_;
            }
        }
        print "\n";
        select($oldfh);
    }
}

#copied from above; don't want to mess w/ caller
sub debug_force {

        no warnings 'uninitialized';
        my ($package, $filename, $line) = caller;
        $filename = $1 if $filename =~ m|/([^/]+)$|;
        my $dbg=((!defined($package) || $package eq 'main') ? '' : "[$package]")."$filename($line): ";
        my $oldfh=select($debug_fh);
        my $first=1;
        for (@_) {
            if ($first) {
                $first=0;
                print $dbg;
            } else {
                print '; ';
            }
            if (ref($_)) {
                print ref($_),': ';
                dumpValue($_);
                $first=1;
            } else {
                print $_;
            }
        }
        print "\n";
        select($oldfh);

}

sub visit_hashlist {
    my $code=shift;
    my $npairs=(scalar @_)/2;
    for my $i (0..$npairs-1) {
        $code->($_[2*$i],$_[2*$i+1]);
    }
}

sub pairlist_from_hashlist {
  my @ret;
#  my $npairs=(scalar @_)/2;
#  for my $i (0..$npairs-1) {
#      push @ret,[$_[2*$i],$_[2*$i+1]];
#  }
  visit_hashlist(sub{push @ret,[$_[0],$_[1]];},@_);
  return @ret;
}

# notice: actually, "letter" means all sorts of symbols, full utf8 ... and
# ncnamechar should include some other types of characters I know nothing about
my %xml=();

my $tagname;
my $tagopen;
$xml{NCNameChar}='[\p{isAlnum}._\-]';
$xml{NCName}=qq{[\\p{isAlpha}_]$xml{NCNameChar}*};
$xml{attr_val}= q{\s*=\s*("[^"]*"|'[^']*')};
$xml{attr}="($xml{NCName})$xml{attr_val}";
$xml{opentag_inside}="($xml{NCName})".'(\s+'.$xml{attr}.')*'; #(?{".'$tagname=$^N})
$xml{closetag_inside}='/\s*'."($xml{NCName})";
$xml{opentag}='\<\s*'."($xml{opentag_inside})".'\s*\>';
$xml{closetag}='\<\s*'."($xml{closetag_inside})".'\s*\>';
$xml{tag}='\<\s*'."($xml{closetag_inside}|$xml{opentag_inside})".'\s*\>';

sub xml_regexps {
    return %xml;
}

sub xml_regexp {
    return $xml{$_[0]} if exists $xml{$_[0]};
}

my $xml_entity_quot='&quot;';
my $xml_entity_apos='&apos;';
my @xmlquotes = (
                '"' => $xml_entity_quot,
                "'" => $xml_entity_apos,
               );
my $xml_entity_lt='&lt;';
my $xml_entity_gt='&gt;';
my @xmlangles= (
                '<' => $xml_entity_lt,
                '>' => $xml_entity_gt,
);
my $xml_entity_amp='&amp;';
my @xmlamp = ( '&' => $xml_entity_amp);
my @xmlents_noamp = ( @xmlquotes, @xmlangles );
my @xmlents = ( @xmlents_noamp, @xmlamp);
my %xmlent = @xmlents;
my %unxmlent = reverse %xmlent;

sub xml_escape_entities {
 my ($t)=@_;
 $t =~ s/(.)/$xmlent{$1}||$1/ge;
 return $t;
}

# expects the text *inside* the quotes. "([^"]*)".
sub xml_unescape_entities {
 my ($t)=@_;
 $t =~ s/(\&[^;]*;)/$unxmlent{$1}||$1/ge;
# $t =~ s/\Q$xml_entity_quot\E/"/og;
# $t =~ s/\Q$xml_entity_apos\E/'/og;
# $t =~ s/\Q$xml_entity_amp\E/&/og; # do this last or ambig.
 return $t;
}

sub xml_attr_quote {
 my ($t,$always_double)=@_;
 $t =~ s/\&/$xml_entity_amp/og;
 return qq{"$t"} unless $t =~ /\"/;
 return qq{'$t'} unless ($t =~ /\'/ || $always_double);
 $t =~ s/\"/$xml_entity_quot/og;
 return qq{"$t"};
}

sub xml_attr_unquote {
 local ($_)=@_;
 s/^\"(.*)\"$/$1/ || s/^\'(.*)\'$/$1/;
 return xml_unescape_entities($_);
}

sub get_xml_attr {
    my ($name,$t)=@_;
    return xml_attr_unquote($1) if $t =~ /\Q$name\E$xml{attr_val}/;
}

sub strip_xml_tags {
    my ($t)=@_;
    if ($DEBUG) {
        my @l=($t=~ /($xml{tag})/g);
        my @s=@+;
        my @f=@-;
        &debug(\@l,\@s,\@f);
    }
    $t =~ s/$xml{tag}//g;
    return $t;
}

sub get_xml_free_text {
    return xml_unescape_entities(&strip_xml_tags);
}

sub make_xml {
 my ($element,$text,$attrref)=@_;
#TODO $attref hash into attributes
 my @attr=();
 for my $k (sort keys %$attrref) {
     push @attr," $k=";
     push @attr,xml_attr_quote($attrref->{$k});
 }
 local $"='';
 return "<$element@attr>$text</$element>";
}

sub flush {
    my ($fh)=@_;
    my($old) = select($fh) if defined $fh;
    $| = 1;
    print "";
    $| = 0;
    select($old) if defined $fh;
}

sub unbuffer {
    my ($fh)=@_;
    my($old) = select($fh) if defined $fh;
    $| = 1;
    print "";
    select($old) if defined $fh;
}


use Cwd qw(getcwd abs_path chdir);

sub getcd {
# my $curdir=`pwd`;
# chomp $curdir;
# $curdir =~ s|/$||;
# return $curdir;
  return getcwd;
}

my $is_shell_special=qr.[ \t\n\\><|&;"'`~*?{}$!()].;
my $shell_escape_in_quote=qr.[\\"\$`!].;

sub nthline {
    my ($line,$file) = @_;
    local (*NTH);
    open NTH,"<",$file or croak "Couldn't open $file: $!";
    my $ret;
    while(<NTH>) {
        if (--$line == 0) {
            $ret=$_;
            last;
        }
    }
    close NTH;
    return $ret;
}

sub escape_shell {
    my ($arg)=@_;
    return undef unless defined $arg;
    return '""' unless $arg;
    if ($arg =~ /$is_shell_special/) {
        $arg =~ s/($shell_escape_in_quote)/\\$1/g;
        return "\"$arg\"";
    }
    return $arg;
}

sub cmdline {
    return join ' ',($0,@ORIG_ARGV);
}

sub escaped_shell_args {
    return map {escape_shell($_)} @_;
}

sub escaped_shell_args_str {
    return join ' ',&escaped_shell_args(@_);
}

sub escaped_cmdline {
    return "$0 ".&escaped_shell_args_str(@ORIG_ARGV);
}


#returns ($dir,$base) s.t. $dir$base accesses $pathname and $base has no
#directory separators
sub dir_file {
    my ($pathname)=@_;
    if ( $pathname =~ m|/| ) {
        croak unless $pathname =~ m|(.*/)(.*)|;
        return ($1,$2);
    } else {
        return ('./',$pathname);
    }
}

#no slash between dir and file.  if absolute, then dir='/'; if relative, dir='.'
sub dir_file_noslash {
    my ($dir,$file)=&dir_file(@_);
    $dir =~ s|/$|| unless $dir eq '/';
    $dir = '.' unless $dir;
    return ($dir,$file);
}

sub mydirname {
    my ($dir,$file)=&dir_file_noslash(@_);
    return $dir;
}

sub mybasename {
    my ($dir,$file)=&dir_file_noslash(@_);
    return $file;
}

sub which_in_path {
    my ($prog)=@_;
    my $which='/usr/bin/which';
    $which='which' unless -x $which;
    $prog=`$which $prog 2>/dev/null`;
    chomp $prog;
    return $prog;
}

our $info_fh=\*STDERR;

sub set_info_fh {
    my $oldfh=$info_fh;
    $info_fh=$_[0] if defined $_[0];
    return $oldfh;
}

sub chomped {
    my ($a)=@_;
    chomp $a;
    return $a;
}

sub info {
#substr($_[$#_],-1) eq "\n" ? "" :
    print $info_fh @_, "\n";
}

sub show_progress {
    my ($n,$big,$small,$max,$fh)=@_;
    $fh=$info_fh unless defined $fh;
    $big=10000 unless $big;
    $small=$big/10 unless $small;
    if (defined ($max)&&$n==$max) {
        print $fh $n,"\n";
    } elsif ($n % $big == 0) {
        print $fh $n;
    } elsif ($n % $small == 0) {
        print $fh '.';
    } else {
        return;
    }
    flush($fh);
}

sub done_progress {
    my ($n,$fh)=@_;
    $fh=$info_fh unless defined $fh;
    print $fh "(done";
    print $fh ", N=$n)" if defined $n;
    print $fh "\n";
    flush($fh);
}

my %info_counts=();

sub count_info {
    no warnings;
    my $n=$_[1] || 1;
    $info_counts{$_[0]}+=$n;
}

sub count_info_gen {
    my ($f)=@_;
    count_info($f);
    my $g=generalize_brackets($f);
    count_info($g) unless $f eq $g;
}
sub info_remember {
    my ($f)=@_;
    &info;
    count_info($f);
}

sub info_remember_gen {
    my ($f)=@_;
    &info;
    count_info_gen($f);
}

sub info_remember_quiet {
    &debug;
    &count_info($_[0]);
}

sub info_summary {
    return unless scalar %info_counts;
    my ($fh)=@_;
    local($info_fh)=$fh if defined $fh;
    info("(<#occurrences> <information type>):");
    &info_show_counts;
    &info_runtimes;
}

sub info_show_counts {
    my ($fh)=@_;
    local($info_fh)=$fh if defined $fh;
    info(sprintf('% 9d',$info_counts{$_}),"\t$_") for (sort keys %info_counts);
}

sub info_runtimes {
    info("(total running time: ".&runtimes.")\n");
}

sub warning {
    my ($f,@r)=@_;
    info_remember("WARNING: $f",@r);
    return undef;
}

sub warning_gen {
    my ($f,@r)=@_;
    info_remember_gen("WARNING: $f",@r);
    return undef;
}

sub fatal {
    my ($f,@r)=@_;
    info_remember("EXITING: $f",@r);
    all_summary();
    exit 0;
}

sub which_prog {
    my ($prog,$defaultprog,$quiet,$nocroak)=@_;
    $defaultprog='' unless defined $defaultprog;
    $quiet=0 unless defined $quiet;
    $nocroak=0 unless defined $nocroak;
    my $absprog = which_in_path($prog);
    my $error='';
    unless (-x $absprog) {
        my $scriptdir=&mydirname;
        $absprog="$scriptdir/$prog";
        unless (-x $absprog) {
            if ($defaultprog) {
                $absprog=$defaultprog;
                $error="Error: couldn't find an executable $prog in PATH, in $scriptdir, or at default $defaultprog!" unless -x $absprog;
            } else {
                $error="Error: couldn't find an executable $prog in PATH or in $scriptdir!";
            }
        }
    }
    if (-x $absprog) {
        info("Using $prog at $absprog") unless $quiet;
        return $absprog;
    }
    if ($nocroak) {
        info($error);
        return '';
    } else {
        croak $error ;
    }
}


sub which_prog_dir {
    my @a=@_;
    $a[1].="/$a[0]" unless $a[1] =~ /\Q$a[0]\E$/;
    which_prog(@a);
}

sub which {
    my ($fullprog,$defaultprog,$quiet,$nocroak)=@_;
    $fullprog =~ /(\S+)(.*)/ && return &which_prog($1,$defaultprog,$quiet,$nocroak) . $2;
}

sub which_dir {
    my @a=@_;
    $a[1].="/$a[0]" unless $a[1] =~ /\Q$a[0]\E$/;
    which(@a);
}

sub expand_symlink {
    my($old) = @_;
    local(*_);
    my $pwd=`pwd`;
    chop $pwd;
    $old =~ s#^#$pwd/# unless $old =~ m#^/#;  # ensure rooted path
    my @dir = split(/\//, $old);
    shift(@dir);                # discard leading null element
    $_ = '';
  dir: for my $dir (@dir) {
        next dir if $dir eq '.';
        if ($dir eq '..') {
            s#/[^/]+$##;
            next dir;
        }
        $_ .= '/' . $dir;
        while (my $r = readlink) {
            if ($r =~ m#^/#) {  # starts with slash, replace entirely
                $_ = &expand_symlink($r);
                s#^/tmp_mnt## && next dir;  # dratted automounter
            } else {            # no slash?  Just replace the tail then
                s#[^/]+$#$r#;
                $_ = &expand_symlink($_);
            }
        }
    }
    s/^\s+//;
    # lots of /../ could have completely emptied the expansion
    return ($_ eq '') ? '/' : $_;
}

use IPC::Open2;
use IPC::Open3;

my $cleanup_pid=undef;
my $waitpid_pid=undef;

sub cleanup {
    kill $cleanup_pid if defined $cleanup_pid;
}

use IO::Handle;


sub devnull {
    close DEVNULL;
    open(DEVNULL, "<", "/dev/null");
    return \*DEVNULL;
}

# first arg can be {mode=>quiet|std|merge} affecting stderr.  default (std) is
# same stderr as caller.  merge combines stderr into stdout pipe returned.
# quiet sends to /dev/null
# returns ($R,$W)=(child_out,child_in), i.e. you read from child's out and write to its in.
sub exec_pipe
{
    my ($args,@cmd)=@_;
    my ($regular,$mode);
    if (ref $args eq 'hash') {
        $mode=$args->{mode};
        $regular=1 if (!$mode or $mode eq 'std')
    } else {
        @cmd=@_;
        $regular=1;
    }

    my ($f_pid,$R,$W);
    if ($regular) {
#        ($R,$W)=(new IO::Handle,new IO::Handle); # should be done by autovivification in both
        $f_pid = open2($R,$W, @cmd);
    } else {
        $f_pid = open3($W,$R, $mode eq 'merge' ? '' : devnull(),@cmd);
    }
#    my $f_pid = open2($R,$W, @_);
    croak "couldn't exec_pipe[mode=$mode](".join(' ',@cmd).')' unless $f_pid;
    $waitpid_pid=$cleanup_pid=$f_pid;
    binmode $W;
    binmode $R;
    #    binmode $, ":utf8";
    return ($R,$W)
}

sub exec_pipe_pair
{
    [&exec_pipe]
}


sub line_pipe
{
    my ($R,$W,$line)=@_;
    $line="$line\n" unless substr($line,-1,1) eq "\n";
    my $old=select $W;
    $|=1;
    print $line;
    $|=0;
    select $old;
    scalar <$R>
}

sub line_pipe_pair
{
    my ($RW,$line)=@_;
    line_pipe($RW->[0],$RW->[1],$line);
}

sub close_pair
{
    my ($RW)=@_;
    close $RW->[1];
    close $RW->[0];
}

sub exec_pipe_parallel
{
    #takes list of cmdlist refs
    [map {[exec_pipe(@{$_})]} @_];
    #returns ref to list of [$R,$W] pairs
}

sub maybe_chomp {
    my ($chomp,$x)=@_;
    chomp $x if ($chomp);
    $x
}

sub line_pipe_parallel
{
    #returns list of outputs from each of the parallel pipe
    my ($RWlist,$line,$chomp)=@_;
    map {maybe_chomp($chomp,line_pipe_pair($_,$line))} @{$RWlist};
}

#returns list of lines in list context, concatenation in scalar
sub exec_filter
{
    my ($program,@input)=@_;
    my ($R,$W)=exec_pipe($program);
    print $W $_ for (@input);
    close $W;
    my @output = <$R>;
#    &debug("Output from $program",@output);
    close $R;
    $cleanup_pid=undef;
    local $,='';
    return wantarray ? @output : "@output";
}

#input may be a reference (prevent copy)
sub exec_filter_ref
{
    my ($input,@program)=@_;
    my ($R,$W)=exec_pipe(@program);
    if ($input) {
        my $oldh=select($W);
        $|=1;
        my $r=ref $input;
        if ($r) {
            if ($r eq 'ARRAY') {
                $|=0;
                print $_ for (@{$input});
                $|=1;
                print "";
            } elsif ($r eq 'SCALAR') {
                print ${$input};
            } else {
                fatal("exec_filter_ref - unknown input ref type $r");
            }
        } else {
            print $input;
            print "\n",if (substr($input,-1,1) ne '\n');
        }
        select($oldh);
    }
    close $W;
    my @output = <$R>;
#    &debug("Output from $program",@output);
    close $R;
    $cleanup_pid=undef;
    local $,='';
    return wantarray ? @output : "@output";
}

sub exec_filter_list
{
    return &exec_filter;
}

sub exec_exitvalue
{
    croak unless defined($waitpid_pid);
    my $ret=waitpid($waitpid_pid,0);
    $waitpid_pid=undef;
    return $ret;
}

#usage: appendzname("a.gz",".suffix") returns "a.suffix.gz",
#appendzname("a",".suffix") return "a.suffix"
sub appendz {
    my ($base,$suffix)=@_;
    my $zext= ($base =~ s/(\.gz|\.bz2|\.Z)$//) ? $1 : '';
    return $base.$suffix.$zext;
}

#usage: $fh=openz($filename); while(<$fh>) ...
sub openz {
    my ($file)=@_;
    my $fh;
    croak "no input file specified" unless $file;
    if ($file =~ /\.gz$/) {
        open $fh,'-|','gunzip','-f','-c',$file or croak "can't gunzip -c $file: $!";
    } elsif ( $file =~ /\.bz2$/) {
        open $fh,'-|','bunzip2','-c',$file  or croak "can't bunzip2 -c $file: $!";
    } elsif ( $file =~ /^(http|ftp|gopher):/) {
        open $fh,'-|','GET',$file  or croak "can't GET $file: $!";
    } else {
        open $fh,$file or croak "can't read $file: $!";
    }
    return $fh;
}

#usage: &argvz; while(<>) ...
sub argvz() {
    for (@ARGV) {
        $_ = "gunzip -f -c ".escape_shell($_)." |" if /\.gz$/;
        $_ = "bunzip2 -c ".escape_shell($_)." |" if /\.bz2$/;
        $_ = "GET ".escape_shell($_)." |" if /^(http|ftp|gopher):/;
    }
}

sub last_file_line() {
    my ($f)=$ARGV;
    $f=~ s/^.*|\S+// if /\.(gz|bz2)$/;
    return ($f,$.);
}

my $out_encoding;

#usage: $fh=openz($filename); print $fh "line\n" ...
sub openz_out_cmd {
    my ($file,$nomkdir)=@_;
    &mkdir_parent($file) unless $nomkdir;
    my $fh;
    if ($file =~ /\.gz$/) {
        return "|gzip -c > $file";
    } elsif ( $file =~ /\.bz2$/) {
        return "|bzip2 -c > $file";
    } elsif (!$file) {
        return ">/dev/null";
    }
    return ">$file";
}

#usage: $fh=openz($filename); print $fh "line\n" ...
sub openz_out {
    my ($file)=@_;
    croak "no output file specified" unless $file;
    my $cmd=&openz_out_cmd($file);
    my $fh;
    if ($cmd) {
        open $fh,$cmd or croak "can't open for output: $cmd: $!";
    } else {
        if ($file eq '-') {
            return \*STDOUT;
        } elsif ($file eq '-2') {
            return \*STDERR;
        } else {
            ($fh,$file)=tempfile($file) if ($file =~ /XXXX$/);
            open $fh,">",$file or croak "can't open for write: $file: $!";
        }
    }
    binmode $fh,$out_encoding if $out_encoding;
    return wantarray ? ($fh,$file) : $fh;
}

sub openz_basename {
    my ($file)=@_;
    $file =~ s/(\.bz2|\.gz)$//;
    return $file;
}


sub expand_tilde {
    my $dir = shift;
    # tilde expansion
    if ($dir =~ /~/) {
	$dir =~ s{ ^ ~( [^/]* ) }
	{ $1 ? (getpwnam($1))[7] : ($ENV{'HOME'} || $ENV{'LOGDIR'} || (getpwuid($>))[7]) }ex;
    }
    return $dir;
}

sub ls_grep {
  my ($dir,$pattern,$include_dir,$include_dot)=@_;
  $include_dir=1 unless defined $include_dir;
  $include_dot=0 unless defined $include_dot;
  $dir='.' unless defined $dir;
  $include_dir=0 if $dir eq '.';
  $pattern='' unless defined $pattern;
  $dir =~ s|/$||;

  -d $dir or croak "$dir is not a directory!";
  opendir LS_RECENT,$dir or croak $!;
  my @files=grep !/^\.\.?$/, readdir LS_RECENT;
  @files = grep !/^\./, @files unless $include_dot;
  close LS_RECENT;
#  &debug("ls_recent(all):",@files);
  my @filtered=grep /$pattern/, @files;
  @filtered=map {"$dir/$_"} @filtered if $include_dir;
#  &debug("ls_grep(filtered):",@filtered);
  return @filtered;
}

#if no mtime, you don't get it back. (filters for things that have a -M then
#sorts by it)
sub sort_mtime {
    &debug("mtimes",map { ($_,-M) } @_);
    return map { $_->[0] }  # restore original values
      sort { $a->[1] <=> $b->[1] }  # sort
       map { [$_, -M] } (grep { defined -M } @_);  # transform: value, sortkey
}

sub ls_mtime {
  my @sorted=&sort_mtime(ls_grep(@_));
#  &debug("ls_mtime(sorted):",@sorted);
  return @sorted;
}


sub cp_file_check {
    my ($from,$to,@opts)=@_;
    my $absfrom=&abspath_from($from,undef,1);
    my $absto=&abspath_from($to,undef,0);
    return warning("cp_file: source $from(=$absfrom) is the same as destination $to(=$absto)") if ($absfrom eq $absto);
    -f $from or return warning("cp_file: source file $from not readable");
    unlink $to || return warning("cp_file: couldn't remove old destination $to: $?") if -r $to;
    my $cp_bin="/bin/cp";
    $cp_bin="cp" unless -x $cp_bin;
    system $cp_bin,@opts,"--",$from,$to;
    if (my $exit=$?>>8) {
        local $"=' ';
        return warning("$cp_bin from $from to $to with opts ".join(' ',@opts)," failed (EXIT: $?)");
    }
}

sub cp_file_force {
    return cp_file_check(@_) or croak "failed to cp_file_check ".join(' ',@_);
}

my $MAXPATH=200; # room for suffixes
sub superchomp {
    my ($ref)=@_;
    if ($$ref) {
        $$ref =~ s|^\s+||;
        $$ref =~ s|\s+$||;
        $$ref =~ s|\s+| |g;
    }
}

sub superchomped {
    my ($a)=@_;
    superchomp(\$a);
    $a
}

sub unquote {
    local ($_)=@_;
    (/^"(.*)"$/) ? $1 : $_;
}

sub filename_from {
    my ($fname)=@_;
   &superchomp(\$fname);
   $fname =~ s|[^a-zA-Z0-9_=-]+|.|g;
   $fname =~ s|^\.|_|;
    return $fname;
}

sub strip_dirnames {
    my ($fname)=@_;
    $fname =~ s|/\S*/([^/ ]+)|$1|g;
    return $fname;
}

### for checkjobs.pl/qsh.pl
sub normalize_jobname {
   my ($fname,$JOBSDIR)=@_;
   $fname=&strip_dirnames($fname);
   $fname=&filename_from($fname);
   my $pathname=substr "$JOBSDIR/$fname",0,$MAXPATH;
   return ($fname,$pathname);
}

sub runtimes {
  my ($user,$sys,$cuser,$csys)=times;
  my $tuser=$user+$cuser;
  my $tsys=$sys+$csys;
  my $ttotal=$tuser+$tsys;
  return "$ttotal wall seconds, $tuser user seconds, $tsys system seconds";
}
###

sub println {
    print @_,"\n";
}

sub read_file_lines_ref {
    my ($filename,$lref)=@_;
    my ($R)=openz($filename);
    @$lref=<$R>;
#    local(*READ_FILE);
#    open READ_FILE,"<",$filename || croak "couldn't read file $filename: $!";
#    @$lref=<READ_FILE>;
#    close READ_FILE;
    close $R;
    scalar @$lref;
}

sub read_file_line {
    my ($filename)=@_;
    my ($R)=openz($filename);
    my $line=<$R>;
    close $R;
    $line
}

sub read_file_line_chomped {
    my $t=&read_file_line;
    superchomp(\$t);
    $t
}

sub read_file_lines {
    my @ret;
    my ($filename,$singleline)=@_;
    local($/) if $singleline;
    read_file_lines_ref($filename,\@ret);
    @ret
}

sub nlines {
    my ($f)=@_;
    require_file($f);
    local $_=`wc -l "$f"`;
    die("error counting lines in $f: exit code ".$?>>8) if $?;
    chomp;
    die("bad wc-l output for $f: $_") unless /^\s*(\d+)/;
    $1
}
sub grep_file_lines {
    my ($f,$pattern,$limit)=@_;
    my ($R)=openz($f);
    local $_;
    my @r;
    while(<$R>) {
        if (/$pattern/) {
          push @r,$_;
          return @r if $limit && scalar @r >= $limit;
        }
    }
    @r
}

sub write_file_lines {
    my ($filename,$noclobber,$perm,@lines)=@_;
    $noclobber=0 unless defined $noclobber;
    $perm=0644 unless defined $perm;
    if ($filename ne '-') {
        if (-f $filename) {
            return &warning("$filename already exists") if $noclobber;
            unlink $filename;
        }
    }
    &debug("writing file $filename ",scalar @lines?"(text $lines[0] ...)":"(empty)");
    my ($fh,$fn)=openz_out($filename);
    if ($filename ne '-') {
        chmod($perm,$filename) or croak "couldn't set permissions on $filename to $perm: $!";
    }
    print $fh @lines;
    print "\n" unless scalar @lines && $lines[$#lines] =~ /\n\s*$/m;
    close $fh unless $fh == \*STDOUT;
    return $fn;
}

sub write_file {
    my ($filename,$text,$noclobber,$perm)=@_;
    write_file_lines($filename,$noclobber,$perm,$text);
}

sub create_script {
    my ($scriptname,$text,$interp,$noclobber)=@_;
    $interp="/bin/bash" unless defined $interp;
    return write_file($scriptname,"#!$interp\n$text\n",$noclobber,0755);
}

sub quote_list {
    local($")=", ";
    my @ql=map { "q{$_}" } @_;
    return "(@ql)";
}

sub pretty_list {
    '('.join(' ',@_).')';
}

sub get_blobs {
    $ENV{BLOBS}='/home/hpc-22/dmarcu/nlg/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    return $ENV{BLOBS};
}


sub clone_shell_script {
   my ($scriptname,$text,$cd,$bash_exe,$noclobber)=@_;
   $bash_exe=&get_blobs."/bash3/bin/bash" unless defined $bash_exe;
   $bash_exe="/bin/bash" unless -x $bash_exe;
   my $preamble='BLOBS=${BLOBS:-'.&get_blobs.'}'.<<'EOSCRIPT';

[ -d $BLOBS ] || BLOBS=~/blobs
export BLOBS
d=`dirname $0`
if [ -f $BLOBS/bashlib/unstable/bashlib.sh ] ; then
 . $BLOBS/bashlib/unstable/bashlib.sh
else
 if [ -f $d/bashlib.sh ] ; then
  . $d/bashlib.sh
 fi
fi
EOSCRIPT
   $cd="\ncd ".&getcd."\n" unless defined $cd;
   create_script($scriptname,$preamble.$cd.$text,$bash_exe,$noclobber);
 }

#allincs is boolean.  this file is included, hopefully by absolute path.  if ENV
#were set this would be closer to a continuation of the calling script $cd if
#undef: set cd to current cd when script starts.  (i.e. to not cd, just set to '')
sub clone_perl_script {
    my ($scriptname,$text,$cd,$allincs,$perl_exe,$noclobber)=@_;
    $perl_exe=$^X unless defined $perl_exe;
    $perl_exe = which_in_path($perl_exe) if ($perl_exe !~ m|^/|);
    $allincs='' unless defined $allincs;
    my $incs=quote_list(@INC);
    my ($package, $filename, $line) = eval {caller};
    my $preamble=<<EOSCRIPT;
use strict;
use warnings;
require "$filename";

EOSCRIPT
    my $pwd=&getcd;
   $cd=<<EOSCRIPT unless defined $cd;
chdir q{$pwd} or croak "Couldn't cd $pwd: \$?";
EOSCRIPT
    $preamble="push \@INC,$incs;\n$preamble" if $allincs;
    return create_script($scriptname,$preamble . $cd. $text,$perl_exe,$noclobber);
}

#appends filename-safe ARGV of script or ENV{suffix} if set, unless
#ENV{overwrite}, in which case $out is available unmodified usage:
#$cmd="logerr=1 $exec_safe_bin \$out blah blah \$argv" note: $cmd is
#interpolated in double quotes which is why you escape $out and $argv (note:
#$argv is shell-escaped.  also available: ($outdef eq
#$outdirdef.$outfiledef).$suffix eq $outdir.$outfile.  also $in versions: $in
#will be pulled from $ENV{infile} or modified from $ENV{insuffix} (at runtime)
#if available, otherwise the default will be used.
sub perl_shell_appending_argv {
    my ($in,$out,$cmd,$perlcode)=@_;
    $perlcode='' unless defined $perlcode;
    local($")=' ';
    return <<EOSCRIPT;
# perl_appending_argv in=$in out=$out cmd={{{$cmd}}} perlcode={{{$perlcode}}}
{
 my (\$indef)=q{$in};
 my (\$indirdef,\$infiledef)=dir_file(\$indef);
 my \$insuffix=exists \$ENV{insuffix} ?  \$ENV{insuffix} : '';
 my \$in = append_basename(\$indef,\$insuffix);
 \$in = \$ENV{infile} if exists \$ENV{infile};
 my (\$indir,\$infile)=dir_file(\$in);

 my \$outdef=q{$out};
 my (\$outdirdef,\$outfiledef)=dir_file(\$outdef);
 my \$argv=join ' ',\@ARGV;
 my \$suffix=filename_from(\$argv);
 \$suffix=\$ENV{suffix} if exists \$ENV{suffix};
 my \$out=\$ENV{overwrite} ? \$outdef : append_basename(\$outdef,\$suffix);
 my (\$outdir,\$outfile)=dir_file(\$out);
 &mkdir_force(\$outdir);

 &debug("in=\$in out=\$out suffix=\$suffix insuffix=\$insuffix,\$in,\$out");
 $perlcode;
 \$argv=&escaped_shell_args_str(\@ARGV);
 my \$cmd="$cmd";
 &debug("CMD=\$cmd");
 system \$cmd;
 if (my \$exit=\$?>>8) {
  print  STDERR "ERROR: CMD={{{\$cmd}}} FAILED, exitcode=\$exit" ;
  exit \$exit;
 }
}

EOSCRIPT
}

sub abspath_from {
    my ($abspath,$base,$expand_symlink) = @_;
    $expand_symlink=0 unless defined $expand_symlink;
    $base=&getcd unless defined $base;
    $abspath =~ s/^\s+//;
    $base =~ s/^\s+//;
    $abspath =~ s/^\~/$ENV{HOME}/e;
    $base =~ s/^\~/$ENV{HOME}/e;
    if ($abspath !~ m|^/|) {
        &debug("$abspath not absolute, so => $base/$abspath");
        $abspath = "$base/$abspath";
    }
    $abspath =~ s|/\.$||;
    &debug($abspath);
    return &expand_symlink($abspath) if $expand_symlink;
    $abspath =~ s|^\s+||;
    $abspath
}

sub abspath {
    abspath_from($_[0],undef,1);
}

sub inc_numpart {
    my ($text)=@_;
    return ($text =~ s/(\d+)/$1+1/e) ? $text : undef;
}

sub files_from {
    my @files=();
    while(defined (my $f=shift)) {
        while(-f $f) {
            push @files,$f;
            $f=inc_numpart($f);
        }
    }
    return @files;
}

use POSIX qw(HUGE_VAL);
my $INFINITY=HUGE_VAL();

sub getln {
    no warnings 'numeric';
    my ($n)=@_;
    return $n+0 if ($n =~ s/^e\^//);
    return ($n > 0 ? log $n : -$INFINITY);
}

sub getreal {
    my ($n)=@_;
    return exp($1) if ($n =~ /^e\^(.*)$/);
    return $n;
}

sub get_ehat {
    return 'e^'.&getln;
}


sub mean {
    my ($sum,$sumsq,$n)=@_;
    return $sum/$n;
}

sub statcounts {
    my $sum=0;
    my $sumsq=0;
    my $n=scalar @_;
    for my $x (@_) {
        $sum+=$x;
        $sumsq+=$x*$x;
    }
    ($sum,$sumsq,$n)
}
sub variance {
    my ($sum,$sumsq,$n)=@_;
    return 0 if ($n<=1);
    my $v=($sumsq-$sum*$sum/$n)/($n-1); # = sample variance (unbiased) - see http://en.wikipedia.org/wiki/Bessel%27s_correction
    return ($v>0?$v:0);
}
sub realvariance {
    my ($sum,$sumsq,$n)=@_;
    return 0 if ($n<=1);
    my $v=($sumsq-$sum*$sum/$n)/$n;
    return ($v>0?$v:0);
}
sub dotprod {
    my ($xl,$yl)=@_;
    my $s=0.;
    for my $i (0..$#$xl) {
        $s+=$xl->[$i]*$yl->[$i];
        #debug($#$xl,$s,$xl->[$i],$yl->[$i]);
    }
    $s
}
use List::Util qw(sum);
sub covariance {
    my ($xl,$yl,$xsum,$ysum)=@_;
    my $n=scalar @$xl;
    die unless $n == scalar @$yl;
    if (!defined($xsum)) {
        $xsum=sum(@$xl);
        $ysum=sum(@$yl);
    }
    covariance_counts(dotprod($xl,$yl),$xsum,$ysum,$n)
}
sub covariance_counts {
    my ($dotprod,$xsum,$ysum,$n)=@_;
    ($dotprod-($xsum/$n)*$ysum)/$n
}
sub linear_regress {
    #return (b,a) so y=x*b+a+(error) with min sum squared error
    #$yl undefined -> (0...$#$xl)
    my ($xl,$yl)=@_;
    ($xl,$yl)=([0..$#$xl],$xl) if !defined($yl);
    my $n=scalar @$xl;
    die unless scalar @$yl == $n;
    return (0,$xl->[0]) if $n<2;
    my ($xs,$xs2)=statcounts(@$xl);
    my $ys=sum(@$yl);
    linear_regress_counts(dotprod($xl,$yl),$xs,$xs2,$ys,$n);
}
sub linear_regress_counts {
    my ($dotprod,$xs,$xs2,$ys,$n)=@_;
    my $b=covariance_counts($dotprod,$xs,$ys,$n)/realvariance($xs,$xs2,$n);
    my $a=($ys-$b*$xs)/$n;
    ($b,$a)
}
sub linear_regress_range {
    #generalization of max{y}-min{y}
    my ($xl,$yl)=@_;
    my $n=scalar @$xl;
    my ($b,$a)=linear_regress($xl,$yl);
    ($n-1)*$b
}
sub stddev {
    sqrt(&variance(@_))
}

sub stderror {
    my ($sum,$sumsq,$n)=@_;
    &stddev/sqrt($n)
}

#returns array of indices from input string of form 1,2,3-10,... (sorted!)
sub parse_range {
    my ($ranges)=@_;
    my @ret;
    if ($ranges ne "") {
        while ($ranges=~/([^,]+)/g) { # for each comma delimited string
            if ($1 =~ /(\d+)-(\d+)/) { # if it's a range
                warning("parse_range: range $1-$2 empty ($2 < $1)") if $2 < $1;
                push @ret,$_ for ($1 .. $2);
            } else {
                push @ret,$1;
                warning("parse_range: $1 is not a nonnegative integer") unless $1 =~ /^\d+$/;
            }
        }
    }
    return sort @ret;
}

sub multiset_from_list_ref {
    my ($l)=@_;
    my $h={};
    no warnings 'numeric';
    local $_;
    $h->{$_}++ for (@$l);
    return $h;
}

sub multiset_from_list {
    return %{multiset_from_list_ref(\@_)}
}


sub index_from_list {
    my %ret;
    my $i=0;
    local $_;
    for (@_) {
        $ret{$_}=$i++;
    }
    return %ret;
}


sub parse_comma_colon_hash {
    my $format="expected comma separated key:value pairs, e.g. 'name:Joe,2:3' with no comma or colon appearing in keys or vals";
    my ($hash)=@_;
    my @ret;
    for (split /[,\n]/,$hash) {
        s/\s*//g;
        unless (/^,?([^:]+):([^:]+),?$/) {
            croak "$format - on $_, {{{$hash}}} instead - , parsed (@ret) so far.";
        }
        push @ret,$1,$2;
    }
    return @ret;
}

sub filename_from_path_end {
    my($path,$n,$slash_replacement)=@_;
    $n=1 unless defined $n;
    $slash_replacement = '_' unless defined $slash_replacement;
    my @comps=();
    while($path =~ m|([^/]+)|g) {
        push @comps,$1;
    }
    my $start=(scalar @comps)-$n;
    $start=0 if $start<0;
    return join($slash_replacement,map { $comps[$_] } ($start..$#comps));
}

sub mkdir_check {
    my ($dir)=@_;
    my $base='';
    while($dir=~m|(/?[^/]*)|g) {
        if ($1) {
            $base.=$1;
#            &debug("mkdir $base");
            croak "Couldn't mkdir $dir because $base is a file" if -f $base;
            mkdir $base unless -d $base;
        }
    }
    system('mkdir','-p','$dir')  unless -d $dir;
    return -d $dir;
}

sub mkdir_force {
    &mkdir_check(@_) or croak "Couldn't mkdir @_ $!";
}

sub mkdir_parent {
    &mkdir_force(&File::Basename::dirname($_[0]))
}


# mkdir by appending number to name if it already exists.  vulnerable to race (2 callers could end up w/ same dir). return name. give up after N tries.  N=0 means forever. doens't create parents.
sub mkdir_fresh {
    my ($d,$N,$no0)=@_;
    $N=1000 unless defined($N);
    my $n=0;
    while (1) {
        my $dn=$no0&&$n==0 ? $d : "$d".sprintf("%04d",$n);
        if (! -e $dn) {
            mkdir $dn;
            return $dn if (-d $dn);
        }
        $n=($n||0)+1;
        croak "couldn't create directory $d ... $d$N" if $N&&$n>$N;
    }
}

sub file_fresh {
    my ($d,$N,$no0)=@_;
    $N=1000 unless defined($N);
    my $n=0;
    while (1) {
        my $dn=$no0&&$n==0 ? $d : "$d".sprintf("%04d",$n);
        if (! -e $dn) {
            my $fh;
            return ($dn,$fh) if (open $fh,'>',$dn && $fh);
        }
        $n=($n||0)+1;
        croak "couldn't create file $d ... $d$N" if $N&&$n>$N;
    }
}


# if path=a/b, returns a${suffix}/b.  if path = ./b or b, returns ${suffix}b
sub append_basename {
    my ($path,$suffix)=@_;
    my ($dir,$base)=dir_file_noslash($path);
    return $suffix.$base if ($dir eq '.');
    return "$dir$suffix/$base";
}

my $stderr_replaced=0;
my $stdout_replaced=0;
sub tee_stderr {
    &debug("also writing STDERR to",@_);
    open (SAVE_STDERR, ">&STDERR");
    open (STDERR, "|tee -- @_ 1>&2");   # No use checking for errors (child
                                        # process)
    &unbuffer(\*STDERR);
    $stderr_replaced=1;
}

sub replace_stderr {
    my ($openstring)=@_;
    open (SAVE_STDERR, ">&STDERR");
    close STDERR;
    $stderr_replaced=1;
    $openstring=">$openstring" unless $openstring =~ /^\s*[>|]/;
    if (open(STDERR,$openstring)) {
        info("Replaced STDERR with $openstring");
    } else {
        &restore_stderr;
        info("Couldn't replace STDERR with $openstring: $! $?");
    }
    set_info_fh(\*STDERR);
}

sub replace_stderr_fh {
    my ($fh)=@_;
    my $fn=fileno $fh;
    open (SAVE_STDERR, ">&STDERR");
    close STDERR;
    $stderr_replaced=1;
    if (open(STDERR,">&=$fn")) {
        info("Replaced STDERR with filehandle");
    } else {
        &restore_stderr;
        info("Couldn't replace STDERR with filehandle: $! $?");
    }
    set_info_fh(\*STDERR);
}

#program will hang unless you close STDERR after tee_stderr (just call this)
sub restore_stderr {
    if ($stderr_replaced) {
        close (STDERR);
        open (STDERR, ">&SAVE_STDERR");
        close (SAVE_STDERR);
        $stderr_replaced=0;
    }
}

sub tee_stdout {
    open (SAVE_STDOUT, ">&STDOUT");
    open (STDOUT, "|tee -- @_");   # No use checking for errors (child process)
    &unbuffer(\*STDOUT);           # For my use, I wanted un-buffered.
    $stdout_replaced=1;
}


sub replace_stdout_fh {
    my ($fh)=@_;
    my $fn=fileno $fh;
    croak "bad stdout replacement - couldn't get fileno $fh" unless defined $fn;
    open (SAVE_STDOUT, ">&STDOUT");
    close STDOUT;
    $stdout_replaced=1;
    open STDOUT,">&=$fn";
    \*STDOUT;
}

sub replace_stdout {
    my ($openstring)=@_;
    return if $openstring eq ">-";
    open (SAVE_STDOUT, ">&STDOUT");
    close STDOUT;
    $stdout_replaced=1;
    $openstring=">$openstring" unless $openstring =~ /^\s*[>|]/;
    if (open(STDOUT,$openstring)) {
        info("Replaced STDOUT with $openstring");
    } else {
        &restore_stdout;
        info("Couldn't replace STDOUT with $openstring: $! $?");
    }
    \*STDOUT;
}

sub outz_stdout {
    my ($file)=@_;

    if ($file && $file ne "-") {
        if ($file =~ /\.(gz|bz2)$/) {
            replace_stdout(openz_out_cmd($file));
            return $file;
        }
        #FIXME: doesn't work for .gz
        my ($fh,$fn)=openz_out($file);
        replace_stdout_fh($fh);
        $fn;
    }
}

sub openz_stdout {
    &outz_stdout;
}

sub outz_stderr {
    my ($file)=@_;
    replace_stderr_fh(openz_out($file)) if ($file);
#    replace_stderr_fh(openz_out_cmd($file)) if ($file);
}

sub openz_stderr {
    &outz_stderr;
}

#program will hang unless you close STDOUT after tee_stdout (just call this)
sub restore_stdout {
    if ($stdout_replaced) {
        close (STDOUT);
        open (STDOUT, ">&SAVE_STDOUT");
        close (SAVE_STDOUT);
        $stdout_replaced=0;
    }
}

END {
 &restore_stdout;
 &restore_stderr;
}

sub get_enc_name {
    my ($enc)=@_;
    $enc="bytes" unless defined $enc;
    $enc=":$enc" unless $enc =~ /^:/;
    return $enc;
}

#$enc: encoding(iso-8859-7) bytes utf8 crlf etc.
#see http://www.devdaily.com/scw/perl/perl-5.8.5/lib/open.pm.shtml
#should affect stdin/out/err, AND all future opens (not already opened handles)
#crap, doesn't work: use is lexically scoped.
sub set_inenc {
    my ($enc)=@_;
    $enc="bytes" unless defined $enc;
    $enc=":$enc" unless $enc =~ /^:/;
#    use open IN  => $enc;
    binmode STDIN, $enc;
    return $enc;
}

sub set_outenc {
    my ($enc)=@_;
    $enc="bytes" unless defined $enc;
    $enc=":$enc" unless $enc =~ /^:/;
#    use open OUT  => $enc;
    binmode STDOUT, $enc;
    binmode STDERR, $enc;
    $out_encoding=$enc;
    return $enc;
}

#e.g. copy_file_code("in.utf8",":utf8","out.gb2312",":euc-cn")
sub copy_file_code {
    my ($f1,$f1code,$f2,$f2code)=@_;
    my $in=openz($f1);
    binmode $in,$f1code;
    my $out=openz_out($f2);
    binmode $out,$f2code;
    print $out $_ while(<$in>);
}

sub transcode {
    my ($s,$senc,$retenc)=@_;
    encode($retenc,decode($senc,$s))
}

sub set_ioenc {
    set_inenc(@_);
    return set_outenc(@_);
}

sub require_files {
    -f $_ || croak "missing file: $_" for @_;
}

sub require_file {
    &require_files;
}

sub require_dirs {
    -d $_ || croak "missing file: $_" for @_;
}

sub require_dir {
    &require_dirs;
}

sub show_cmdline {
 info("\nCOMMAND LINE:");
 info("cd ".&getcd." && \\\n  ".&escaped_cmdline);
 info();
}

#TODO: test on LIST/HASH, make a recursive ref_to_string dispatch (see WebDebug.pm)
sub ref_to_string {
    my ($pval,$escape,$print_unknown)=@_;
    return 'undef' unless ref($pval);
    my $type=ref($pval);
    return ($escape? escape_shell($$pval) : $$pval) if $type eq 'SCALAR';
    return '('.join ',',@$pval.')' if $type eq 'ARRAY';
    return '('.join ',',(map {$_ . "=>". $pval->{$_}} keys(%{$pval})).')' if $type eq 'HASH';
    return $print_unknown ? "[REF: $pval]" : '';
}

use Getopt::Long;

sub getoptions_or_croak {
    croak "Bad command line options" unless getoptions_catch(@_);
}

sub defined_ref {
#return true iff arg is a ref to a defined scalar or nonempty array/hash.
    my ($r)=@_;
    my $rr=ref $r;
    $rr ? (($rr eq 'SCALAR') ? defined($$r)
           : ($rr eq 'ARRAY') ? scalar(@$r)
           : ($rr eq 'HASH') ? scalar(%$r)
           : 0
        ) : 0
}

sub yield_vals {
    #return list of values from list of refs/scalars (hash gets val not key)
    local $_;
    my @r;
    for (@_) {
        my $t=ref $_;
        push @r,!$t ? $_ : ($t eq 'SCALAR' ? $$_ : $t eq 'ARRAY' ? @$_ : values %$_);
    }
    @r
}

sub usage_for_arg {
    local ($_)=@_; # ref to ARRAY
    my $option="  --".$_->[0];
    my $description=defined $_->[2] ? $_->[2] : '';
    my $vref=$_->[1];
    my $t=uc(&type_from_arg($_));
    $option=~s/=.*/=$t/ if $t;
    $description .=" (default=".quote_nonnumeric($$vref).")" if defined $vref and ref($vref) eq 'SCALAR' and defined $$vref;
    ($option,$description)
}

sub usage_for_arg_str {
    my ($o,$d)=&usage_for_arg;
    "$o [$d]"
}

#undef unless input is [name,ddest,usage,...TYPE]. returns TYPE
sub type_from_arg {
    my ($r)=@_;
    return '' unless (ref $r eq 'ARRAY' && scalar @$r>=4);
    my $t=$r->[$#$r];
    $t eq 1 ? 'required' : $t;
}

sub required_from_usage {
    #returns [ [\$varref,"var usage"] ... ]
    local $_;
    my $r=[];
    for (@_) {
        my $reqtype=type_from_arg($_);
        #if (ref $_ eq 'ARRAY' && scalar @$_>=4) {
         #   my $reqtype=$_->[$#$_];
            if ($reqtype) {
                # required arg: ['a'=>\$a,"a option",1] # 1 means required. 'file' means required (existing) input file. 'dir' means required (existing) dir
#                &debug("required from usage",$_);
                push @$r,[$_->[1],usage_for_arg_str($_),$reqtype];
            }
        #}
    }
#    &debug("REQUIRED OPTIONS:",$r);
    $r
}

my %check_type = (
    'file' => sub { $_ eq '-' || -r $_ },
                  'file?' => sub { !$_ || $_ eq '-' || -r $_ },
    'dir' => sub { -d $_ },
    'exe' => sub { -x $_ },
    'required' => sub { 1 },
);

sub bad_type {
    my ($ref,$t)=@_;
    $t=lc($t);
    my $check=$check_type{$t};
    croak "no check_type{$t}" unless $check;
    local $_;
    for (yield_vals($ref)) {
        return $_ unless &$check;
    }
    undef
}
sub missing_options {
    # return list of missing options (empty if ok). arg is [[\$ref,"usage",1|'file'|'dir'] ... ]
    my ($req)=@_;
    &debug('have_required_options',$req);
    my @missing;
    for my $r (@$req) {
        my ($ref,$usage,$type)=@$r;
        $type='required' unless defined $type;
        &debug("checking required arg ",$ref,$type,$usage);
        my $optional=$type=~/\?$/; #optional: type ends in ?. check type if defined
        if ($optional || defined_ref($ref)) {
            my $errtype;
            if (!$optional || scalar yield_vals($ref)) {
                push @missing,"\t(bad $type: $errtype)\t$usage\n" if ($errtype=bad_type($ref,$type));
                &debug("have required arg $usage, checking type: $type",$errtype);
            }
        } else {
            push @missing,"\t(MISSING)\t".$usage." \n";
        }
    }
    @missing
}

my ($getoptions_warn);
sub getoptions_catch {
    &debug('getoptions_catch',@_);
    $getoptions_warn=undef;
    local $SIG{__WARN__}=sub {$getoptions_warn=$_[0]};
    if (GetOptions(@_) && !defined($getoptions_warn) ) { # can change to || because warn is always called.
        return 1;
    } else {
        &show_opts;
        info();
        warning "Error parsing options:\n$getoptions_warn";
        return 0;
    }
}

sub getoptions_catch_required {
    my $required=shift;
    &getoptions_catch && have_required_options($required)
}

use English;

our ($option,$description);
#note: this is nicer, but looks terrible with fixed-width font
format OptionDescriptionTabular =
@>>>>>>>>>>>>>>>>>>>>>>>>>>  ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$option,                        $description
                             ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<~~
                                 $description
.

format OptionDescription =
 @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$option
	^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<~~
    $description
.

sub write_option_description {
    my $tabular=0;
    ($option,$description,$tabular)=@_;
    $FORMAT_NAME=($tabular ? 'OptionDescriptionTabular':'OptionDescription');
    write;
}

format Description =
^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$description
  ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<~~
  $description
.

sub write_description {
    $FORMAT_NAME='Description';
    ($description)=@_;
    write;
}

my $showopts_column=0;
sub set_tabular_show_opts {
    $showopts_column=$_[0];
}
sub show_opts_g {
    my $show_empty=shift;
    my $name=undef;
    my($old) = select($info_fh) if defined $info_fh && ref($info_fh);
#    $FORMAT_NAME='OptionDescription';
    for (@_) {
        if (!defined $name) { # alternate name ...
            $name=$_;
        } else { # and ref ...
            croak if defined $_ && ! ref $_;
            my $val=&ref_to_string($_,1);
            $val='' unless defined $val;
            if ($name ne "help") {
#                $option=$name;
#                $description=$val;
                write_option_description($name,$val,$showopts_column) if $val ne '' || $show_empty;
#                write;
#                info(" --$name\t\t$val");
            }
            $name=undef;
        }
    }
    select($old);
}
sub show_opts {
    show_opts_g(0,@_);
}
# takes GetOptions style hash-list (alternating name,ref)
sub show_opts_all {
    show_opts_g(1,@_);
}



#use FileHandle;
my $usage_was_called=0;
# for handling END{} blocks gracefully
sub usage_called() {
    my ($u)=@_;
    my $o=$usage_was_called;
    $usage_was_called=$u if defined $u;
    return $o;
}

sub is_zero {
    return ($_[0] ne "" && ! $_[0]);
}

sub is_numeric {
    no warnings 'numeric';
    return ($_[0] != 0 || &is_zero(@_));
}

sub is_numeric_or_ehat {
    my($v)=@_;
    $v=~s/^(e|10)\^//;
    return is_numeric($v);
}

sub quote_nonnumeric {
    return is_numeric(@_) ? $_[0] : defined $_[0] ? qq{"$_[0]"} : '""';
}

my $tabular_usage=0;
sub set_tabular_usage {
    ($tabular_usage)=@_;
}
# usage: ((["number-lines!",\$num"prepend original line number to output"],[]
# ...))
sub usage_from {
    my @args=@_; # copy so closure below works:
    return sub {
        my ($fh)=@_;
        my $isfh=defined $fh && ref($fh);
        my($old) = select(shift) if $isfh;
        print "NAME\n     $0\n\nUSAGE:\n\n";

        for (@args) {
            if (ref($_) eq 'ARRAY') {
                ($option,$description)=usage_for_arg($_);
                write_option_description($option,$description,$tabular_usage);

#                $FORMAT_NAME = "OptionDescription";
#                write;
            } elsif (!ref($_)) {
#                $description=$_;
#                $FORMAT_NAME = "Description";
#                write;
                print "\n";
                write_description($_);
            }
        }
        print "\n";
        $description="(note: unambiguous abbreviations like -h are allowed, and boolean options may be prefixed with 'no', e.g. --flag! can be unset by --noflag)";
        write_description($description);
#        $FORMAT_NAME = "Description";
#        write;
        print "\n";
        print "\nBAD COMMAND LINE OPTION: $getoptions_warn\n" if defined $getoptions_warn;
        print "$fh\n" if defined $fh && !$isfh;
        select($old) if defined $old;
        &usage_called(1);
        exit(1);
    }
}

sub opts_from {
    my @opts=();
    for (@_) {
        if (ref($_) eq 'ARRAY') {
            push @opts,($_->[0],$_->[1]);
        }
    }
    return @opts;
}

#returns usage sub ref then all the GetOptions style opts (for show_opts)
sub getoptions_usage {
    my @opts=opts_from(@_);
    my $required=required_from_usage(@_);
    my $usage=&usage_from(@_);
    my @validate=grep { ref $_ eq 'CODE' } @_;
    push @opts,("help|h"=>sub {$usage->()});
    my $errpre="ERROR: (command line options)";
#    &debug (@opts);
    if (!getoptions_catch(@opts)) {
        $usage->($errpre);
    }
    for my $code (@validate) {
        &$code;
    }
    my @missing=missing_options($required);
    $usage->("$errpre - please supply options:\n@missing") if @missing;
    return ($usage,@opts);
}

#returns usage sub ref then all the GetOptions style opts (for show_opts)
sub getoptions_usage_verbose {
    my $cmdline=&escaped_cmdline;
    my($old) = select($info_fh) if defined $info_fh && ref($info_fh);
    my ($usage,@opts)=&getoptions_usage(@_);
    print "<<<<<< PARAMETERS:\n";
    print "CWD: ",&getcd,"\n";
    print "COMMAND LINE: $cmdline\n\n";
    show_opts(@opts);
    print ">>>>>> PARAMETERS\n\n";
    select($old);
    return ($usage,@opts);
}

sub sort_num {
    return sort { $a <=> $b } @_;
}

my $EPSILON=0.001;

sub set_epsilon {
    my $oldeps=$EPSILON;
    $EPSILON=$_[0] if defined $_[0];
    return $oldeps;
}
sub epsilon_equal {
    my $eps=$_[2];
    $eps=$EPSILON unless defined $eps;
    return abs($_[1]-$_[0])<= $EPSILON;
}

sub epsilon_greater {
    return $_[0]>$_[1]+$EPSILON;
}

sub epsilon_lessthan {
    return $_[1]>$_[0]+$EPSILON;
}

sub relative_epsilon_equal {
    return &relative_error <= $EPSILON;
}

sub relative_error {
    return abs(&relative_change);
}

sub relative_change {
    my ($a,$b,$INF)=@_;
    $INF=$INFINITY unless defined $INF;
    return 0 if !$a && !$b;
    return $INF*$b if (!$a);
    return $INF*$a if (!$b);
    my ($A,$B,$D)=(abs($a),abs($b),$b-$a);
    return $D/($A<$B?$A:$B);
}

#useful for more efficient sorting on val when key is long
sub push_pairs_hash_to_list {
    my ($href,$lref)=@_;
    while (my ($k,$v)=each %$href) {
        push @$lref,[$k,$v];
    }
}

#subst("aBc", qr/([a-z]+)/ => sub { uc $_[1] }, qr/([A-Z]+)/ => sub { lc$_[1] }, ) ;
#subst("aBc", qr/([a-z]+)/ => '$1$1')
sub subst
{
    local $_;
    my $str = shift;
    my $pos = 0;
    my @subs;
    while (@_) {
        push @subs, [ shift, shift ];
    }
    my $res;
    while ($pos < length $str) {
        my (@bplus, @bminus, $best);
        for my $rref (@subs) {
            pos $str = $pos;
            if ($str =~ /\G$rref->[0]/) {
                if (!defined $bplus[0] || $+[0] > $bplus[0]) {
                    @bplus = @+;
                    @bminus = @-;
                    $best = $rref;
                }
            }
        }
        if (@bminus) {
            my $temp = $best->[1];
            if (ref $temp eq 'CODE') {
                $res .= $temp->(map { substr $str, $bminus[$_], $bplus[$_]-$bminus[$_] } 0..$#bminus);
            }
            elsif (not ref $temp) {
                $temp = subst($temp,
                              qr/\\\\/        => sub { '\\' },
                              qr/\\\$/        => sub { '$' },
                              qr/\$(\d+)/     => sub { substr $str, $bminus[$_[1]], $bplus[$_[1]]-$bminus[$_[1]] },
                              qr/\$\{(\d+)\}/ => sub { substr $str, $bminus[$_[1]], $bplus[$_[1]]-$bminus[$_[1]] },
                        );
                $res .= $temp;
            }
            else {
                croak 'Replacements must be strings or coderefs, not ' . ref($temp) . ' refs';
            }
            $pos = $bplus[0];
        }
        else {
            $res .= substr $str, $pos, 1;
            $pos++;
        }
    }
    return $res;
}

my $default_template='<{[()]}>';

#expand_template(121212,1,3,6) => 32622 and an uninit value warning.
sub restore_extract {
    my ($text,$template,@vals)=@_;
    $template=$default_template unless defined $template;
    my $i;
    subst($text,qr/\Q$template\E/,sub { $vals[$i++] });
}

my $mega_suffix_class=qr/[kKmMgGtT]/;

my %mega_suffix=(
                 'k'=>1000,
                 'K'=>1024,
                 'm'=>1000*1000,
                 'M'=>1024*1024,
                 'g'=>1000.0*1000*1000,
                 'G'=>1024.0*1024*1024,
                 't'=>1000.0*1000*1000*1000,
                 'T'=>1024.0*1024*1024*1024,
);

sub from_mega {
    my ($num)=@_;
    if ($num =~ s/($mega_suffix_class)$//) {
        my $suffix=$1;
        $num *= $mega_suffix{$suffix};
    }
    $num;
}

sub first_mega {
    my ($m)=split ' ',$_[0];
    &from_mega($m);
}
sub sort_by_num {
    my ($by,$lref,$reverse)=@_;
    return () unless defined $lref;
    return $reverse ?
        (map { $_->[0] } sort { $b->[1] <=> $a->[1] } map { [$_,$by->($_)] } @$lref) :
        (map { $_->[0] } sort { $a->[1] <=> $b->[1] } map { [$_,$by->($_)] } @$lref);
}
sub sort_by {
    my ($by,$lref,$reverse)=@_;
    return () unless defined $lref;
    return $reverse ?
        map { $_->[0] } sort { $b->[1] cmp $a->[1] } map { [$_,$by->($_)] } $lref :
        map { $_->[0] } sort { $a->[1] cmp $b->[1] } map { [$_,$by->($_)] } $lref;
}

#base10 means 10^3k, base2 means 2^10k
sub to_mega {
    my ($num,$base10,$prec)=@_;
    $prec=4 unless $prec;
    if (is_numeric($num)) {
        my $last=$num;
        for my $s qw(T G M K) {
            my $unit=$base10 ? lc($s) : $s;
            my $div=$mega_suffix{$unit};
            my $amount=$num/$div;
            return real_prec($amount,$prec).$unit if $amount >= 1;
        }
    }
    return $num;
}

my $num_match=qr/[+\-]?(?:\.\d+|\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?)/;
my $loose_num_match=qr/($num_match$mega_suffix_class?)(?=sec|s|)\b/;
my $integer_match=qr/\b[+\-]?\d+\b/;
my $natural_match=qr/\b\+?\d+\b/;
sub integer_re {
    return $integer_match;
}

sub number_re {
    return $num_match;
}

sub loose_number_re {
 return $loose_num_match;
}

# number is loosely defined, with -+.^eE getting stolen unless protected by
# word boundaries and
### NO LONGER: e^number as a legal alternative (log scale) rep.
sub extract_numbers {
    my ($text,$template,$pattern)=@_;
    $pattern=$loose_num_match unless defined $pattern;
    $template=$default_template unless defined $template;
    my @nums=();
    while ($text=~s/$loose_num_match/$template/) {
#FIXME: replace only if really numeric
        push @nums,from_mega($1);
    }
    superchomp(\$text);
    return ($text,@nums);
}

my %summary_list_n_sum_max_min;

sub get_number_summary {
    return \%summary_list_n_sum_max_min;
}

#list: # of events,sums,(TODO:maxes)
sub add_to_n_list {
    my ($ppOld,@nums)=@_;
    my $pOld=$$ppOld;
    if (!defined $pOld) {
        $$ppOld=[1,[@nums],[@nums],[@nums]];
    } else {
        $pOld->[0]++;
        my $pSum=$pOld->[1];
        my $pMax=$pOld->[2];
        my $pMin=$pOld->[3];
        local $_;
        for (0..$#nums) {
            $pSum->[$_]+=$nums[$_];
            $pMax->[$_]=$nums[$_] unless $pMax->[$_] > $nums[$_];
            $pMin->[$_]=$nums[$_] unless $pMin->[$_] < $nums[$_];
        }
    }
}

my $ln_of_10=log(10);
#fixme: use POSIX will define this already.
no warnings 'redefine';
#eval "use List::Util;1"
#use List::Util qw(max min);
sub max {
    my $x=shift;
    local $_;
    for (@_) {
        $x=$_ if $x<$_;
    }
    $x
}

sub min {
    my $x=shift;
    local $_;
    for (@_) {
        $x=$_ if $x>$_;
    }
    $x
}

sub log10 {
        #/$ln_of_10; #slower
    log($_[0])*0.434294481903252
}
use warnings 'redefine';

sub log10_to_ln {
    $_[0]*2.302585092994; #*$ln_of_10
}

sub ln_to_log10 {
    $_[0]*0.434294481903252 #/$ln_of_10
}

sub exp10 {
    my $n=shift;
    exp($n*2.302585092994) #*$ln_of_10);
}
sub intlog10 {
    return int &log10; # same as length int $_[0]
}

sub significant_digits {
    my ($n,$m)=@_;
    my $h=$n;
    $m=10000 unless defined $m;
    chomp $h;
    $h=~s/^0*//;
    $h=~s/\.//g;
    $h=~s/e[-+]?\d+$//;
    $h=~s/0000*\d?\d?\d?$//;
    $h=~s/9999*?\d?\d?\d?$//;
#    &debug("significant digits of $n -> $h (limit: $m)",length($h));
    return min(length($h),$m);
}
my $NUMBER_SUMMARY_PREC=13;

sub set_number_summary_prec {
    $NUMBER_SUMMARY_PREC=defined $_[0] ? $_[0] : 7;
}

sub print_number_summary {
    return unless scalar keys %summary_list_n_sum_max_min;
    my ($fh,$show_sums,$no_header,$avgonly)=@_;
    &debug;
    $fh=$info_fh unless defined $fh;
    my($old) = select($fh);
    my @temps=sort keys %summary_list_n_sum_max_min;
    my $avgorsum=$show_sums?"sum":"avg";
    unless ($no_header) {
        print "SUMMARY OF NUMBERS - \"$avgorsum(N=<n-occurences>): text-with-".
          ($show_sums ? "[sum (avg)]":"[min/avg/max]").
            "-replacing-numbers\":\n";
    }
        for (@temps) {
        my ($n,$pSum,$pMax,$pMin)=@{$summary_list_n_sum_max_min{$_}};
        my $restore_avg=restore_extract($_,undef,
          map {
            my ($min,$sum,$max)=($pMin->[$_],$pSum->[$_],$pMax->[$_]);
            my $avg=$sum/$n;
            my $ndigits=min($NUMBER_SUMMARY_PREC,max(significant_digits($min),significant_digits($sum),significant_digits($max))+max(1,intlog10($n)));
            #&debug('number summary #digits',$ndigits,$min,$sum,$max);
            my ($pmin,$pavg,$pmax,$psum)=map {real_prec($_,$ndigits) } ($min,$avg,$max,$sum);
            $show_sums ? ("[$psum ($pavg)]") : ($min==$max ? $min : ($avgonly ? $pavg : "[$pmin/$pavg/$pmax]"))}
          (0..$#$pSum));
        print "$avgorsum(N=$n): $restore_avg\n";
    }
    select($old);
}

sub all_summary {
    &info_summary;
    &print_number_summary;
}

sub remember_numbers {
    my ($text,@nums)=@_;
#    &debug("remember",$text,@nums);
    add_to_n_list(\$summary_list_n_sum_max_min{$text},@nums);
}

#generalize what's in [brackets]
sub remember_numbers_gen {
    &remember_numbers;
    my $o=$_[0];
    $_[0]=generalize_brackets($o);
    &remember_numbers unless $_[0] eq $o;
}


sub log_numbers {
    #    &debug("log_number_summary",@_);
    remember_numbers(&extract_numbers_pre_post);
}

sub extract_numbers_pre_post {
    my ($t,$pre,$post)=@_;
    $pre='' unless $pre;
    $post='' unless $post;
    chomp $t;
    chomp $pre;
    chomp $post;
    my ($s,@n)=extract_numbers($t);
    return ($pre.$s.$post,@n);
}



sub generalize_brackets {
    my ($text)=@_;
    $text =~ s/\[[^\[\]]*\]/[]/g;
    return $text;
}

#returns mapped generalized list, or empty list if no change
sub generalize_list_brackets {
    my $n=0;
    my @ret=map {
        my $g=generalize_brackets($_);
        ++$n unless $g eq $_;
        $g
    } @_;
    return $n ? @ret : ();
}


sub log_numbers_gen {
    &log_numbers;
    &log_numbers_gen_only;
}

#generalize what's in [brackets]
sub log_numbers_gen_only {
    my @l=&generalize_list_brackets;
    log_numbers(@l) if scalar @l;
}


sub push_hash_list {
    my ($ppOld,@nums)=@_;
    if (!defined $ppOld) {
        $$ppOld=[@nums];
    } else {
        push @$$ppOld,@nums;
    }
}

#regexp: $1 = fieldname, $2 = {{{attr-with-spaces}}} or attr, $3 =
#attr-with-spaces, $4 = attr
sub getfield_regexp {
    my ($fieldname)=@_;
    $fieldname = defined $fieldname ? qr/\Q$fieldname\E/ : qr/\S+/;
    return qr/\b(\Q$fieldname\E)=({{{(.*?)}}}|([^{]\S*))\b/;
}

sub getfield {
  my ($field,$line)=@_;
  if ($line =~ /\Q$field\E=(?:{{{(.*?)}}}|(\S*))/) {
      return (defined $1) ? $1 : $2;
  } else {
      return undef;
  }
}

sub getfields {
    my ($href,$line)=@_;
    while ($line =~ /\b([\w\-]+)=({{{(.*?)}}}|([^{]\S*))/go) {
        my $key=$1;
        $href->{$key}=defined $3 ? $3 : $4;
    }
}

sub single_quote_interpolate {
    my ($string)=@_;
    my $quoted=q{"$string"};
    my $evaled=eval $quoted;
    return $evaled ? $evaled : $string;
}

sub double_quote_interpolate {
    my ($string)=@_;
    my $quoted=qq{"$string"};
    my $evaled=eval $quoted;
    return $evaled ? $evaled : $string;
}

#if @opts=(["a",\$a],...)
#usage: expand_opts(\@opts,["before","after"])
sub expand_opts {
    my $ref_opts_usage=shift;
      for (@$ref_opts_usage) {
          if (ref($_) eq 'ARRAY') {
              my ($name,$ref)=@$_;
              if (ref($ref) eq 'SCALAR' && defined $$ref) {
                  my $old_val=$$ref;
                  for (@_) {
                      $$ref =~ s/\Q$_->[0]\E/$_->[1]/g;
                  }
                  info("Expanded $name from $old_val to $$ref") unless $old_val eq $$ref;
              }
          }
      }
}

sub system_postmortem {
    if ($? == -1) {
        return "failed to execute: $!";
    } elsif ($? & 127) {
        return sprintf("child croakd with signal %d, %s coredump",
          ($? & 127),  ($? & 128) ? 'with' : 'without');
    } elsif ($? >> 8) {
        return sprintf("child exited with value %d\n", $? >> 8);
    }
    return '';
}

sub system_postmortem_assert {
    my $result=&system_postmortem;
    croak $result if $result;
}

sub send_email {
    my ($text,$subject,@to)=@_;
    local(*EMAIL);
    open(EMAIL,'|mail -s "'.$subject.'" '.join(' ',@to)) or croak "Couldn't send email - $!: $?";
    print EMAIL $text,"\n";
    close EMAIL;
}

sub symlink_update {
    my ($source,$dest,$resolve,$force)=@_;
    $dest="$dest/".mybasename($source) if (!readlink($dest) || $dest =~ m|/$|o) && -d $dest;
    unlink $dest if readlink($dest) || ($force && -f $dest);
    $source=&expand_symlink($source) || $source if $resolve;
    symlink($source,$dest);
}

sub symlink_force {
    my ($source,$dest,$resolve)=@_;
    symlink_update($source,$dest,$resolve,1);
}

sub read_srilm_unigrams {
    my ($fh,$hashref)=@_;
    local $_;
    my $uni=0;
    while(<$fh>) {
        if (/^\\1-grams:$/) {
            &debug("expecting 1-grams: $. $_");
            $uni=1;
        } elsif (/^\\.*:$/) {
            &debug("end of 1-grams: $. $_");
            return;
        } elsif ($uni) {
#            &debug("trying to parse unigram",$_);
            if (/^(\S+)\s+(\S+)/) {
                my ($prob,$word)=($1,$2);
                $hashref->{$word}=$prob;
#                &debug("parsed srilm unigram",$word,$prob);
            }
        }
    }
}


my $default_precision=13;
sub set_default_precision {
    $default_precision=defined $_[0] ? $_[0] : 6;
    &set_number_summary_prec;
}

sub real_prec {
    my ($n,$prec)=@_;
    $prec=$default_precision unless defined $prec;
    sprintf("%.${prec}g",$n);
}

sub percent_prec {
    my ($frac,$prec)=@_;
    $prec=2 unless defined $prec;
    return real_prec($frac*100,$prec)."%";
}

sub log10_to_ln_prec {
    my ($log10,$prec)=@_;
    return real_prec(log10_to_ln($log10));
}

#prob -> prob
sub log10_to_ehat {
    return "e^".&log10_to_ln_prec;
}

#cost -> prob
sub real_to_ehat {
    return "e^".real_prec(&getln);
}

#take a number and return it unmodified (a cost). or see 10^-cost or e^-c and return cost or ln_to_log10(
sub to_cost {
    local ($_)=@_;
    $_=-ln_to_log10($1) if /^e\^(.*)/;
    $_=-$1 if /^10\^(.*)/;
    s/^\+//;
    $_
}


# nearest_two_linear($x,\@B) returns ($i,$a) such that $B[$i]*(1-$a) + $B[$i+1]*$a == $x
sub nearest_two_linear {
    my ($x,$R)=@_;
#    &debug("nearest_two_linear",$x,$R->[0]);
    return (0,0) if ($x < $R->[0]);
    my $LAST=$#$R; # last index
    for (0..$LAST-1) { #FIXME (in theory): binary search, blah blah - not using
                    #large lists anyway
        my $b=$R->[$_+1];
#        &debug("if $x <= $b...");
        if ($x <= $b) {
            my $a=$R->[$_];
#            &debug(" $x between $a and $b at index $_");
            return ($_,($x-$a)/($b-$a)); # fraction of the way from b to c (0
                                         # means you're on $_
        }
    }
    return ($LAST,0),
}

sub escape_3brackets {
    local($_)=@_;
    return "{{{$_}}}" if (/\s/);
    return $_;
}

sub read_hash {
    my ($file,$href,$invhref)=@_;
    my $fh=&openz($file);
    local $_;
    while(<$fh>) {
        chomp;
        my ($f1,$f2)=split;
        next unless defined $f2;
        $href->{$f1}=$f2 if defined $href;
        $invhref->{$f2}=$f1 if defined $invhref;
    }
    close $fh;
}

sub write_hash {
    my ($fh,$href)=@_;
    $fh=$info_fh unless $fh;
    for (sort keys %$href) {
        print $fh "$_\t$href->{$_}\n";
    }
}

sub hash_lookup_default {
    my ($hashref,$key,$default)=@_;
    return (exists $hashref->{$key}) ? $hashref->{$key} : $default;
}

#returns old value (pre-inc), i.e. 0 first time key is inced
sub inc_hash {
    my ($hashref,$key)=@_;
    my $ret=$hashref->{$key};
    no warnings 'numeric';
    ++$hashref->{$key}; #=$ret ? $ret +1 : 1; # simpler: ++$h{k}
    return $ret;
}

sub split_text {
    my ($text,$split,$max)=@_;
    $max=0 unless defined $max;
    return (split($split,$text,$max)) if (defined $split);
    return (split(' ',$text,$max));
}

sub count_words {
    return scalar &split_text;
}

sub capture_3brackets_re {
    '{{{(.*?)}}}';
}

my %w2id;
my @id2w;

sub haveword {
    return exists $w2id{$_[0]};
}

sub word2id {
    my ($word)=@_;
    return $w2id{$word} if exists $w2id{$word};
#    fatal("word2id: unknown word $word") unless $add;
    push @id2w,$word;
    $w2id{$word}=$#id2w;
}


sub id2word {
    return $id2w[$_[0]];
}

sub dot_product_array_hash {
    my ($aref,$href)=@_;
    my $sum=0;
    while (my ($k,$v)=each %{$href}) {
        $sum+=$v*$aref->[$k] if defined $aref->[$k];
    }
    $sum;
}

sub push_ref {
    my ($lref_ref,@push)=@_;
    my $lref=$$lref_ref;
    if (defined $lref and ref $lref eq 'ARRAY') {
        push @$lref,@push;
    } else {
        $$lref_ref=[@push];
    }
}

sub at_grow_default {
    my ($aref,$index,$default,$v)=@_;
    $default=0 unless defined $default;
    my $n=$#$aref;
    while ($index > $n) {
        push @{$aref},$default;
        ++$n;
    }
    $aref->[$index]=$v if defined($v);
    \$aref->[$index]
}
sub at_default {
    my ($aref,$index,$default)=@_;
    $default=[] unless defined $default;
    my $cellref;
    if (ref $aref eq 'ARRAY') {
        $cellref=\$aref->[$index];
    } else {
        croak unless ref $aref eq 'HASH';
        $cellref=\$aref->{$index};
    }
    $$cellref=$default unless defined $$cellref;
    return $$cellref;
}

sub replace_span {
    my ($str,$a,$b,$with)=@_;
    substr($str,$a,$b-$a,$with);
    return $str;
}

sub search_replace {
    my ($sref,$from,$to)=@_;
    $$sref =~ s/\Q$from\E/$to/g;
}

sub search_replace_b {
    my ($sref,$from,$to)=@_;
    $$sref =~ s/\b\Q$from\E\b/$to/g;
}

sub delta_to_last_weekday {
#dow: 0(sun)...6(sat)
    my ($now,$last)=@_;
    $last-=7 if ($last >= $now);
    return $last-$now; # returns -1 to -7
}

sub push_no_dup {
    my ($lref,$x)=@_;
    if (scalar @$lref == 0 || $lref->[-1] ne $x) {
        push @$lref,$x;
    }
}

sub to_tempfile {
    my ($text,$ftemp)=@_;
    $ftemp="tmp" unless $ftemp;
    $ftemp .= ".XXXXXX" unless $ftemp =~ /XXXXXX/;
    my ($fh,$fname)=tempfile($ftemp);
    print $fh $text;
    close $fh;
    $fname;
}

# args: b=ln(base),x_1,x_2 return log_base (sum_i ( base ^ x[i]))
sub logadd_b {
    my ($b,$x1,$x2)=@_;
    my $d=($x1-$x2);
    my $BIGDIFF=36/$b;
    &debug($x1,$x2,$d,$BIGDIFF,$b*($x2-$x1));
    return $x1 if ($d>$BIGDIFF);
    return $x2 if ($d<-$BIGDIFF);
    if ($d<0) {
        my $t=$x1;$x1=$x2;$x2=$t;
    }
    # x1 > x2
    $x1+log(1+exp($b*($x2-$x1)))/$b;
}

sub logadd_b_quick {
    my ($b,$x1,$x2)=@_;
    return log(exp($b*$x1)+exp($b*$x2))/$b;
}

sub logadd_base {
    logadd_b(log($_[0]),$_[1],$_[2]);
}

sub logadd_base_quick {
    logadd_b_quick(log($_[0]),$_[1],$_[2]);
}


my $tailn=5;
sub preview_files {
    my $n=shift;
    $n=$tailn unless defined $n;
    my $fn=join(' ',map {escape_shell($_)} @_);
    my $cmd="tail -n $n $fn";
    my $text=`$cmd`;
    debug($cmd,$text);
    $text
}

sub scratchdir {
    my $tmpdir = $ENV{'MY_TMP'} ||
        $ENV{TMP} ||
        $ENV{TEMP} ||
        $ENV{TMPDIR} ||
        File::Spec->tmpdir() ||
        '/tmp';
    $tmpdir = '/scratch' if -d '/scratch';
    $tmpdir
}

sub timestamp(;$) {
    # returns: ISO 8601 like (not quite) formatted date stamp space time stamp
    # warning: somewhat fudges the standard concerning zone and separator
    my @x = localtime( defined $_[0] ? shift() : time() );
    sprintf( "%04u-%02u-%02u %02u:%02u:%02u",
	     $x[5]+1900, $x[4]+1, @x[3,2,1,0] );
}

sub multi_args($@) {
    local $_;
    my $p=shift;
    my @r;
    push @r,$p,$_ for (@_);
    @r
}

1

