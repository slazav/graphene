namespace eval xblt::legmenu {
    variable menu
    variable curelem
    bind xblt::legmenu <ButtonPress-3> {xblt::legmenu::post_menu %W %X %Y}
}

proc xblt::legmenu {graph args} {
    eval xblt::legmenu::add $graph $args
}
	
proc xblt::legmenu::add {graph args} {
    variable menu
    xblt::parse_options xblt::legmenu::add $args {
	-showseparator sep 0
    }
    if {![info exist menu($graph)]} {
	set menu($graph) [menu $graph.legmenu -tearoff 0]
	xblt::bindtag::add $graph xblt::legmenu
    }
    if {$sep} {
	if {[$menu($graph) index last] ne "none"} {
	    $menu($graph) add separator
	}
    }
    return $menu($graph)
}

proc xblt::legmenu::get {graph} {
    variable menu
    if {[info exist menu($graph)]} {
	return $menu($graph)
    } else {
	return ""
    }
}

proc xblt::legmenu::post_menu {W X Y} {
    variable menu
    variable curelem
    set e [$W legend get current]
    if {$e == "" || ![info exist menu($W)]} return
    set curelem($W) $e
    tk_popup $menu($W) $X $Y
    return -code break
}

proc xblt::legmenu::curelem {graph} {
    variable curelem
    if {[info exist curelem($graph)]} {
	return $curelem($graph)
    } else {
	return ""
    }
}
