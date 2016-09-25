namespace eval xblt::rubberrect {
    variable data
}

proc xblt::rubberrect::add {graph args} {
    variable data
    xblt::parse_options xblt::rubberrect::add $args {
	-modifier mod {}
	-button button 1
	-cancelbutton button2 3
	-type type xy
	-command cmd {}
	-configure cfg {}
	-invtransformx invx {}
	-invtransformy invy {}
	-name name rectangle
    }
    set id rr$mod$button
    set data($graph,$id,cmd) $cmd
    set data($graph,$id,s) 0
    set data($graph,$id,invx) $invx
    set data($graph,$id,invy) $invy

    set data($graph,$id,usex) [expr {[string first x $type] >= 0}]
    set data($graph,$id,usey) [expr {[string first y $type] >= 0}]
    if {! ($data($graph,$id,usex) || $data($graph,$id,usey))} return

    xblt::unitaxes::add $graph

    set data($graph,$id,r) \
	[$graph marker create polygon -dashes 5 -fill "" \
	     -linewidth 2 -mapx xblt::unitx -mapy xblt::unity]
    if {$cfg ne ""} {
	eval $graph marker configure $data($graph,$id,r) $cfg
    }
    
    
    if {$mod eq ""} {
	set e ButtonPress-$button
    } else {
	set e $mod-ButtonPress-$button
    }
    set tag xblt::$id
    bind $tag <$e> \
	[list xblt::rubberrect::start %W $id %x %y]
    bind $tag:a <ButtonRelease-$button> \
	[list xblt::rubberrect::end %W $id %x %y]
    bind $tag:a <Button$button-Motion> \
	[list xblt::rubberrect::do %W $id %x %y]
    if {$button2 ne ""} {
	bind $tag:a <Button$button-ButtonPress-$button2> \
	    [list xblt::plotoperation::cancel %W $id]\nbreak
    }
    xblt::bindtag::add $graph $tag
    xblt::plotoperation::register $graph $id \
	-cancelcommand xblt::rubberrect::cancel \
	-cancellabel "Cancel $name" -bindtag $tag:a
}

proc xblt::rubberrect::start {graph id x y} {
    variable data
    if {! [$graph inside $x $y]} return
    set data($graph,$id,x0) $x
    set data($graph,$id,y0) $y
    set data($graph,$id,s) 1
    $graph marker before $data($graph,$id,r)
    xblt::plotoperation::activate $graph $id
    return -code break
}

proc xblt::rubberrect::cancel {graph id} {
    variable data
    if {! $data($graph,$id,s)} return
    set data($graph,$id,s) 0
    $graph marker configure $data($graph,$id,r) -coords {}
    return -code break
}

proc xblt::rubberrect::small {graph id x y} {
    variable data
    expr {($data($graph,$id,usex) && (abs($x - $data($graph,$id,x0)) < 4)) || ($data($graph,$id,usey) && (abs($y - $data($graph,$id,y0)) < 4))}
}

proc xblt::rubberrect::do {graph id x y} {
    variable data
    if {!$data($graph,$id,s)} return
    if {[small $graph $id $x $y]} {
	set c {}
    } else {
	if {$data($graph,$id,usex)} {
	    set x1 \
		[$graph axis invtransform xblt::unitx $data($graph,$id,x0)]
	    set x2 [$graph axis invtransform xblt::unitx $x]
	} else {
	    set x1 0
	    set x2 1
	}
	if {$data($graph,$id,usey)} {
	    set y1 \
		[$graph axis invtransform xblt::unity $data($graph,$id,y0)]
	    set y2 [$graph axis invtransform xblt::unity $y]
	} else {
	    set y1 0
	    set y2 1
	}
	set c [list $x1 $y1 $x2 $y1 $x2 $y2 $x1 $y2]
    }
    $graph marker configure $data($graph,$id,r) -coords $c
    return -code break
}

proc xblt::rubberrect::end {graph id x y} {
    variable data
    if {! $data($graph,$id,s)} return
    xblt::plotoperation::cancel $graph $id
    if {![small $graph $id $x $y]} {
	set invx $data($graph,$id,invx)
	set invy $data($graph,$id,invy)
	if {$invx eq ""} {
	    set resx [lsort -integer [list $data($graph,$id,x0) $x]]
	} else {
	    set resx [lsort -real [list [$graph axis invtransform $invx $data($graph,$id,x0)] [$graph axis invtransform $invx $x]]]
	}
	if {$invy eq ""} {
	    set resy [lsort -integer [list $data($graph,$id,y0) $y]]
	} else {
	    set resy [lsort -real [list [$graph axis invtransform $invy $data($graph,$id,y0)] [$graph axis invtransform $invy $y]]]
	}
	if {$data($graph,$id,cmd) ne ""} {
	    uplevel \#0 $data($graph,$id,cmd) $graph $resx $resy
	}
    }
    return -code break
}
