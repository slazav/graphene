namespace eval xblt::unitaxes {}

proc xblt::unitaxes::add {graph} {
    if {[$graph axis names xblt::unitx] eq ""} {
	$graph axis create xblt::unitx -hide 1 -min 0 -max 1
    }
    if {[$graph axis names xblt::unity] eq ""} {
	$graph axis create xblt::unity -hide 1 -min 0 -max 1
    }
}
