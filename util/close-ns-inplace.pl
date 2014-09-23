#!/usr/bin/perl -i~

my @lines;

while (<>) {
    push @lines, $_;
    if (eof(ARGV)) {
        $i=$#lines;
        $m=$i - 10;
        $m = 0 if ($m < 0);
        while($i > $m && $lines[$i] =~ /^#endif|^\s*$/) { --$i; }
        if ($lines[$i] =~ s#^(}+)//(ns|.*namespace).*$#$1#) {
            if ($i >= 2) {
                if ($lines[$i-1] =~ /\S/) {
                    $lines[$i-1] .= "\n\n";
                } elsif ($lines[$i-2] =~ /\S/) {
                    $lines[$i-2] .= "\n";
                }
            }
        } else {
            $e=$i;
            while($i > $m && $lines[$i] =~ /^}\s*$/) { --$i; }
            $nclose = $e - $i;
            if ($nclose) {
                $s = $i;
                while ($s > 0 && $lines[$s] =~ /^\s*$/) { --$s; }
                ++$s;
                splice @lines,$s,$e-$s+1,"\n","\n",('}' x $nclose)."\n";
            }
        }
        print for (@lines);
        @lines=();
    }
}
