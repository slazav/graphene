namespace eval xblt::plotmenu {
    variable menu
    variable curelem
    bind xblt::plotmenu <ButtonPress-3> \
	{xblt::plotmenu::post_menu %W %X %Y %x %y}
}

proc xblt::plotmenu {graph args} {
    eval xblt::plotmenu::add $graph $args
}
	
proc xblt::plotmenu::add {graph args} {
    variable menu
    xblt::parse_options xblt::plotmenu::add $args {
	-showseparator sep 0
	-menuoncanvas usecanvas 1
	-showbutton showbutton 0
	-buttonlabel blabel Plot
	-buttonfont bfont {Helvetica 10}
	-buttonrelief brelief groove
	-buttonposition bpos {5 2}
    }
    if {![info exist menu($graph)]} {
	set menu($graph) [menu $graph.plotmenu -tearoff 0]
	if {$usecanvas} {
	    xblt::bindtag::add $graph xblt::plotmenu -after xblt::legmenu
	}
	if {$showbutton} {
	    set f [frame $graph.plotmenu_button -relief $brelief -bd 2]
	    label $f.label -text $blabel -font $bfont -bd 0
	    pack $f.label -padx 2
	    place $f -x [lindex $bpos 0] -y [lindex $bpos 1]
	    bind $f.label <ButtonPress-1> \
		[list xblt::plotmenu::post_menu $graph %X %Y]
	}
    }
    if {$sep} {
	if {[$menu($graph) index last] ne "none"} {
	    $menu($graph) add separator
	}
    }
    return $menu($graph)
}

proc xblt::plotmenu::get {graph} {
    variable menu
    if {[info exist menu($graph)]} {
	return $menu($graph)
    } else {
	return ""
    }
}

proc xblt::plotmenu::on_canvas {graph} {
    xblt::bindtag::check $graph xblt::plotmenu
}

proc xblt::plotmenu::post_menu {W X Y args} {
    variable menu
    variable curelem
    if {![info exist menu($W)]} return
    if {[llength $args] == 2 &&
	![$W inside [lindex $args 0] [lindex $args 1]]} return
    set curelem($W) [$W element get current]
    tk_popup $menu($W) $X $Y
    return -code break
}

proc xblt::plotmenu::curelem {graph} {
    variable curelem
    if {[info exist curelem($graph)]} {
	return $curelem($graph)
    } else {
	return ""
    }
}
