#!/usr/bin/env perl
use Getopt::Long;
use POSIX;
use strict;
use warnings;

### script info ##################################################
use File::Basename;
my $scriptdir; # location of script
my $scriptname; # filename of script
my $BLOBS;
BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
    push @INC, $scriptdir;
    $ENV{BLOBS}='/home/nlg-01/blobs' unless exists $ENV{BLOBS};
    $ENV{BLOBS}="$ENV{HOME}/blobs" unless -d $ENV{BLOBS};
    $BLOBS=$ENV{BLOBS};
    my $libgraehl="$BLOBS/libgraehl/unstable";
    push @INC,$libgraehl if -d $libgraehl;
}

require "libgraehl.pl";
###


#&debug(escape_mathsym('a<= b+<> != 23$'));

my $fontdir='/home/nlg-03/mt-apps/texlive/texmf-local/fonts/truetype/';
my %fontdir;
my %font;

$fontdir{chi}="$fontdir/bitstream/"; #sorry - you'll have to change this is in the __DATA_ section instead
$font{chi}="cyberbit";
$fontdir{ara}=$fontdir;
$font{ara}="ScheherazadeRegOT";

sub usage {
    print STDERR "\n";
    print STDERR "help -- print this message\n";
    print STDERR "d    -- driver file (format:  [latex anno |] line no)\n";
    print STDERR "a    -- latex anno file (will override driver anno)\n";
    print STDERR "e    -- etree file\n";
    print STDERR "f    -- fstring file\n";
    print STDERR "a1   -- first alignment file\n";
    print STDERR "a2   -- second alignment file\n";
    print STDERR "o    -- out file\n";
    print STDERR "l    -- language of fstring (default: eng, other options: chi, ara)\n";
    print STDERR "t    -- tight tree format\n";
    print STDERR "i p  -- -l p.e-parse -f p.f -a1 p.a -o p.tex -a p.info\n";
    print STDERR "c m  -- caption 'm'\n";
    print STDERR "debug -- turn on debug messages\n";
    print STDERR "s    -- dim identity n-n alignment links, add spring lines for missing n-n links\n";
    print STDERR "k    -- keep .tex.utf8 for chinese\n";
    print STDERR "pagen p -- set page number to p instead of 1\n";
    print STDERR "[no]x -- x: use xelatex (unicode, slower). nox: use latex (GB2312 for chinese)\n";
    exit;
}
my $DEBUG=0;
my $linewidth=".1mm"; # ".02mm"

sub set_debug { $DEBUG=1; }

sub debug_print {
    print STDERR @_ if $DEBUG;
}
my $pagen=1;
my $jarpath=".";
my $driverfile;
my $annofile;
my $etreefile;
my $fstringfile;
my $alfile1;
my $alfile2;
my $outfile;
my $lang;
my $tight;
my $inpre;
my $checkid;
my $caption='';
my $keeputf8;
my $xetex=1;

GetOptions(  "help"       => \&usage
	   , "jarpath:s"  => \$jarpath
             ,"s!"=>\$checkid
	   , "d=s"  => \$driverfile
	   , "a=s"  => \$annofile
	   , "e=s"    => \$etreefile
           , "f=s"  => \$fstringfile
           , "a1=s"    => \$alfile1
           , "a2=s"    => \$alfile2
           , "o=s"      => \$outfile
	   , "l=s"  => \$lang
	   , "t!",   => \$tight
       , "i=s" => \$inpre
             ,"c=s" => \$caption
           , "debug"      => \&set_debug
             , "xetex!" => \$xetex
             ,"k!"=>\$keeputf8
             , "pagenumber=i" => \$pagen
    );
#&debug("xetex=$xetex");
my $xsuf=$xetex?'xe':'la';
my $langx=$lang.$xsuf;
my %accept;
$accept{all}=1;
$accept{$lang}=1;
$accept{$langx}=1;
$accept{$xsuf}=1;
my %fword;
$fword{chila}=['\begin{CJK}{GB}{song}','\end{CJK}'];
$fword{chixe}=['{\zhfont ',' }'];
$fword{arala}=['\RL{ ',' }'];
$fword{araxe}=['\RL{\arabicfont ',' }'];
$fword{engla}=$fword{engxe}=['{ ',' }'];
my ($preword,$postword)=('\fword{','}');
#@{$fword{$langx}};

my $caption_tex=pretty_latex($caption);
my $caption_text=$caption?" for $caption_tex":'';
my $caption_cap=$caption?"\\caption{$caption_tex}\n":'';
#$caption_cap='';
my $caption_foot=$caption?"\\chead{$caption_tex}\n":'';
if ($inpre) {
    $alfile1="$inpre.a";
    $outfile="$inpre.tex";
    $etreefile="$inpre.e-parse";
    $fstringfile="$inpre.f";
    my $a="$inpre.info";
    $annofile=$a if -f $a;
}
if ($lang) {
    debug_print("language specified = $lang\n");
    if ($lang eq "chi") {
    } elsif ($lang eq "ara") {
    } elsif ($lang ne "eng") {
        warn "Unknown language '$lang', defaulting to english\n";
        $lang='eng';
    }
} else {
    debug_print("language specified = none (default to english)\n");
    $lang='eng';
}

sub readlines($\@) {
    my $fn = shift;
    my $slinesref = shift;
    my %out;

    my @sorted_slines = sort { $a <=> $b } @{$slinesref};

    open F, $fn or die "Can't open $fn: $!\n";
    my $next = shift @sorted_slines;
    #print STDERR "next = $next\n";
  OUTER:
    while (<F>) {
	while ( $. == $next ) {
	    $out{$.} = $_;
	    $next = shift @sorted_slines || last OUTER;
	    #print STDERR "next = $next\n";
	}
    }
    close F;
    warn "# there is stuff remaining!" if @sorted_slines;

    my @out;
    for my $i ( @{$slinesref} ) {
	push(@out, $out{$i} );
	#print STDERR "outline $i\n";
    }

    return @out;
}

die "$outfile should end in .tex" unless ($outfile =~ /\.tex$/);
my $outdir = `dirname $outfile`;
chomp $outdir;
my $outbase = $outfile;
$outbase =~ s/\.tex$//;
debug_print "Base is $outbase\n";
my $dvifile = "$outbase.dvi";
my $psfile = "$outbase.ps";

my @sourcelines;
my @desc;
my @estrings;
if ($driverfile) {
    open D, $driverfile or die "Can't open $driverfile: $!\n";
    while (<D>) {
	if (/^\s*((.*)\s+\|\s+)?\s*(\d+)\s*$/) {
	    #print STDERR "desc = $2, sl = $3\n";
	    push @desc, $2;  # optional -- will be overriden by annofile
            push @sourcelines, int($3);
	}
    }

    @estrings = readlines($etreefile, @sourcelines) or die "Can't open $etreefile: $!\n";
} else {
    open E, $etreefile or die "Can't open $etreefile: $!\n";
    @estrings = <E>;
    close E;
}
sub escape_tree {
    local ($_)=@_;
    chomp;
    # strip tildes and numbers
    s/~\d+~\d+ [-\d\.]+//g;
    $_=pretty_latex($_);
    # dot every nonterm
    s/(\(\s*)(\S)/$1\.$2/g;
    # replace parens with squares
    s/\(/ \[ /g;
    s/\)/ \] /g;
    $_
}
@estrings = map {escape_tree($_)} @estrings;
# wrap terminals with indices
my $lineid=0;
my @elens=map { 0 } @estrings;
foreach my $string (@estrings) {
    my $wordid=0;
    while ($string =~ / ([^\s\}\{]+) \]/) {
	my $word = $1;
	my $match = " $word ]";
	# make match perl safe
	$match =~ s/([\\\|\(\)\[\{\^\$\*\+\?\.])/\\$1/g;
	# bold any triple prestarred words -- for highlighting
	$word =~ s/\*\*\*(\S+)/\\textbf{$1}/;
	my $repl = " \\rnode{s$lineid"."e$wordid}{$word} ]";
	$string =~ s/$match/$repl/;
	$wordid++;
	debug_print "String to $string\n";
    }
    $elens[$lineid]=$wordid;
    $lineid++;
}

my @fstrings;
if ($driverfile) {
    @fstrings = readlines($fstringfile, @sourcelines) or die "Can't open $fstringfile: $!\n";
} else {
    open F, $fstringfile or die "Can't open $fstringfile: $!\n";
    @fstrings = <F>;
    close F;
}

if ($annofile) {
    if ($driverfile) {
	@desc = readlines($annofile, @sourcelines) or die "Can't open $annofile: $!\n";
    } else {
	open F, $annofile or die "Can't open $annofile: $!\n";
	@desc = <F>;
	close F;
    }
}

$lineid=0;
# put labels on f
my @modfs = ();
my @flens=map {0} @fstrings;
foreach my $string (@fstrings) {
    my $wordid=0;
    my @newf = ();
    foreach my $word (split /\s+/, $string) {
	# make latex safe
        $word=pretty_latex($word);
#	$word =~ s/([\$_%\{\}&])/\\$1/g;
	# bold any triple prestarred things -- for highlighting
#	while ($word =~ s/^\*\*\*(\S+)$/\\textbf{$1}/g) {};
	push @newf, "\\rnode{s$lineid"."f$wordid}{$preword$word$postword}";
	$wordid++;
    }
    $flens[$lineid]=$wordid;
    push @modfs, join ' \hspace{0.3cm}', @newf;
    $lineid++;
}

my @aligns1;
if ($driverfile) {
    @aligns1 = readlines($alfile1, @sourcelines) or die "Can't open $alfile1: $!\n";
} else {
    open A1, $alfile1 or die "Can't open $alfile1: $!\n";
    @aligns1 = <A1>;
    close A1;
}
my @aligns2;
if ($alfile2) {
    if ($driverfile) {
	@aligns2 = readlines($alfile2, @sourcelines) or die "Can't open $alfile2: $!\n";
    } else {
	open A2, $alfile2 or die "Can't open $alfile2: $!\n";
	@aligns2 = <A2>;
	close A2;
    }
}

my @printals = ();
# calculate the alignments and make the commands
for (my $i = 0; $i < @aligns1; $i++) {
    my @set = ();
    my $al1 = $aligns1[$i];
    my (%a1s, %a2s);
    map {/(\d+)-(\d+)/; $a1s{$1}{$2} = 1 } split /\s+/, $al1;
    if ($alfile2) {
	my $al2 = $aligns2[$i];
	map {/(\d+)-(\d+)/; $a2s{$1}{$2} = 1 } split /\s+/, $al2;
    }
    # find all arcs in both alignments and in al1 only
    foreach my $e (keys %a1s) {
	foreach my $f (keys %{$a1s{$e}}) {
        my $dim=$checkid && $e==$f;
        my $ina2=$alfile2 && !exists $a2s{$e}{$f};
        my @styles;
        push @styles,'linestyle=dashed' if $ina2;
        my $pre="\\ncline[".join(',',"linewidth=$linewidth",@styles)."]{-}";
	    my $item="{s$i"."e$e}{s$i"."f$f}";
        push @set, $pre.$item unless $dim;
	}
    }
    # print missing id links if dimid
    if ($checkid) {
        my ($ne,$nf)=($elens[$i],$flens[$i]);
        my $n=$ne>$nf?$nf:$ne;
        for (0..$n) {
            if (!exists $a1s{$_}{$_}) {
                my $item="\\nczigzag[fillstyle=none,linewidth=$linewidth,coilwidth=1mm,linearc=.01]{o-o}{s$i"."e$_}{s$i"."f$_}";
                push @set, $item;
            }
        }
    }
    # find all arcs in al2 only
    if ($alfile2) {
	foreach my $e (keys %a2s) {
	    foreach my $f (keys %{$a2s{$e}}) {
		next if (exists $a1s{$e}{$f});
		my $item="\\nczigzag[fillstyle=none,linewidth=$linewidth,coilwidth=1mm,linearc=.01]{o-o}{s$i"."e$e}{s$i"."f$f}";
		push @set, $item;
	    }
	}
    }
    push @printals, join "\n", @set;
}

debug_print "English 1 is:\n";
debug_print "$estrings[0]\n";
debug_print "Foreign 1 is:\n";
debug_print "$modfs[0]\n";
debug_print "Align 1 is:\n";
debug_print "$printals[0]\n";

my $togb=$lang eq "chi" && !$xetex;
# preparatory stuff
if ($togb) {
    open OUT, ">$outfile.utf8" or die "Can't open $outfile.utf8: $!\n";
} else {
    # don't need to write utf-8 for english (hopefully)
    open OUT, ">$outfile" or die "Can't open $outfile: $!\n";
}


my $prefix;
while ( <DATA> ) {
    ($prefix,$_) = split /:/, $_, 2;
    my $lp=length($prefix);
    print OUT if $accept{$prefix};
}

print OUT "\\setcounter{page}{$pagen}\n" if $pagen;

if ($tight) {
  print OUT "\\psset{levelsep=36pt,nodesep=2pt,treesep=24pt,treefit=tight}\n";
} else {
  #print OUT "\\psset{nodesep=0.1cm}\n";
  print OUT "\\psset{levelsep=36pt,nodesep=2pt,treesep=24pt,treefit=loose}\n";
}

#print OUT "\n\\begin{CJK}{GB}{song}\\end{CJK}\n" if $lang eq "chi";
print OUT $caption_foot;

if (@estrings == 0) {
    print OUT "No (tree,string,[alignment]) lines provided$caption_text.\n";
}
for (my $i = 0; $i < @estrings; $i++) {
    my $tree = $estrings[$i];
    my $fstring = $modfs[$i];
    my $als = $printals[$i];
    my $sl = $sourcelines[$i];
    my $desc = $desc[$i];
    print OUT "\\newpage\n\n" if $i > 0;
    print OUT "Training Data line = $sl\n\n" if $sl;
    print OUT pretty_latex($desc),"\n\n" if $desc;
    print OUT "\\begin{table}\n" if $caption_cap;
    print OUT "\\begin{tabular}{c}\n";
    print OUT "\\shrinkboxtokeepaspect(9.5in,5.5in){\\Tree $tree} \\\\ \n\n";
    print OUT "\\vspace{1in} \\\\ \n\n";
    print OUT "\\shrinkboxtokeepaspect(9.5in,1in){$fstring}\n";
    print OUT $caption_cap;
    print OUT "\\end{tabular}\n";
    print OUT "\\end{table}\n" if $caption_cap;
    print OUT "$als\n";
}

print OUT "\\end{document}\n";

close OUT;
# convert utf-8 to something LaTex can use
if ($togb) {
#    `/home/nlg-01/contrib/local/j2sdk1.4.2_03/bin/java -jar /home/rcf-78/sdeneefe/bin/java/encoding_converter.jar $outfile.utf8 - UTF-8 GB2312 > $outfile`;
    copy_file_code("$outfile.utf8",":utf8",$outfile,":encoding(euc-cn)");
    unlink("$outfile.utf8") unless $keeputf8;
}

#
#require geometry 5.1 or greater - http://latex-community.org/?option=com_content&view=article&id=351:geometry-package-updated&catid=44:news-latex&Itemid=111&fontstyle=f-smaller - or workaround http://www.tug.org/pipermail/tex-live/2009-December/023888.html
__DATA__
all:\documentclass[letterpaper]{article}
all:\usepackage{fancyhdr}
la:\usepackage[left=12mm,right=12mm,top=20mm,bottom=12mm,landscape,dvips]{geometry}
xe:\usepackage[left=12mm,right=12mm,top=20mm,bottom=12mm,landscape,xetex]{geometry}
all:\usepackage{times,tabularx,supertabular}
all:\usepackage{qtree,pst-tree,pstricks,pst-node,pst-coil}
all:\include{pst-qtree}
xe:\usepackage{fontspec}
chila:\usepackage{CJK}
chixe:\newfontfamily{\zhfont}[Scale=1.2,Path=/home/nlg-03/mt-apps/texlive/texmf-local/fonts/truetype/bitstream/]{cyberbit}
araxe:\usepackage{arabxetex}
araxe:\newfontfamily{\arabicfont}[Script=Arabic,Scale=1.5,Path=/home/nlg-03/mt-apps/texlive/texmf-local/fonts/truetype/]{arabic}
arala:\usepackage{arabtex,atrans,nashbf,utf8}
arala:\usepackage{times}
all:\newcommand{\fword}[1]{
chila:\begin{CJK}{GB}{song}#1\end{CJK}
arala:\RL{#1}
chixe:{\zhfont #1}
araxe:\RL{\arabicfont #1}
all:}
all:\makeatletter
all:\def\shrinkboxtokeepaspect(#1,#2)#3{\shrinkboxto(#1,0){\shrinkboxto(0,#2){#3}}}
all:\def\shrinkboxto(#1,#2){\pst@makebox{\@shrinkboxto(#1,#2)}}
all:\def\@shrinkboxto(#1,#2){%
all:\begingroup
all:\pssetlength\pst@dima{#1}%
all:\pssetlength\pst@dimb{#2}%
all:\ifdim\pst@dima=\z@\else
all:\pst@divide{\pst@dima}{\wd\pst@hbox}\pst@tempc
all:\ifdim\pst@tempc pt > 1 pt
all:\def\pst@tempc{1 }%
all:\fi
all:\edef\pst@tempc{\pst@tempc\space}%
all:\fi
all:\ifdim\pst@dimb=\z@
all:\ifdim\pst@dima=\z@
all:\@pstrickserr{%
all:\string\shrinkboxto\space dimensions cannot both be zero}\@ehpa
all:\def\pst@tempa{}%
all:\def\pst@tempc{1 }%
all:\def\pst@tempd{1 }%
all:\else
all:\let\pst@tempd\pst@tempc
all:\fi
all:\else
all:\pst@dimc=\ht\pst@hbox
all:\advance\pst@dimc\dp\pst@hbox
all:\pst@divide{\pst@dimb}{\pst@dimc}\pst@tempd
all:\ifdim\pst@tempd pt > 1 pt
all:\def\pst@tempd{1 }%
all:\fi
all:\edef\pst@tempd{\pst@tempd\space}%
all:\ifdim\pst@dima=\z@ \let\pst@tempc\pst@tempd \fi
all:\fi
all:\edef\pst@tempa{\pst@tempc \pst@tempd scale }%
all:\@@scalebox
all:\endgroup}
all:\pslongbox{Shrinkboxto}{\shrinkboxto}
all:
all:\DeclareRobustCommand*\textsubscript[1]{%
all:{\m@th\ensuremath{_{\mbox{\fontsize\sf@size\z@\selectfont#1}}}}}
all:\makeatother
all:
all:\pagestyle{fancy}
all:\newcommand{\sentence}[1]{%
all:  \section*{#1}%
all:  \markboth{{#1}\hfill {$heading}\hfill}{{#1}\hfill {$heading}\hfill}}
all:
all:\begin{document}
all:\qtreecenterfalse
arala:
arala:\setcode{utf8}
arala:\setarab
