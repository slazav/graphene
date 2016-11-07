package require Itcl
package require ParseOptions 1.0
package require BLT

# connects a scrollbar with a blt graph


itcl::class Scroll {
  variable blt_plot
  variable scrollbar

  ######################################################################
  constructor {plot sbar args} {
    set blt_plot  $plot
    set scrollbar $sbar

    $scrollbar configure -orient horizontal -command "$this scroll_command"
    $blt_plot  axis configure x -scrollcommand "$this scrollbar_set"

    bind . <Key-End>   "$this scroll_to_end"
    bind . <Key-Left>  "$this scroll_command scroll -1 units"
    bind . <Key-Right> "$this scroll_command scroll 1 units"
    bind . <Key-Prior> "$this scroll_command scroll -1 pages"
    bind . <Key-Next>  "$this scroll_command scroll 1 pages"
    bind . <ButtonPress-4> "$this scroll_command scroll -1 units"
    bind . <ButtonPress-5> "$this scroll_command scroll 1 units"
  }

  ######################################################################

  variable oxmin {}
  variable oxmax {}

  method set_time_scale {} {
    axlims xmin xmax
    # if limits didn't change - return
    if {$xmin==$oxmin && $xmax==$oxmax} return

    set oxmin $xmin
    set oxmax $xmax

    # calculate size of various time units in pixels:
    set w [winfo width $blt_plot]
    set w [expr {$w-140}];  # not accurate, size of legend + vertical axis
    set SS [expr {1000*$w/($xmax - $xmin)}]
    set MM [expr {$SS*60}]
    set HH [expr {$MM*60}]
    set dd [expr {$HH*24}]
    set ww [expr {$dd*7}]
    set mm [expr {$dd*30}]
    set yy [expr {$dd*365}]
puts "::: [expr {($xmax - $xmin)/$w/1000}]"
puts "::: $yy $mm $dd $HH $MM $SS $w"

    if { $SS > 10000 } {
#      $blt_plot axis configure x -stepsize 10
#      $blt_plot axis configure x -subdivisions 10
      return
    }

    if {$SS > 1000.0 } {
      $blt_plot axis configure x -stepsize 100 -majorticks {}
      $blt_plot axis configure x -subdivisions 10\
                                 -command "format_time {%Y-%m-%d%n%H:%M:%S}"
      return
    }
    if {$SS > 100.0 } {
      $blt_plot axis configure x -stepsize 1000 -majorticks {}
      $blt_plot axis configure x -subdivisions 10\
                                 -command "format_time {%Y-%m-%d%n%H:%M:%S}"
      return
    }
    if {$SS > 10 } {
      $blt_plot axis configure x -stepsize 10000 -majorticks {}
      $blt_plot axis configure x -subdivisions 10\
                                 -command "format_time {%Y-%m-%d%n%H:%M:%S}"
      return
    }
    if {$MM > 100 } {
      $blt_plot axis configure x -stepsize 60000 -majorticks {}
      $blt_plot axis configure x -subdivisions 6\
                                 -command "format_time {%Y-%m-%d%n%H:%M}"
      return
    }
    if {$MM > 10 } {
      $blt_plot axis configure x -stepsize [expr {600*1000}] -majorticks {}
      $blt_plot axis configure x -subdivisions 10\
                                 -command "format_time {%Y-%m-%d%n%H:%M}"
      return
    }
    if {$HH > 100 } {
      $blt_plot axis configure x -stepsize [expr {3600*1000}] -majorticks {}
      $blt_plot axis configure x -subdivisions 6\
                                 -command "format_time {%Y-%m-%d%n%H:%M}"
      return
    }
    if {$dd > 100 } {
      # build list with days. We can not use -stepsize [expr {86400*1000}]
      # because of local time shift
      set d [clock format [expr {int($xmin/1000)}] -format "%Y-%m-%d"]
      set d [clock scan $d]
      set dl {}
      while {$d < $xmax/1000} {
        set d [expr $d+86400]
        lappend dl [expr $d*1000]
      }
      $blt_plot axis configure x -majorticks $dl -command "format_time {%Y-%m-%d}"
      return
    }
    if {$ww > 100 } {
      # build list with weeks. We can not use -stepsize [expr {7*86400*1000}]
      # because of local time shift
      set d [clock format [expr {int($xmin/1000)}] -format "%Y-%m-%d"]
      set w [clock format [expr {int($xmin/1000)}] -format "%u"]; # weekday
      set d [clock scan $d]; # beginning of the day
      set d [expr {$d - ($w-1)*86400}]; # go to Monday
      set dl {}
      while {$d < $xmax/1000} {
        set d [expr $d+7*86400]
        lappend dl [expr $d*1000]
      }
      $blt_plot axis configure x -majorticks $dl -command "format_time {%Y-%m-%d}"
      return
    }
    if {$mm > 100 } {
      # build list of months
      set d [clock format [expr {int($xmin/1000)}] -format "%Y-%m"]
      set d [clock scan "$d-01"]; # beginning of the month
      set dl {}
      while {$d < $xmax/1000} {
        set d [expr $d+32*86400]
        set d [clock format $d -format "%Y-%m"]
        set d [clock scan "$d-01"]; # beginning of the month
        lappend dl [expr $d*1000]
      }
      $blt_plot axis configure x -majorticks $dl -subdivisions 31 -command "format_time {%Y-%m}"
      return
    }
    if {$yy > 100 } {
      # build list of years
      set d [clock format [expr {int($xmin/1000)}] -format "%Y"]
      set d [clock scan "$d-01-01"]; # beginning of the year
      set dl {}
      while {$d < $xmax/1000} {
        set d [expr $d+367*86400]
        set d [clock format $d -format "%Y"]
        set d [clock scan "$d-01-01"]; # beginning of the year
        lappend dl [expr $d*1000]
      }
      $blt_plot axis configure x -majorticks $dl -command "format_time %Y"
      return

    }
      $blt_plot axis configure x -stepsize 0
      $blt_plot axis configure x -subdivisions 0

  }

  method scrollbar_set {x1 x2} {
    set_time_scale
    $scrollbar set $x1 $x2
  }

  method scroll_command {args} {

    axlims xmin xmax
    sblims smin smax

    set k [expr {($xmax-$xmin)/($smax-$smin)}]
    set xw [expr {$xmax - $xmin}]

    switch -exact [lindex $args 0] {
      moveto {
        set s [lindex $args 1]
        if {$smax >= 1 && $s > $smin} return
        if {$smin <= 0 && $s < $smin} return
        if {$smax <= $smin} return
        set k [expr {($xmax-$xmin)/($smax-$smin)}]
        set xmin [expr {$xmin + ($s-$smin)*$k}]
        set xmax [expr {$xmax + ($s-$smin)*$k}]
      }
      scroll {
        set n [lindex $args 1]
        if {[string equal [lindex $args 2] units]} {
          set xmin [expr {$xmin + $n*$xw/20.0}]
          set xmax [expr {$xmax + $n*$xw/20.0}]
        } else {
          set xmin [expr {$xmin + $n*$xw}]
          set xmax [expr {$xmax + $n*$xw}]
        }
      }
    }
    $blt_plot axis configure x -min $xmin -max $xmax
  }

  method scroll_to_end {} {
    axlims xmin xmax
    sblims smin smax
    set k [expr {($xmax-$xmin)/($smax-$smin)}]
    set xmin [expr {$xmin + (1-$smax)*$k}]
    set xmax [expr {$xmax + (1-$smax)*$k}]
    $blt_plot axis configure x -min $xmin -max $xmax
  }

  method axlims {var1 var2} {
    set lims [$blt_plot axis limits x]
    uplevel 1 [list set $var1 [lindex $lims 0]]\n[list set $var2 [lindex $lims 1]]
  }
  method sblims {var1 var2} {
    set lims [$scrollbar get]
    uplevel 1 [list set $var1 [lindex $lims 0]]\n[list set $var2 [lindex $lims 1]]
  }
}

