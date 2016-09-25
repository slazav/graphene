namespace eval xblt::bindtag {}

proc xblt::bindtag::add {widget tag args} {
    xblt::parse_options xblt::bindtag::add $args [list -after aft $widget]
    remove $widget $tag
    set oldtags [bindtags $widget]
    if {[llength $aft]} {
	set wn [lsearch $oldtags $aft]
	if {$wn < 0 || $wn >= [llength $oldtags] - 1} {
	    # add to end
	    bindtags $widget [concat $oldtags $tag]
	} else {
	    # add as specified
	    bindtags $widget [eval lreplace [list $oldtags] [expr {$wn+1}] $wn $tag]
	}
    } else {
	# add to front
	bindtags $widget [concat $tag $oldtags]
    }
}

proc xblt::bindtag::remove {widget tags} {
    set wtags [bindtags $widget]
    foreach tag $tags {
	set n [lsearch $wtags $tag] 
	if {$n >= 0} {
	    set wtags [lreplace $wtags $n $n]
	}
    }
    bindtags $widget $wtags
}

proc xblt::bindtag::check {widget tag} {
    expr {[lsearch [bindtags $widget] $tag] >= 0}
}

