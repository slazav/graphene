#!/usr/bin/wish

package require Itcl
package require ParseOptions 1.0
package require xBlt 3
package require Graphene

source libs/data_source.tcl
source libs/comment_source.tcl
source libs/data_mod.tcl
source libs/scroll.tcl
source libs/hielems.tcl
source libs/readout.tcl
source libs/format_time.tcl

#source comments.tcl
#set ocomments 1

namespace eval graphene {

proc format_xlabel {w x} { time2str $x }

#proc axlims {graph ax var1 var2} {
#    set lims [$graph axis limits $ax]
#    uplevel 1 [list set $var1 [lindex $lims 0]]\n[list set $var2 [lindex $lims 1]]
#}


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

    scrollbar $swid -orient horizontal
    pack $swid -side bottom -fill x

    set swidth [winfo screenwidth .]
    set pwidth [expr {$swidth - 80}]
    if {$pwidth > 1520} {set pwidth 1520}
    blt::graph $pwid -width $pwidth -height 600 -leftmargin 60
    pack $pwid -side top -expand yes -fill both

    $pwid legend configure -activebackground white

    $pwid axis configure y -hide 1

    $pwid axis configure x -command graphene::format_xlabel

    $pwid axis create zy -hide 0 -min 0 -max 1
#    $pwid marker create text -name logmes -background white \
#        -foreground black -justify left -anchor w -mapy zy
#    $pwid marker create polygon -name swpoutl -mapy zy \
#        -fill "" -outline gray50 -linewidth 2 -under 1
#    $pwid marker create text -name swptxt -mapy zy -anchor n \
#        -background "" -foreground black


#    init_comments_view
#    init_comments


    # configure standard xBLT things:
    xblt::plotmenu $pwid -showbutton 1 -buttonlabel Menu\
       -buttonfont {Helvetica 12} -menuoncanvas 0
    xblt::legmenu  $pwid -showseparator 0

    yblt::hielems $pwid -usemenu 1
    xblt::crosshairs $pwid -variable v_crosshairs -usemenu 1
    xblt::measure $pwid -event <Key-equal> -usemenu 1\
          -quickevent <Alt-1>; # -command "$this message"
    yblt::readout $pwid -variable v_readout -usemenu 1\
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
    Scroll  #auto $pwid $swid;
#    CommentSource #auto $pwid;

    bind . <Key-F1> show_help
    bind . <Key-Insert> do_update

    bind . <Alt-Key-q>     "$this finish"
    bind . <Control-Key-q> "$this finish"
    wm protocol . WM_DELETE_WINDOW "$this finish"


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
   -name     comp_temp\
   -conn     $conn\
   -cnames   {temp}\
   -ctitles  {CPU Temperature}\
   -ccolors  {magenta}\
   -cfmts    {%.2f}\
   -ncols    1

viewer add_comments\
   -name cpu_comm.txt

viewer update_data 0 1 10240


package require Iwidgets 4.0
source mv_comments.tcl
#comments::add_comment 1 .p .

set xl [viewer tmin]
set xh [viewer tmax]
#.p marker create polygon -name swpoutl -mapy zy \
#  -fill gray50 -outline {} -linewidth 2 -under 1
#.p marker configure swpoutl -coords \
#  [list $xl 0 $xl 0.02 $xh 0.02 $xh 0 $xl 0]

.p marker create line -name dd -mapy zy \
  -outline grey80 -linewidth 5 -under 1
.p marker configure dd -coords [list $xl 0 $xh 0]

