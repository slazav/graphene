proc xblt::parse_options {procname arglist optlist} {
    foreach {optnames varname defval} $optlist {
	foreach optname $optnames {set opts($optname) $varname}
	uplevel 1 [list set $varname $defval]
    }
    foreach {opt val} $arglist {
	if {[info exists opts($opt)]} {
	    uplevel 1 [list set $opts($opt) $val]
	} else {
	    error "$procname: unknown option $opt. Should be one of [array names opts *]."
	}
    }
    if {[llength $arglist] % 2 != 0} {
	error "$procname: no value provided for option [lindex $arglist end]."
    }
}
