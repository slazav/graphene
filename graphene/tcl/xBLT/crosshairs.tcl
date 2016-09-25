namespace eval xblt::crosshairs {
    variable data

    bind xblt::crosshairs <Motion> {
	%W crosshairs configure -position @%x,%y
    }
}

proc xblt::crosshairs {graph args} {
    eval xblt::crosshairs::add $graph $args
}
	
proc xblt::crosshairs::add {graph args} {
    variable data
    xblt::parse_options xblt::crosshairs::add $args {
	-variable var {}
	-show sh {}
	-usemenu usemenu 0
	-menulabel menulabel Crosshairs
    }
    set data($graph,var) $var
    if {!$usemenu} {set menulabel ""}
    xblt::var::use $graph $var $sh xblt::crosshairs::show $menulabel
}

proc xblt::crosshairs::show {W {sh 1}} {
    variable data
    upvar \#0 $data($W,var) v
    if {[info exists v]} {set sh $v}
    if {$sh} {
	xblt::bindtag::add $W xblt::crosshairs
	$W crosshairs configure -hide 0
    } else {
	xblt::bindtag::remove $W xblt::crosshairs
	$W crosshairs configure -hide 1
    }
}

