namespace eval xblt::escape {
    # use Escape key to cancel operation
    variable cmd
    bind xblt::escape <Key-Escape> {xblt::escape::do %W; break}
}

proc xblt::escape::activate {w {wcmd ""}} {
    variable cmd
    set cmd($w) $wcmd
    xblt::bindtag::add $w xblt::escape
    focus $w ;# for key to work
}

proc xblt::escape::do {w} {
    variable cmd
    xblt::bindtag::remove $w xblt::escape
    if {$cmd($w) ne ""} {
	# catch possible return -code break
	foreach a a {
	    uplevel \#0 $cmd($w) [list $w]
	}
    }
}
