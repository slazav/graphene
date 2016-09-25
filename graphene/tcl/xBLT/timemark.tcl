namespace eval xblt::timemark {
    variable data
}

proc xblt::timemark::create {graph args} {
    variable data
    xblt::parse_options xblt::timemark::create $args {
	-name name {}
	-xaxis axis x
	-formatcommand fcmd xblt::timemark::format_time
    }
    xblt::unitaxes::add $graph
    if {$name eq ""} {
	set m [$graph marker create text]
    } else {
	set m [$graph marker create text -name $name]
    }
    $graph marker configure $m -background PaleTurquoise \
	-foreground black -justify left \
	-mapx $axis -mapy xblt::unity
    set data(axis,$m) $axis
    set data(fcmd,$m) $fcmd
    return $m
}

proc xblt::timemark::show {graph m t msg} {
    variable data
    foreach {t1 t2} [$graph axis limits $data(axis,$m)] break
    if {$t < $t1} {
	$graph marker configure $m -anchor w \
	    -text "<<[$data(fcmd,$m) $graph $t]\n$msg" -coords {-Inf 0.45}
    } elseif {$t >= $t2} {
	$graph marker configure $m -anchor e \
	    -text "[$data(fcmd,$m) $graph $t]>>\n$msg" -coords {Inf 0.45}
    } else {
	$graph marker configure $m -anchor w \
	    -text $msg -coords [list $t 0.45]
    }
}

proc xblt::timemark::hide {graph m} {
    $graph marker configure $m -coords {}
}

proc xblt::timemark::format_time {graph t} {
    clock format [expr {int($t)}] -format "%H:%M:%S"
}
