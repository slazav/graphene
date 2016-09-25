namespace eval xblt::zoomstack {
    variable data
}

proc xblt::zoomstack {graph args} {
    eval xblt::zoomstack::add $graph $args
}
	
proc xblt::zoomstack::add {graph args} {
    variable data
    xblt::parse_options xblt::zoomstack::add $args {
	-modifier mod {}
	-button button1 1
	-unzoombutton button2 3
	-scrollbutton button3 {}
	-axes axes {}
	-recttype recttype xy
	-rectconfig rectcfg {}
	-usemenu usemenu 0
	-menuunzoomlabel uzlab Unzoom
	-menurezoomlabel rzlab {Restore zoom}
    }

    set data($graph,prev) {}
    set data($graph,next) {}
    set data($graph,axes) [lsort -ascii -unique $axes]
    set data($graph,useaxes) [expr {[llength $axes] > 0}]

    xblt::rubberrect::add $graph -modifier $mod \
	-button $button1 -cancelbutton $button2 \
	-type $recttype -configure $rectcfg -name zoom \
	-command xblt::zoomstack::zoom

    if {$button3 ne ""} {
	xblt::scrollbydrag::add $graph -axes $axes \
	    -modifier $mod -button $button3
    }

    if {$button2 ne ""} {
	if {$mod == ""} {set ep ""} else {set ep ${mod}- }
	set tag xblt::zoomstack:$mod:$button2
	bind $tag <${ep}ButtonPress-$button2> \
	    {xblt::zoomstack::unzoom_click %W %x %y}
	xblt::bindtag::add $graph $tag -after xblt::plotmenu
    }

    if {$usemenu} {
	# add zoom commands to beginning (for easy unzoom)
	set m [xblt::plotmenu::add $graph]
	$m insert 0 command -label $rzlab \
	    -command [list xblt::zoomstack::rezoom $graph]
	$m insert 0 command -label $uzlab \
	    -command [list xblt::zoomstack::unzoom $graph]
    }
}

proc xblt::zoomstack::for_axes {ax graph cmd} {
    variable data
    upvar [lindex $ax 0] a
    if {[llength $ax] > 1} {upvar [lindex $ax 1] d}
    foreach d {x y} {
	foreach a [concat [$graph ${d}axis use] [$graph ${d}2axis use]] {
	    if {$data($graph,useaxes) &&
		[lsearch -sorted $data($graph,axes) $a] < 0} continue
	    uplevel $cmd
	}
    }
}

proc xblt::zoomstack::get_limits {graph} {
    variable data
    set s {}
    for_axes a $graph {
	set axmin [$graph axis cget $a -min]
	set axmax [$graph axis cget $a -max]
	lappend s $a $axmin $axmax
    }
    return $s
}

proc xblt::zoomstack::zoom {graph x1 x2 y1 y2} {
    variable data
    array set lolim [list x $x1 y $y1]
    array set hilim [list x $x2 y $y2]
    # construct saved state
    set s [get_limits $graph]
    # change axes limits
    for_axes {a d} $graph {
	set a1 [$graph axis invtransform $a $lolim($d)]
	set a2 [$graph axis invtransform $a $hilim($d)]
	if {$a1 < $a2} {
	    $graph axis configure $a -min $a1 -max $a2
	} else {
	    $graph axis configure $a -min $a2 -max $a1
	}
    }
    #puts "zoom: {$s}"
    # now change stacks
    if {[llength $s] == 0} return
    lappend data($graph,prev) $s
    set data($graph,next)  [lrange $data($graph,next) 0 end-1]
}

proc xblt::zoomstack::restore_limits {graph s} {
    foreach {a a1 a2} $s {
	$graph axis configure $a -min $a1 -max $a2
    }
}

proc xblt::zoomstack::walk_stack {graph prev next} {
    variable data
    if {[llength $data($graph,$prev)] == 0} {return -1}
    set zprev [lindex $data($graph,$prev) end]
    set data($graph,$prev) [lrange $data($graph,$prev) 0 end-1]
    #puts "set from $prev: {$zprev}"
    set znext [get_limits $graph]
    if {[llength $znext] != 0} {lappend data($graph,$next) $znext}
    restore_limits $graph $zprev
    return [llength $data($graph,$prev)]
}

proc xblt::zoomstack::unzoom {graph} {
    if {[walk_stack $graph prev next] < 0} {
	# scroll by drag can change limits even when stack is empty
	# so we restore automatic axes limits
	for_axes a $graph {
	    $graph axis configure $a -min "" -max ""
	}
    }
}

proc xblt::zoomstack::unzoom_click {graph x y} {
    if {![$graph inside $x $y]} return
    unzoom $graph
    return -code break
}

proc xblt::zoomstack::rezoom {graph} {
    walk_stack $graph next prev
}

proc xblt::zoomstack::rezoom_click {graph x y} {
    if {![$graph inside $x $y]} return
    rezoom $graph
    return -code break
}
