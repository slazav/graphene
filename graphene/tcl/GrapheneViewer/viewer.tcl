#!/usr/bin/wish

package require Itcl
package require ParseOptions 1.0
package require xBlt 3
package require Graphene

source libs/data_source.tcl
source libs/data_mod.tcl
source libs/hielems.tcl

source comments.tcl
set ocomments 1

namespace eval graphene {


proc format_xlabel {w x} { tim2str $x }

proc tim2str {x} {
  set ret [ clock format [expr {int($x/1000)}] -format %H:%M:%S -gmt yes]
  set ms [expr int($x)%1000]
  if { $ms > 0 } {set ret "$ret.$ms"}
  return $ret
}

proc axlims {graph ax var1 var2} {
    set lims [$graph axis limits $ax]
    uplevel 1 [list set $var1 [lindex $lims 0]]\n[list set $var2 [lindex $lims 1]]
}

proc autoscale {graph} {
    set v [$graph legend get current]
    if {$v == ""} return
    set mi [vector expr min(p$v)]
    set ma [vector expr max(p$v)]
    if {$ma > $mi} {
      $graph axis configure $v -min $mi -max $ma
      position_all_comments
    }
}


######################################################################
itcl::class viewer {

  variable data_sources; # data source objects

  # Range source
  # contain fields:
  # - name - file/db name
  # - conn - graphene connection for a database source
    variable ranges

  # Comment source
  # contain fields:
  # - name - file/db name
  # - conn - graphene connection for a database source
    variable comments

  # widgets: root, plot, top menu, scrollbar:
    variable rwid
    variable pwid
    variable mwid
    variable swid

  # updater handler
    variable updater

  ######################################################################

  constructor {} {
    set data_sources {}
    set names     {}
    set comments  {}
    set ranges    {}
    set update_state 0

#    set rwid $root_widget
    set rwid {}
    if {$rwid ne {}} {frame $rwid}
    set pwid $rwid.p
    set mwid $rwid.f
    set swid $rwid.sb

    frame $mwid
#    menubutton $mwid.show -text "Show" -menu $mwid.show.menu \
#      -relief raised -bd 2 -anchor c
#    pack $mwid.show -side left -padx 4
#
#    set m [menu $mwid.show.menu -tearoff 0]
#
#    $m add checkbutton -variable omarks -command show_marks \
#      -onvalue 0 -offvalue 1 -label "File marks"
#    set ::omarks 0
#
#    $m add checkbutton -variable osranges -label "Sweep ranges" \
#      -command clear_sweep_range
#    set ::osranges 1
#
#    $m add checkbutton -variable ocomments -command show_comments \
#      -onvalue 0 -offvalue 1 -label "Log comments"
#    set ::ocomments 0
#
#    $m add checkbutton -variable omessages \
#      -label "Log messages"
#    set ::omessages 1
#
#    $m add checkbutton -variable oscomments -command show_scomments \
#      -onvalue 0 -offvalue 1 -label "Comments"
#    set ::oscomments 0
#
#    checkbutton $mwid.cross -text "Crosshairs" -variable var_crosshairs
#    pack $mwid.cross -side left -padx 2
#
#    checkbutton $mwid.readout -text "Readout" -variable var_readout
#    pack $mwid.readout -side left -padx 2
#
#    $m add checkbutton -variable oalldata -command xaxis_changed \
#        -label "All data"
#    set ::oalldata 0
#
#    $m add separator
#    $m add command -label "Graph window" -command show_graph
#    $m add command -label "Table window" -command show_table

    entry $mwid.message -textvariable message -state readonly \
        -relief flat -width 5
    set ::message "Press <F1> for help"
    pack $mwid.message -side left -padx 5 -expand yes -fill x

    checkbutton $mwid.auto -variable oauto -command "$this check_auto_update" \
        -text "Auto update"
    set ::oauto 0
    pack $mwid.auto -side left -padx 2

    checkbutton $mwid.scroll -variable oscroll \
        -text "Auto scroll"
    set ::oscroll 0
    pack $mwid.scroll -side left -padx 2

    button $mwid.update -command "$this do_update_button" -text Update
    pack $mwid.update -side left -padx 2

    button $mwid.exit -command "$this finish" -text Exit
    pack $mwid.exit -side left -padx 2

    pack $mwid -side top -fill x -padx 4 -pady 4

    scrollbar $swid -orient horizontal -command "$this scroll_command"
    pack $swid -side bottom -fill x

    set swidth [winfo screenwidth .]
    set pwidth [expr {$swidth - 80}]
    if {$pwidth > 1520} {set pwidth 1520}
    blt::graph $pwid -width $pwidth -height 600 -leftmargin 60
    pack $pwid -side top -expand yes -fill both

    $pwid axis create zy -hide 1 -min 0 -max 1
    $pwid axis configure x -command graphene::format_xlabel
    $pwid axis configure y -hide 1

    $pwid legend configure -activebackground white
    $pwid axis configure x -scrollcommand "$this scrollbar_set"

    $pwid marker create text -name logmes -background white \
        -foreground black -justify left -anchor w -mapy zy

    $pwid marker create polygon -name swpoutl -mapy zy \
        -fill "" -outline gray50 -linewidth 2 -under 1
    $pwid marker create text -name swptxt -mapy zy -anchor n \
        -background "" -foreground black


    init_comments_view
    init_comments


    # configure standard xBLT things:
    xblt::plotmenu $pwid -showbutton 1 -buttonlabel Menu\
       -buttonfont {Helvetica 12} -menuoncanvas 0
    xblt::legmenu  $pwid -showseparator 0

    yblt::hielems $pwid -usemenu 1
    xblt::crosshairs $pwid -variable v_crosshairs -usemenu 1
    xblt::measure $pwid -event <Key-equal> -usemenu 1\
          -quickevent <Alt-1>; # -command "$this message"
    xblt::readout $pwid -variable v_readout -usemenu 1\
          -active 1; # -eventcommand "1 $this message"
    xblt::zoomstack $pwid -scrollbutton 2  -usemenu 1 -axes x -recttype x

    # Additional rubberrect
    xblt::rubberrect::add $pwid -type x -modifier Shift \
      -configure {-outline blue} \
      -invtransformx x -command "$this show_rect" -cancelbutton ""

    # Configure shifting and scaling of data curves
    # - 2nd button on plot: shift and rescale
    # - 3rd button on legend: autoscale
    DataMod #auto $pwid;

    bind . <Key-F1> show_help
    bind . <Key-Insert> do_update
    bind . <Key-End>   "$this scroll_to_end"
    bind . <Key-Left>  "$this scroll_command scroll -1 units"
    bind . <Key-Right> "$this scroll_command scroll 1 units"
    bind . <Key-Prior> "$this scroll_command scroll -1 pages"
    bind . <Key-Next>  "$this scroll_command scroll 1 pages"
    bind . <ButtonPress-4> "$this scroll_command scroll -1 units"
    bind . <ButtonPress-5> "$this scroll_command scroll 1 units"

    bind . <Alt-Key-q>     "$this finish"
    bind . <Control-Key-q> "$this finish"
    wm protocol . WM_DELETE_WINDOW "$this finish"


#    wm title . "Stripchart"

    update idletasks
    #after idle {bindtags $pwid [bindtags $w]} ;# Obscur tk/blt bug workaround
  }

  destructor {
  }

  ######################################################################
  method message {args} {
    puts "$args"
  }
  method show_rect {graph x1 x2 y1 y2} {
    puts " rect selected $x1 -- $x2"
  }

  ######################################################################

  # add data source
  method add_data {args} {
    lappend data_sources [DataSource #auto $pwid {*}$args] }

  method add_comments {args} {
    set opts {
      -name    name    {}
      -conn    conn    {}
    }
    if {[catch {parse_options "graphene::viewer::add_fdata" \
      $args $opts} err]} { error $err }
    set comments [dict create\
      name $name\
      conn $conn\
    ]
  }

  method add_ranges {args} {
    set opts {
      -name    name    {}
      -conn    conn    {}
    }
    if {[catch {parse_options "graphene::viewer::add_fdata" \
      $args $opts} err]} { error $err }
    set ranges [dict create\
      name $name\
      conn $conn\
    ]
  }

  ######################################################################

  # update data
  method update_data {t1 t2 dt} {
    foreach d $data_sources {
      $d update_data $t1 $t2 $dt
    }
  }

############################################################


  #####################################
  # run the program


  method finish {} { exit }


  method tmin {} {
    set ret {}
    foreach d $data_sources {
      set mm [$d get_tmin]
      if {$ret eq {} || $ret > $mm} {set ret $mm}
    }
    return $ret
  }
  method tmax {} {
    set ret {}
    foreach d $data_sources {
      set mm [$d get_tmax]
      if {$ret eq {} || $ret < $mm} {set ret $mm}
    }
    return $ret
  }


  #####################################
  # scrolling

  method scrollbar_set {x1 x2} {
    $swid set $x1 $x2
    set N [winfo width $pwid]
#puts "$x1 $x2 $N"
#    foreach d $data_sources { $d update_data $x1 $x2 $N }
  }

  method scroll_command {args} {
    set tmin [$this tmin]
    set tmax [$this tmax]
    if {$tmin eq {} || $tmax eq {}} return
    foreach {view_min view_max} [$pwid axis limits x] break
    set xw [expr {double($view_max - $view_min)}]
    switch -exact [lindex $args 0] {
      moveto {
        set x [expr {[lindex $args 1]*($tmax - $tmin) + $tmin}]
        set view_min $x
        set view_max [expr {$x + $xw}]
      }
      scroll {
        set n [lindex $args 1]
        if {[string equal [lindex $args 2] units]} {
          set view_min [expr {$view_min + $n*$xw/20}]
          set view_max [expr {$view_max + $n*$xw/20}]
        } else {
          set view_min [expr {$view_min + $n*$xw}]
          set view_max [expr {$view_max + $n*$xw}]
        }
      }
    }
    if {$view_min < $tmin} {
      set view_max [expr {$tmin + $xw}]
      set view_min $tmin
    } elseif {$view_max > $tmax} {
      set view_min [expr {$tmax - $xw}]
      set view_max $tmax
    }
    $pwid axis configure x -min $view_min -max $view_max
  }

  method scroll_to_end {} {
    graphene::axlims $pwid x xmin xmax
    set tmax [$this tmax]
    $pwid axis configure x -min [expr {$tmax-$xmax+$xmin}] -max $tmax
  }

}
}

graphene::viewer viewer
set conn [graphene::open -db_path "."]

viewer add_data\
   -name     cpu_load.txt\
   -cnames   {"load 1m" "load 5m" "load 10m"}\
   -ctitles  {"CPU load, 10m average" "CPU load, 5m average" "CPU load, 1m average"}\
   -ccolors  {}\
   -cfmts    {%.3f %.3f %.3f}\
   -ncols    3

viewer add_data\
   -name     cpu_temp\
   -conn     $conn\
   -cnames   {temp}\
   -ctitles  {CPU Temperature}\
   -ccolors  {magenta}\
   -cfmts    {%.2f}\
   -ncols    1

viewer add_comments\
   -name cpu_comm.txt

viewer update_data 0 1 4096

