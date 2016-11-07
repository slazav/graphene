namespace eval comments {
    variable listdlg
    variable finddlg
    variable comdlg
    variable cdata {}
}

proc comments::initdb {} {
    db "prepare load_comments(double precision,double precision) as
      select sernum, from_tstamp(tstamp) as tsec, body
      from comments where tstamp between to_tstamp(\$1) and to_tstamp(\$2)
         and not delflag order by tstamp"
    pg_listen $db::conn monitor_comments comments::load
}

proc comments::load {} {
    # load comments in the range
    global range_tstart range_tend
    variable cdata
    set cdata {}
    set res [pg_exec $db::conn "execute load_comments($range_tstart,$range_tend)"]
    set n [pg_result $res -numTuples]
    for {set i 0} {$i < $n} {incr i} {
	lappend cdata [pg_result $res -getTuple $i]
    }
    foreach v [itcl::find objects -class viewer] {
	$v comments_changed
    }
    pg_result $res -clear
}

proc comments::closest {t} {
    variable cdata
    set l [llength $cdata]
    if {$l == 0} {return 0}
    if {$l == 1 || $t < [lindex $cdata 0 1]} {return [lindex $cdata 0 0]}
    if {$t >= [lindex $cdata end 1]}  {return [lindex $cdata end 0]}
    set i 0
    set j [expr {$l-1}]
    while { $i < $j - 1} {
	set k [expr {($i+$j)/2}]
	if {$t < [lindex $cdata $k 1]} {
	    set j $k
	} else {
	    set i $k
	}
    }
    # our closest element is i-th or j-th
    if {$t - [lindex $cdata $i 1] < [lindex $cdata $j 1] - $t} {
	return [lindex $cdata $i 0]
    } else {
	return [lindex $cdata $j 0]
    }
}

proc comments::move {serial newtim} {
    db "update comments set tstamp=to_tstamp($newtim), username=current_user, modtime=current_timestamp where sernum=$serial; notify monitor_comments"}

proc comments::list_dlg {viewer win {deleted 0}} {
    global range_tstart range_tend
    global str_data_tstart str_data_tend
    variable listdlg
    make_list_dlg
    .clist configure -master $win -title "List of comments" 
    set listdlg(viewer) $viewer
    $listdlg(l) configure -labeltext [string map {\n " "} "Comments from $str_data_tstart to $str_data_tend"][lindex {"" "\n(marked as deleted)"} $deleted]
    list_dlg_fill "tstamp between to_tstamp($range_tstart) and to_tstamp($range_tend) and [lindex not $deleted] delflag"
    .clist activate
}

proc comments::list_dlg_again {viewer win} {
    variable listdlg
    if {![info exists listdlg(viewer)]} {
	return [list_dlg $viewer $win]
    }
    .clist configure -master $win
    set listdlg(viewer) $viewer
    .clist activate
}

proc comments::list_dlg_fill {query} {
    variable listdlg
    set lb $listdlg(l)
    $lb clear
    set n 0
    db::loop "select to_char(tstamp,'YYYY-MM-DD HH24:MI:SS') as tim, body, from_tstamp(tstamp) as tsec, sernum, case delflag when true then 1 else 0 end as flag from comments where $query order by tstamp" {
	set str [string map {\n " "} $body]
	$lb insert end "$tim    $str"
	set listdlg($n,tsec) $tsec
	set listdlg($n,body) $body
	set listdlg($n,sernum) $sernum
	set listdlg($n,delflag) $flag
	incr n
    }
}

proc comments::make_list_dlg {} {
    variable listdlg
    if {[winfo exists .clist]} return
    iwidgets::dialogshell .clist -modality application 
    .clist add goto -text "Go to" -command comments::list_dlg_goto
    .clist add edit -text "Edit"  -command comments::list_dlg_edit
    .clist add close -text "Close" -command comments::list_dlg_close
    bind .clist <Key-Return> {}
    bind .clist <Key-Escape> comments::list_dlg_close

    set f [.clist childsite]

    iwidgets::scrolledlistbox $f.l -width 400 -height 300 \
	-selectmode single -vscrollmode dynamic -hscrollmode dynamic \
	-textfont {helvetica 12} -labelpos nw
    $f.l component label configure -justify left
    set listdlg(l) $f.l
    bind [$f.l component listbox] <Key-Return> comments::list_dlg_goto
    bind [$f.l component listbox] <Double-ButtonPress-1> comments::list_dlg_goto
    pack $f.l -side top -fill both -expand yes
}

proc comments::list_dlg_goto {} {
    variable listdlg
    global range_tstart range_tend
    set sel [$listdlg(l) curselection]
    if {$sel == ""} return
    set tsec $listdlg($sel,tsec)
    if {$tsec < $range_tstart || $tsec > $range_tend} {
	if {![get_confirmation "This comment is outside of the current timespan. Do you want to adjust the timespan to include the comment?" .clist]} {
	    return
	}
	# shift range
	set rw [expr {$range_tend - $range_tstart}]
	if {$tsec < $range_tstart} {
	    set_data_range_sec [expr {$tsec-0.1*$rw}] [expr {$tsec+0.9*$rw}]
	} else {
	    set_data_range_sec [expr {$tsec-0.9*$rw}] [expr {$tsec+0.1*$rw}]
	}
    }
    $listdlg(viewer) include_in_view_range $tsec 0.1
    $listdlg(viewer) highlight_comments $tsec
    .clist deactivate 1
}

proc comments::list_dlg_close {} {
    .clist deactivate 0
}

proc comments::list_dlg_edit {} {
    variable comdlg
    variable listdlg
    set sel [$listdlg(l) curselection]
    if {$sel == ""} return
    if {[edit_comment $listdlg($sel,sernum) $listdlg($sel,tsec) \
	     $listdlg($sel,body) $listdlg($sel,delflag) \
	     $listdlg(viewer) .clist]} {
	# update our info
	set listdlg($sel,tsec) $comdlg(newtim)
	set listdlg($sel,body) $comdlg(newbody)
	set listdlg($sel,delflag) $comdlg(dv)
	# now update list line
	set tim [clock format [expr {int($comdlg(newtim))}] -format "%Y-%m-%d %H:%M:%S"]
	set str [string map {\n " "} $comdlg(newbody)]
	set lb $listdlg(l)
	$lb delete $sel
	$lb insert $sel "$tim    $str"
	$lb selection set $sel
    }
}

proc comments::find_dlg {viewer win} {
    variable finddlg
    make_find_dlg
    .cfind configure -master $win
    set finddlg(viewer) $viewer
    after idle [list focus [$finddlg(s) component entry]]
    .cfind activate
}

proc comments::make_find_dlg {} {
    variable finddlg
    if {[winfo exists .cfind]} return
    iwidgets::dialogshell .cfind -modality application \
	-title "Find comments"
    .cfind add proceed -text "Proceed" -command comments::find_dlg_proceed
    .cfind add close -text "Close" -command comments::find_dlg_close
    .cfind default proceed
    bind .cfind <Key-Escape> comments::find_dlg_close
    
    set f [.cfind childsite]

    iwidgets::radiobox $f.t -labeltext "Time range:" -labelpos nw \
	-ipadx 2 -ipady 2
    $f.t add range -text "Current timespan" -highlightthickness 1
    $f.t add all -text "All time" -highlightthickness 1
    $f.t add user -text "Given range" -highlightthickness 1
    set ft [$f.t childsite]
    iwidgets::entryfield $ft.f -labeltext "   from:" -width 25
    iwidgets::entryfield $ft.t -labeltext "   to:" -width 25
    grid $ft.f -sticky we
    grid $ft.t -sticky we
    iwidgets::Labeledwidget::alignlabels $ft.f $ft.t
    set finddlg(tfrom) $ft.f
    set finddlg(tto) $ft.t
    
    $f.t select range
    set finddlg(t) $f.t
    pack $f.t -side top -fill x

    iwidgets::combobox $f.s -labeltext "Search string:" \
	-width 35 -labelpos nw -completion false \
	-command comments::find_dlg_proceed
    set finddlg(s) $f.s
    pack $f.s -side top -fill x
    
}

proc comments::find_dlg_proceed {} {
    variable finddlg
    variable listdlg
    global range_tstart range_tend
    global str_data_tstart str_data_tend
    set s [$finddlg(s) get]
    if {[string length [string trim $s]] > 0 &&
	[lsearch -exact [$finddlg(s) component list get 0 end] $s] < 0} {
	$finddlg(s) insert list end $s
    }
    # now check time
    switch -exact -- [$finddlg(t) get] {
	range {
	    set qtim "tstamp between to_tstamp($range_tstart) and to_tstamp($range_tend)"
	    set timdesc [string map {\n " "} "from $str_data_tstart to $str_data_tend"]\n
	}
	all {
	    set qtim true
	    set timdesc ""
	}
	user {
	    foreach tv {tfrom tto} tdesc {start end} {
		set tim [$finddlg($tv) get]
		if {[catch {db "select from_tstamp('$tim') as tx"}]} {
		    show_error "Invalid $tdesc time specification '$tim'." .cfind
		    after idle [list focus [$finddlg($tv) component entry]]
		    return
		}
		set $tv $tim
		set tsec($tv) $tx
	    }
	    set qtim "tstamp between '$tfrom' and '$tto'"
	    set timdesc "from [format_time_x $tsec(tfrom)] to [format_time_x $tsec(tto)]\n"
	}
    }
    # now pattern
    set pgs [string map {' '' "\\*" * "\\?" ?
	"\\" "\\\\" % "\\\\%" _ "\\\\_" * % ? _} $s]
    set qpat "body like '%${pgs}%'"
    make_list_dlg
    .clist configure -master [.cfind cget -master] -title "Found comments" 
    set listdlg(viewer) $finddlg(viewer)
    $listdlg(l) configure -labeltext "Comments ${timdesc}matching '$s'"
    list_dlg_fill "$qtim and $qpat and not delflag"
    .cfind deactivate 1
    .clist activate
}

proc comments::find_dlg_close {} {
    .cfind deactivate 0
}

proc comments::make_com_dlg {} {
    variable comdlg
    if {[winfo exists .comd]} return
    iwidgets::dialogshell .comd -modality application
    .comd add OK -text OK -command comments::com_dlg_ok
    .comd add Cancel -text Cancel -command comments::com_dlg_cancel
    bind .comd <Key-Return> {}
    bind .comd <Key-Escape> comments::com_dlg_cancel
    set f [.comd childsite]
    label $f.l1 -text "Place comment at a time:"
    pack $f.l1 -side top -anchor w
    set comdlg(t) [entry $f.t -width 30]
    bind $f.t <Key-Return> comments::com_dlg_checktime
    pack $f.t -side top -anchor w
    label $f.l3 -text "Comment text:"
    pack $f.l3 -side top -anchor w
    text $f.b -background white -width 30 -height 6 -font {helvetica 12}
    set comdlg(b) $f.b
    bind $f.b <Alt-Key-Return> comments::com_dlg_ok
    pack $f.b -side top -fill both -expand yes
    set comdlg(d) [checkbutton $f.d -text "Mark as deleted" -variable comments::comdlg(dv)]
}

proc comments::com_dlg_ok {} {
    variable comdlg
    set txt [string trim [$comdlg(b) get 1.0 end]]
    if {[string length $txt] == 0} {
	if {$comdlg(serial)==0} {
	    .comd deactivate 0
	} else {
	    show_error "Please provide comment text." .comd
	}
	return
    }
    set tsec [com_dlg_checktime]
    if {$tsec < 0} return
    if {![$comdlg(viewer) in_displayed_range_ext $tsec] &&
	[get_confirmation "The comment will be placed outside of the current view. Do you want to continue?" .comd] == 0} {
	return
    }
    set etxt [string map {' '' "\\" "\\\\"} $txt]
    if {$comdlg(serial) == 0} {
	# add comment
	db "insert into comments values(nextval('serial'),to_tstamp($tsec),'$etxt',current_user,current_timestamp,false); notify monitor_comments"
    } else {
	# modify comment
	db "update comments set tstamp=to_tstamp($tsec), body='$etxt', username=current_user, modtime=current_timestamp, delflag=[lindex {false true} $comdlg(dv)] where sernum=$comdlg(serial); notify monitor_comments"
    }
    set comdlg(newtim) $tsec
    set comdlg(newbody) $txt
    .comd deactivate 1
}

proc comments::com_dlg_cancel {} {.comd deactivate 0}

proc comments::com_dlg_checktime {} {
    variable comdlg
    set tim [$comdlg(t) get]
    if {[catch {db "select from_tstamp('$tim') as tsec"}]} {
	show_error "Invalid time specification '$tim'." .comd
	return -1
    }
#    $comdlg(viewer) move_commentpos $tsec
    return $tsec
}

proc comments::add_comment {tim viewer topwin} {
    variable comdlg
    make_com_dlg
    .comd configure -title "Add comment" -master $topwin
    catch {pack forget $comdlg(d)}
    set comdlg(dv) 0
    $comdlg(t) delete 0 end
    $comdlg(t) insert end [format_time_z $tim]
    $comdlg(b) delete 1.0 end
    after idle [list focus $comdlg(b)]
    set comdlg(viewer) $viewer
    set comdlg(serial) 0
#    $viewer move_commentpos $tim
    .comd activate
#    $viewer hide_commentpos
}

proc comments::edit_comment {serial tim body delflag viewer topwin} {
    variable comdlg
    make_com_dlg
    .comd configure -title "Edit comment" -master $topwin
    catch {pack $comdlg(d) -side top -anchor w}
    set comdlg(dv) $delflag
    $comdlg(t) delete 0 end
    $comdlg(t) insert end [format_time_z $tim]
    $comdlg(b) delete 1.0 end
    $comdlg(b) insert end $body
    after idle [list focus $comdlg(b)]
    set comdlg(viewer) $viewer
    set comdlg(serial) $serial
#    $viewer move_commentpos $tim
    set res [.comd activate]
#    $viewer hide_commentpos
    return $res
}