#!/usr/bin/perl -i~

my @lines;
while (<>) {
    push @lines, $_;
    if (eof(ARGV)) {
        $i=$#lines;
        $e=$i;
        $m=$i - 10;
        $m = 0 if ($m < 0);
        @endif = ();
        while($i > $m) {
            $_ = $lines[$i];
            if (/^\#endif/) {
                last if scalar @endif;
                @endif = ("\n#endif\n");
            } elsif (/\S/) {
                last;
            }
            --$i;
        }
        $len = $e-$i;
        splice @lines,$i+1,$len,@endif if ($len > 0);
        print for (@lines);
        @lines=();
    }
}
