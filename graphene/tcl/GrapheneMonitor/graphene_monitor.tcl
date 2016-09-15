package provide GrapheneMonitor 1.1
package require Itcl
package require xBlt 3
package require Prectime


namespace eval graphene {

######################################################################
itcl::class monitor_module {
  public variable tmin    1;  # measurement period
  public variable dbname {};  # database name
  public variable dbcon  {};  # database connection
  public variable name   {};  # module name
  public variable vvar   {};  # global var name for the last measured value
  public variable verb   0;   # be verbose
  public variable rh     {};  # run handler

  public variable filter  1;  # data filtering?
  public variable tmax   {};  # maximum save period
  public variable atol   {};  # absolute tolerance (list)
  public variable rtol   {};  # relative tolerance (list)

  public variable sweeper 0;  # is this a sweeper module?
  public variable s0      0;  # sweep parameter
  public variable ds      0;  # sweep step
  public variable smin    0;  # sweep lower limit
  public variable smax    0;  # sweep upper limit
  public variable sdir    0;  # sweep direction
  public variable srun    0;  # run/stop sweep

  public variable cnames {};  # column names
  public variable nplot 500; # max number of points to be plot
  variable v0 {}; # three last measured values
  variable v1 {};
  variable v2 {};
  variable dt  0; # time from previously saved point
  variable tim {}; # time array for the plot
  variable dat {}; # data arrays for the plot
  variable w {};

  # Set module state. This function is run from the
  # UI checkbox when its state changes.
  method ch_state {s} {
    if {$s} {
      if {$verb} {puts "start $this"}
      start
      start_run
    } else {
      stop_run
      stop
      if {$verb} {puts "stop $this"}
    }
  }

  # methods to be set in module implementation
  method get   {} {}
  method start {} {}
  method stop  {} {}

  # run get method regularly, save data
  method start_run {} {
    global $vvar

    # sweeper: make a step
    if {$sweeper !=0 && $srun != 0} {
      set s0 [expr {$s0 + $ds*$sdir}];
      if {$s0 > $smax} {set sdir -1}
      if {$s0 < $smin} {set sdir +1}
    }

    # shift old values, measure new one
    set v2 $v1
    set v1 $v0
    set v0 [get]

    # data filtering: do not save point if change was small
    set save 1
    if {$filter!=0} {
      set dt [expr {$dt + int($tmin*1000)}]
      if {$dt < $tmax*1000 &&\
          [llength $v0] <= [llength $v1] &&\
          [llength $v0] <= [llength $v2] } {

        set rsave 0
        for {set i 0} {$i < [llength $v0]} {incr i} {
          set v0i [lindex $v0 $i]
          set v1i [lindex $v1 $i]
          set v2i [lindex $v2 $i]
          set dv  [expr {abs($v0i+$v2i-2*$v1i)}]
          set at 0
          set rt 0
          if { [ llength $atol] > $i } {set at [lindex $atol $i]}
          if { [ llength $rtol] > $i } {set rt [lindex $rtol $i]}
          set t [expr max($at, $rt*abs($v0i))]
          if { $dv >= $t } {set rsave 1}
        }
        if {$rsave == 0} {set save 0}
      }
    }

    # save data for plotting
    if {[llength $v0] == [llength $cnames]} {
      #create data vectors if needed
      if {[llength $cnames] != [llength $dat]} {
        set tim [blt::vector create #auto]
        set dat {}
        for {set i 0} {$i < [llength $cnames]} {incr i} {
          lappend dat [blt::vector create #auto]
          puts "[lindex dat $i] $i"
        }
      }
      # append values:
      foreach D $dat V $v0 { $D append $V }
      $tim append [prectime]
      # remove old values:
      if {[$tim length] > $nplot} {
        foreach D $dat {$D delete 0:99}
        $tim delete 0:99
      }
    }

    # save data in the database
    set res [join $v0 " "]
    set $vvar $res
    if {$save} {
      graphene::cmd $dbcon "put $dbname now $res"
      set dt 0
    }

    # run the next iteration
    set rh [after [expr int(1000*$tmin)] $this start_run]
    if {$verb} {puts [list $this: del: $tmin save: $save val $res ]}
  }

  # stop the measurement
  method stop_run {} { after cancel $rh; set rh {} }

  # show pop-up window, edit module parameters
  method setup {} {
    if {$verb} {puts "$this: open setup window"}
    destroy .setup
    toplevel .setup

    # general parameters
    grid [label  .setup.name -text $name -padx 10 -pady 5] -sticky w
    grid [labelframe .setup.main -text "General parameters" -padx 10 -pady 10] -sticky ew
    grid [ label .setup.main.l2 -text "database name:" ]\
         [ label .setup.main.e2 -textvariable [itcl::scope dbname] ] -sticky ne
    grid [ label .setup.main.l1 -text "period, s:" ]\
         [ entry .setup.main.e1 -textvariable [itcl::scope tmin]] -sticky ne
    grid [ label .setup.main.l3 -text "log events:" ] -sticky ne
    grid [ checkbutton .setup.main.e6 -variable [itcl::scope verb] ]  -row 2 -column 1 -sticky nw

    # filter parameters
    if {$filter!=0} {
      grid [labelframe .setup.filter -text "Filter parameters" -padx 10 -pady 10] -sticky ew
      grid [ label .setup.filter.l1 -text "max.save period, s:" ]\
           [ entry .setup.filter.e1 -textvariable [itcl::scope tmax] ] -sticky ne
      grid [ label .setup.filter.l2 -text "absolute tolerance:" ]\
           [ entry .setup.filter.e2 -textvariable [itcl::scope atol] ] -sticky ne
      grid [ label .setup.filter.l3 -text "relative tolerance:" ]\
           [ entry .setup.filter.e4 -textvariable [itcl::scope rtol] ] -sticky ne
    }

    # sweep parameters
    if {$sweeper!=0} {
      grid [labelframe .setup.sweep -text "Sweep parameters" -padx 10 -pady 10] -sticky ew
      grid [ label .setup.sweep.sl1 -text "value:" ]\
           [ label .setup.sweep.se1 -textvariable [itcl::scope s0] ] -sticky ne
      grid [ label .setup.sweep.sl2 -text "direction:" ]\
           [ label .setup.sweep.se2 -textvariable [itcl::scope sdir] ] -sticky ne
      grid [ label .setup.sweep.sl3 -text "low limit:" ]\
           [ entry .setup.sweep.se3 -textvariable [itcl::scope smin]] -sticky ne
      grid [ label .setup.sweep.sl4 -text "high limit:" ]\
           [ entry .setup.sweep.se4 -textvariable [itcl::scope smax] ] -sticky ne
      grid [ label .setup.sweep.sl5 -text "step:" ]\
           [ entry .setup.sweep.se5 -textvariable [itcl::scope ds] ] -sticky ne

      grid [ frame .setup.sweep.btns ] -sticky e -columnspan 2
      grid [ button .setup.sweep.btns.sb1 -text "Start/Stop" -state normal -command "$this chrun" ]\
           [ button .setup.sweep.btns.sb2 -text "Change dir" -state normal -command "$this chdir" ]\

    }

    grid [ button .setup.b1 -text Close -state normal -command "destroy .setup" ] -sticky e
  }

  method btn_name {} {if {$srun!=0} {return Stop} else {return Start}}
  method chrun {} {set srun [expr {$srun==0}]}
  method chdir {} {set sdir [expr {-$sdir}]}


  # show pop-up window with a plot
  method plot {} {
    if {$verb} {puts "$this: open plot window"}
    destroy .plot
    toplevel .plot
    grid [ label .plot.name -text $name ]
    set w [blt::graph .plot.graph -highlightthickness 0 -bufferelements 0]

    $w element create data1 -symbol {} -linewidth 2 -color red
    $w element create data2 -symbol {} -linewidth 2 -color blue
#    $w element create last1 -symbol circle -pixels 5 -linewidth 0 -color red
#    $w element create last2 -symbol circle -pixels 5 -linewidth 0 -color blue
    $w legend configure -hide 1

#    xblt::crosshairs $w
#    xblt::measure $w
#    xblt::zoomstack $w -scrollbutton 2
#    xblt::readout $w

    set xsel $tim
    set ysel [lindex $dat 0]

    set xsel [lindex $dat 0]
    set ysel [lindex $dat 1]
    set zsel [lindex $dat 2]

    if {$xsel != ""} {
      set xl [$xsel index end]
      $w element configure data1 -xdata $xsel
      $w element configure data2 -xdata $xsel
#      $w element configure last1 -xdata $xl
#      $w element configure last2 -xdata $xl
#      $w axis configure x -title $vardata($xsel,l)
      if {[string compare $xsel tim] == 0} {
        $w axis configure x -command fmt_time
      } else {
      $w axis configure x -command {}
      }
    }
    if {$ysel != ""} {
      set yl [$ysel index end]
      $w element configure data1 -ydata $ysel
#      $w element configure last1 -ydata $yl
      $w axis configure y -title TITLE
    }

    if {$zsel != ""} {
      set zl [$zsel index end]
      $w element configure data2 -ydata $zsel
#      $w element configure last2 -ydata $zl
    }

    grid $w
    grid [ frame .plot.btns ] -sticky e
    grid [ button .plot.btns.b1 -text Clear -state normal -command "$this clear_plot" ]\
         [ button .plot.btns.b2 -text Close -state normal -command "destroy .plot" ] -sticky e
  }
  method clear_plot {} {
    foreach D $dat V $v0 { $D delete 0:end }
    $tim delete 0:end
  }

}

######################################################################
itcl::class monitor {
  public variable verb  0;      # verbosity level
  public variable sync  10000;  # sync period
  variable dbcon   {}; # database connection
  variable modules {}; # modules
  variable sh;         # sync handler

  #regular db sync
  method do_sync {} {
    set sync_needed 0
    foreach m $modules {
      if { [$m cget -rh] != "" } {set sync_needed 1}
    }
    if {$sync_needed} {
      if {$verb} { puts "sync DB" }
      graphene::cmd $dbcon sync
    }
    set sh [after $sync $this do_sync]
  }

  constructor { dbcon_ } {
    set dbcon $dbcon_
    do_sync; # start regular db sync
  }
  destructor {
    if {$verb} { puts "close DB connection" }
    after cancel $sh; # stop db sync
    foreach m $modules { itcl::delete object $m }
    graphene::close dbcon
  }

  method add_module {mod} {
    set i [llength $modules]
    lappend modules $mod
    set cbv  cb$i
    set vvar ent$i
    $mod configure -dbcon "$dbcon" -vvar "$vvar" -verb "$verb"
    grid [checkbutton .cb$i -text [$mod cget -name]\
         -variable $cbv -command "$mod ch_state $$cbv" ] -sticky nw -column 0 -row $i
    grid [entry  .ent$i -textvariable $vvar ] -sticky nw -column 1 -row $i
    grid [button .sbtn$i -text Setup -command "$mod setup"] -sticky nwe -column 2 -row $i
    grid [button .pbtn$i -text Plot  -command "$mod plot"] -column 3 -row $i
  }

}

######################################################################
}