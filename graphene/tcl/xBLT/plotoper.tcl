namespace eval xblt::plotoperation {
    # Support operation - own menu, cursor, Escape to cancel
    variable data
}

proc xblt::plotoperation::register {graph name args} {
    variable data
    xblt::parse_options xblt::plotoperation::register $args [subst {
	-cancelcommand data($graph,$name,cmd) {}
	-usemenu usemenu 1
	-cancellabel lab {Cancel $name}
	-chainlabel clab {Plot menu}
	-cursor data($graph,$name,cursor) {}
	-bindtag data($graph,$name,tag) {}
    }]
    set tag xblt::plotoper:$name
    bind $tag <Key-Escape> [list xblt::plotoperation::cancel %W $name]\nbreak

    if {$usemenu} {
	set m [menu $graph.plotoper_$name -tearoff no]
	if {$lab ne ""} {
	    $m add command -label $lab -command \
		[list xblt::plotoperation::cancel $graph $name]
	}
	if {$clab ne "" && [xblt::plotmenu::on_canvas $graph]} {
	    $m add cascade -label $clab -menu [xblt::plotmenu::get $graph]
	}
	set data($graph,$name,menu) $m
	bind $tag <ButtonPress-3> \
	    [list xblt::plotoperation::post_menu $graph $name %X %Y %x %y]
    }
}

proc xblt::plotoperation::activate {graph name} {
    variable data
    xblt::bindtag::add $graph xblt::plotoper:$name
    if {$data($graph,$name,tag) ne ""} {
	xblt::bindtag::add $graph $data($graph,$name,tag)
    }
    if {$data($graph,$name,cursor) ne ""} {
	set data($graph,$name,ocursor) [$graph cget -cursor]
	$graph configure -cursor $data($graph,$name,cursor)
    }
    focus $graph ;# for Escape key to work
}

proc xblt::plotoperation::cancel {graph name} {
    variable data
    if {$data($graph,$name,tag) ne ""} {
	xblt::bindtag::remove $graph $data($graph,$name,tag)
    }
    xblt::bindtag::remove $graph xblt::plotoper:$name
    if {[info exist data($graph,$name,ocursor)]} {
	$graph configure -cursor $data($graph,$name,ocursor)
    }
    set cmd $data($graph,$name,cmd)
    if {$cmd ne ""} {
	# catch possible return -code break
	foreach a a {
	    uplevel \#0 $cmd [list $graph $name]
	}
    }
}

proc xblt::plotoperation::post_menu {graph name X Y x y} {
    variable data
    if {![info exist data($graph,$name,menu)]} return
    if {![$graph inside $x $y]} return
    tk_popup $data($graph,$name,menu) $X $Y
    return -code break
}

proc xblt::plotoperation::get_menu {graph name} {
    variable data
    if {[info exist data($graph,$name,menu)]} {
	return $data($graph,$name,menu)
    } else {
	return ""
    }
}

proc xblt::plotoperation::is_active {graph name} {
    xblt::bindtag::check $graph xblt::plotoper:$name
}
