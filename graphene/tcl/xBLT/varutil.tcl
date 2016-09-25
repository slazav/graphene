namespace eval xblt::var {}

proc xblt::var::use {graph var opt cmd {menulabel ""}} {
    if {$var ne ""} {
	uplevel \#0 trace variable [list $var] w \
	    [list [list xblt::var::trace_cb $graph $cmd]]
	if {$opt ne ""} {uplevel \#0 [list set $var $opt]}
	if {$menulabel ne ""} {
	    [xblt::plotmenu::add $graph] add checkbutton \
		-label $menulabel -variable $var
	}
    } else {
	if {$opt eq ""} {set opt 1}
	uplevel \#0 $cmd [list $graph $opt]
    }
}

proc xblt::var::trace_cb {graph cmd args} {
    uplevel \#0 $cmd [list $graph]
}
