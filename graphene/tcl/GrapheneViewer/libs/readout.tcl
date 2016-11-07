namespace eval yblt::readout { # popup readout
    variable data
    variable message

    bind yblt::readout <Motion> [namespace code {do %W %x %y}]
}

proc yblt::readout {graph args} {
    eval yblt::readout::add $graph $args
}
	
proc yblt::readout::add {graph args} {
    variable data
    variable message
    xblt::parse_options yblt::readout::add $args [subst {
	-variable data($graph,var) {}
	-active do {}
	-command data($graph,cmd) {}
	-interpolate data($graph,interp) no
	-onmarkers data($graph,marks) {} 
	-usemenu usemenu 0
	-menulabel menulabel Readout
	-formatcommand data($graph,fcmd) {}
	-eventcommand eventcommand {}
    }]
    xblt::mtmarker::create $graph yblt::readout -bg yellow -fg black
    set data($graph,e) {}
    set message($graph) {}
    if {![info exists data($graph,susp)]} {
	set data($graph,susp) 0
    }

    if {[llength $eventcommand]>=2} {
	set data($graph,evcmd) [lrange $eventcommand 1 end]
	bind $graph <[lindex $eventcommand 0]> \
	    {yblt::readout::eventcmd %W %x %y}
    }

    if {!$usemenu} {set menulabel ""}
    xblt::var::use $graph $data($graph,var) $do \
	yblt::readout::activate $menulabel
}

proc yblt::readout::activate {W {do 1}} {
    variable data
    upvar \#0 $data($W,var) v
    if {[info exists v]} {set do $v}
    if {$do} {
	xblt::bindtag::add $W yblt::readout -after all
    } else {
	xblt::bindtag::remove $W yblt::readout
	finish $W
    }
}

proc yblt::readout::do {graph x y} {
    variable data
    variable message
    if {$data($graph,susp)} return
    
    set m [$graph marker get current]
    if {$m != ""} {
	set retf 1
	foreach gm $data($graph,marks) {
	    if {[string match $gm $m]} {
		set retf 0
		break
	    }
	}
	if {$retf} {
	    finish $graph
	    return
	}
    }
    
    if {[$graph element closest $x $y ee \
	     -interpolate $data($graph,interp)] && \
	    [$graph element cget $ee(name) -label] != {}} {
	set xax [$graph element cget $ee(name) -mapx]
	set yax [$graph element cget $ee(name) -mapy]
	$graph crosshairs configure -position "@[$graph axis transform $xax $ee(x)],[$graph axis transform $yax $ee(y)]"
	if {[string equal $data($graph,e) $ee(name)] \
		&& $data($graph,ox) == $ee(x) \
		&& $data($graph,oy) == $ee(y)} return
	array set data [list $graph,e $ee(name) \
			    $graph,ox $ee(x) $graph,oy $ee(y)]
	if {$data($graph,fcmd) ne ""} {
	    set msg [uplevel \#0 $data($graph,fcmd) \
			 [list $graph $ee(name) $ee(x) $ee(y)]]
	} else {
	    # format ourselves
	    set xstr [format $graph x $ee(name) $ee(x) $xax]
	    set ystr [format $graph y $ee(name) $ee(y) $yax]
	    set msg "$xstr\n$ee(name): $ystr"
	}
	if {$data($graph,cmd) != ""} {
	    uplevel \#0 $data($graph,cmd) [list $graph $ee(name) $ee(x) $ee(y)]
	}
	$graph marker before yblt::readout
	$graph marker configure yblt::readout -mapx $xax -mapy $yax
	xblt::mtmarker::move $graph yblt::readout $ee(x) $ee(y) $msg
	set message($graph) $msg
    } else {
	finish $graph
    }
}

proc yblt::readout::format {graph axdir ename eval {ax ""}} {
    variable data
    if {$ax eq ""} {
	set ax [$graph element cget $ename -map$axdir]
    }
    if {[info exist data($graph,efmt$axdir,$ename)]} {
	set fmt $data($graph,efmt$axdir,$ename)
    } elseif {[info exist data($graph,axfmt,$ax)]} {
	set fmt $data($graph,axfmt,$ax)
    } else {
	set fmt %.6g
    }
    if {$fmt ne ""} {
	set str [::format $fmt $eval]
    } else {
	set str ""
    }
    set axcmd [$graph axis cget $ax -command]
    if {$axcmd ne ""} {
	set astr [eval "$axcmd $graph $eval"]
	if {$str ne ""} {
	    set str "$astr ($str)"
	} else {
	    set str $astr
	}
    }
    return $str
}

proc yblt::readout::set_axis_format {graph ax fmt} {
    variable data
    set data($graph,axfmt,$ax) $fmt
}

proc yblt::readout::set_element_format {graph ename yfmt {xfmt none}} {
    variable data
    foreach d {efmty efmtx} fmt [list $yfmt $xfmt] {
	if {$fmt ne "none"} {
	    set data($graph,$d,$ename) $fmt
	}
    }
}

proc yblt::readout::finish {graph} {
    variable data
    variable message
    if {$data($graph,e) != ""} {
	set data($graph,e) {}
	$graph marker configure yblt::readout -coords {}
	if {$data($graph,cmd) != ""} {
	    uplevel \#0 $data($graph,cmd) [list $graph {} {} {}]
	}
	set message($graph) {}
    }
}

proc yblt::readout::suspend {graph {s 1}} {
    variable data
    if {$s} {finish $graph}
    set data($graph,susp) $s
}

proc yblt::readout::eventcmd {graph x y} {
    variable data
    variable message
    if {$data($graph,e) ne ""} {
	uplevel \#0 $data($graph,evcmd) \
	    [list $message($graph) $graph $data($graph,e) \
		 $data($graph,ox) $data($graph,oy)]
    }
}
