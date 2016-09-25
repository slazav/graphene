namespace eval xblt::scrollbydrag {
    variable data
}

proc xblt::scrollbydrag {graph args} {
    eval xblt::scrollbydrag::add $graph $args
}
	
proc xblt::scrollbydrag::add {graph args} {
    variable data
    xblt::parse_options xblt::scrollbydrag::add $args {
	-modifier mod {}
	-button button 2
	-axes axes {}
    }
    
    set data($graph,axes) [lsort -ascii $axes]
    set data($graph,useaxes) [expr {[llength $axes] > 0}]
    
    if {$mod == ""} {set ep ""} else {set ep ${mod}- }
    bind xblt::scrollbydrag:$mod:$button <${ep}ButtonPress-$button> {
	xblt::scrollbydrag::start %W %x %y
    }
    bind xblt::scrollbydrag:$button <ButtonRelease-$button> {
	xblt::scrollbydrag::stop %W %x %y
    }
    bind xblt::scrollbydrag:$button <Button$button-Motion> {
	xblt::scrollbydrag::do %W %x %y
    }
    set data($graph,s) 0
    xblt::bindtag::add $graph xblt::scrollbydrag:$mod:$button
    xblt::bindtag::add $graph xblt::scrollbydrag:$button
}

proc xblt::scrollbydrag::start {graph x y} {
    variable data
    if {! [$graph inside $x $y]} return
    set data($graph,s) 1
    set data($graph,x) $x
    set data($graph,y) $y
    return -code break
}

proc xblt::scrollbydrag::stop {graph x y} {
    variable data
    if {! $data($graph,s)} return
    set data($graph,s) 0
    return -code break
}

proc xblt::scrollbydrag::do {graph x y} {
    variable data
    if {! $data($graph,s)} return
    foreach d {x y} b0 [list $data($graph,x) $data($graph,y)] b [list $x $y] {
	foreach a [concat [$graph ${d}axis use] [$graph ${d}2axis use]] {
	    if {$data($graph,useaxes) &&
		[lsearch -sorted $data($graph,axes) $a] < 0} continue
	    set aa [$graph axis limits $a]
	    set da [expr {[$graph axis invtransform $a $b0] - [$graph axis invtransform $a $b]}]
	    $graph axis configure $a \
		-min [expr {[lindex $aa 0] + $da}] -max [expr {[lindex $aa 1] + $da}]
	}
    }
    set data($graph,x) $x
    set data($graph,y) $y
    return -code break
}
