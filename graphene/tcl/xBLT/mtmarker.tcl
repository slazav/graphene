namespace eval xblt::mtmarker { ;# moving text marker
    variable data
}
    
proc xblt::mtmarker {graph name args} {
    eval xblt::mtmarker::create $graph $name $args
}
	
proc xblt::mtmarker::create {graph name args} {
    variable data
    xblt::parse_options xblt::mtmarker::create $args {
	{-background -bg} bg yellow
	{-foreground -fg} fg black
	-pin pin 0
	-pinsize pinsize 18
	{-xaxis -mapx} xaxis x
	{-yaxis -mapy} yaxis y
	-element bindelem {}
	-font font {Helvetica 12}
    }
    $graph marker create text -name $name -justify left \
	-background $bg -foreground $fg -mapx $xaxis -mapy $yaxis \
	-element $bindelem -font $font
    set data($graph,usepin,$name) $pin
    if {$pin} {
	set data($graph,pin,$name) \
	    [$graph marker create text \
		 -font "-adobe-symbol-*-r-*-*-$pinsize-*-*-*-*-*-*-*" \
		 -anchor c -text "\267" -mapx $xaxis -mapy $yaxis \
		 -foreground $fg -background "" -element $bindelem]
    }
}

proc xblt::mtmarker::move {graph name x y msg} {
    variable data
    set xax [$graph marker cget $name -mapx]
    set yax [$graph marker cget $name -mapy]
    set xlims [$graph axis limits $xax]
    if {$x < [lindex $xlims 0] + \
	    ([lindex $xlims 1] - [lindex $xlims 0])/2.5} {
	set xofs 6
	set xanch w
    } else {
	set xofs -6
	set xanch e
    }
    set ylims [$graph axis limits $yax]
    if {$y > [lindex $ylims 1] - \
	    ([lindex $ylims 1] - [lindex $ylims 0])/2.5} {
	set yofs 6
	set yanch n
    } else {
	set yofs -6
	set yanch s
    }
    $graph marker configure $name \
	-xoffset $xofs -yoffset $yofs -anchor $yanch$xanch \
	-text $msg \
	-coords [list $x $y]
    if {$data($graph,usepin,$name)} {
	$graph marker configure $data($graph,pin,$name) -coords [list $x $y]
    }
}

proc xblt::mtmarker::hide {graph name} {
    variable data
    $graph marker configure $name -coords {}
    if {$data($graph,usepin,$name)} {
	$graph marker configure $data($graph,pin,$name) -coords {}
    }
}
