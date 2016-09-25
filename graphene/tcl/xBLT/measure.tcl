namespace eval xblt::measure {
    variable data
    variable message

    bind xblt::measure <Motion> [namespace code {e_motion %W %x %y}]
    bind xblt::measure <ButtonPress-1> [namespace code {
	e_setpt %W %x %y; break}]
    bind xblt::measure <ButtonRelease> [namespace code {
	if {$data(%W,qstop)} {xblt::plotoperation::cancel %W xblt_measure; e_cmd %W; break}}]
    bind xblt::measure <ButtonPress-2> [namespace code {e_cmd %W; break}]
}

proc xblt::measure {graph args} {
    eval xblt::measure::add $graph $args
}
	
proc xblt::measure::add {graph args} {
    variable data
    variable message
    xblt::parse_options xblt::measure::add $args {
	-command cmd {}
	-commandlabel lab usercmd
	-event ev {}
	-quickevent qev {}
	-usemenu usemenu 0
	-menulabel menulabel Measure
	-finishlabel finlab {Finish measure}
    }

    $graph marker create text -name xblt::measpin \
	-font {courier 18 bold} -text "+"
    xblt::mtmarker::create $graph xblt::meastext -bg pink -fg black

    if {$ev ne ""} {bind $graph $ev [namespace code {
	activate %W; break}]}
    if {$qev ne ""} {bind $graph $qev [namespace code {
	activate %W %x %y; break}]}

    set data($graph,cmd) $cmd

    xblt::plotoperation::register $graph xblt_measure \
	-usemenu $usemenu \
	-cancelcommand xblt::measure::e_finish \
	-cancellabel $finlab \
	-cursor dotbox -bindtag xblt::measure

    if {$usemenu} {
	[xblt::plotmenu::add $graph] add command -label $menulabel \
	    -command [list xblt::measure::activate $graph]
	if {$cmd != {}} {
	    set m [xblt::plotoperation::get_menu $graph xblt_measure]
	    $m add command -label $lab -command \
		"$cmd \$xblt::measure::message($graph)"
	}
    }

    set message($graph) {}
}

proc xblt::measure::activate {graph {x {}} {y {}}} {
    variable data
    if {[is_active $graph]} return

    set data($graph,ptset) 0
    set data($graph,qstop) 0
    if {$x ne ""} {
	if {![setpt $graph $x $y]} return
	set data($graph,qstop) 1
    }

    xblt::plotoperation::activate $graph xblt_measure
}

proc xblt::measure::is_active {graph} {
    xblt::plotoperation::is_active $graph xblt_measure
}

proc xblt::measure::e_motion {graph x y} {
    variable data
    variable message
    if {! $data($graph,ptset)} return
    if {! [$graph inside $x $y]} return
    set xx [$graph axis invtransform $data($graph,xax) $x]
    set yy [$graph axis invtransform $data($graph,yax) $y]
    set res {}
    if {$data($graph,dox)} {
	lappend res [format "d$data($graph,xax) = %.6g" [expr {$xx - $data($graph,x0)}]]
    }
    if {$data($graph,doy)} {
	lappend res [format "d$data($graph,yax) = %.6g" [expr {$yy - $data($graph,y0)}]]
    }
    set message($graph) [join $res \n]
    xblt::mtmarker::move $graph xblt::meastext $xx $yy $message($graph)
}

proc xblt::measure::setpt {graph x y} {
    variable data
    if {[$graph element closest $x $y ee -interpolate yes] && \
	    [$graph element cget $ee(name) -label] != {}} {
	set data($graph,dox) 1
	set data($graph,doy) 1
	set data($graph,xax) [$graph element cget $ee(name) -mapx]
	set data($graph,yax) [$graph element cget $ee(name) -mapy]
	set data($graph,x0) $ee(x)
	set data($graph,y0) $ee(y)
    } else {
	set alxax {}
	foreach a [concat [$graph xaxis use] [$graph x2axis use]] {
	    if {! [$graph axis cget $a -hide]} {lappend alxax $a}
	}
	set alyax {}
	foreach a [concat [$graph yaxis use] [$graph y2axis use]] {
	    if {! [$graph axis cget $a -hide]} {lappend alyax $a}
	}
	if {[llength $alxax] == 0 || [llength $alyax] == 0} {return 0}
	set data($graph,xax) [lindex $alxax 0]
	set data($graph,dox) [expr {[llength $alxax] == 1}]
	set data($graph,yax) [lindex $alyax 0]
	set data($graph,doy) [expr {[llength $alyax] == 1}]
	set data($graph,x0) \
	    [$graph axis invtransform $data($graph,xax) $x]
	set data($graph,y0) \
	    [$graph axis invtransform $data($graph,yax) $y]
    }
    if {($data($graph,dox) == 0 && $data($graph,doy) == 0)} {return 0}

    $graph marker configure xblt::measpin \
	-mapx $data($graph,xax) -mapy $data($graph,yax) \
	-coords [list $data($graph,x0) $data($graph,y0)]
    $graph marker configure xblt::meastext \
	-mapx $data($graph,xax) -mapy $data($graph,yax) -coords {}
    $graph marker before xblt::meastext
    xblt::readout::suspend $graph 1
    set data($graph,ptset) 1
    return 1
}

proc xblt::measure::e_setpt {graph x y} {
    if {[setpt $graph $x $y]} {
	return -code break
    }
}

proc xblt::measure::e_finish {graph args} {
    variable data
    $graph marker configure xblt::meastext -coords {}
    $graph marker configure xblt::measpin -coords {}
    xblt::readout::suspend $graph 0
}

proc xblt::measure::e_cmd {graph} {
    variable data
    variable message
    set cmd $data($graph,cmd)
    if {$cmd != ""} {
	uplevel \#0 [list $cmd $message($graph)]
    }
}
