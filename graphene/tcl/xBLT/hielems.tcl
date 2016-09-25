namespace eval xblt::hielems { # highlight elements
    variable data

    bind xblt::hielems <Motion> {xblt::hielems::trace_current %W}
    bind xblt::hielems <ButtonPress-1> {xblt::hielems::toggle_hide_event %W}
    bind xblt::hielems <ButtonPress-2> {xblt::hielems::toggle_hilit_event %W}
    bind xblt::hielems <ButtonPress-3> {xblt::hielems::prepare_legmenu %W}
}

proc xblt::hielems {graph args} {
    eval xblt::hielems::add $graph $args
}
	
proc xblt::hielems::add {graph args} {
    variable data
    xblt::parse_options xblt::hielems::add $args {
	-usemenu usemenu 0
	-hidemenulabel hideml Hide
	-hilitmenulabel hilitml Highlight
    }

    if {$usemenu} {
	set m [xblt::legmenu::add $graph]
	$m add check -label $hideml -variable xblt::hielems::data(h,$graph) \
	    -command [list xblt::hielems::toggle_hide_menu $graph]
	$m add check -label $hilitml \
	    -variable xblt::hielems::data(l,$graph) \
	    -command [list xblt::hielems::toggle_hilit_menu $graph]
    }

    set data(a,$graph) {}

    xblt::bindtag::add $graph xblt::hielems
}

proc xblt::hielems::trace_current {graph} {
    variable data
    set e [$graph legend get current]
    set a $data(a,$graph)
    if {$e eq ""} {set e [$graph element get current]}
    if {$e eq $a} return
    if {$a ne ""} {$graph legend deactivate $a}
    if {$e ne ""} {$graph legend activate $e}
    set data(a,$graph) $e
}

proc xblt::hielems::is_hidden {graph e} {
    $graph element cget $e -hide
}

proc xblt::hielems::toggle_hide {graph e {hide -1}} {
    if {$e eq ""} return
    set en [string trim [$graph element cget $e -label] \[\] ]
    if {$hide == -1} {
	set hide [expr {! [is_hidden $graph $e]}]
    }
    if {$hide} {
	$graph element configure $e -hide 1 -label "\[$en\]"
    } else {
	$graph element configure $e -hide 0 -label $en
    }
}

proc xblt::hielems::toggle_hide_event {graph} {
    set e [$graph legend get current]
    if {$e eq ""} return
    toggle_hide $graph $e
    return -code break
}

proc xblt::hielems::toggle_hide_menu {graph} {
    variable data
    toggle_hide $graph [xblt::legmenu::curelem $graph]
}

proc xblt::hielems::is_hilit {graph e} {
    if {$e == ""} { return 0 }
    expr { [$graph element cget $e -labelrelief] != "flat" }
}

proc xblt::hielems::toggle_hilit {graph e {hilit -1}} {
    if {$e eq ""} return
    set ishilit [is_hilit $graph $e]
    if {$hilit == -1} {
	set hilit [expr {! $ishilit}]
    }
    if {$hilit} {
	if {!$ishilit} {
	    $graph element configure $e -linewidth \
		[expr {[$graph element cget $e -linewidth]+2}] \
		-labelrelief raised
	    # fight BLT bug
	    $graph element configure $e -hide \
		[$graph element cget $e -hide]
	}
    } else {
	if {$ishilit} {
	    $graph element configure $e -linewidth \
		[expr {[$graph element cget $e -linewidth]-2}] \
		-labelrelief flat
	    # fight BLT bug
	    $graph element configure $e -hide \
		[$graph element cget $e -hide]
	}
    }
}

proc xblt::hielems::toggle_hilit_event {graph} {
    set e [$graph legend get current]
    if {$e eq ""} return
    toggle_hilit $graph $e
    return -code break
}

proc xblt::hielems::toggle_hilit_menu {graph} {
    variable data
    toggle_hilit $graph [xblt::legmenu::curelem $graph]
}


proc xblt::hielems::prepare_legmenu {graph} {
    variable data
    set e [$graph legend get current]
    if {$e eq ""} return
    set data(h,$graph) [is_hidden $graph $e]
    set data(l,$graph) [is_hilit $graph $e]
}
