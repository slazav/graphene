package provide GrapheneMonitor 1.0
package require Itcl

namespace eval graphene {

######################################################################
itcl::class monitor_module {
  public variable tmin 1000;  # measurement period
  public variable tmax   {};  # maximum save period
  public variable atol   {};  # absolute tolerance (list)
  public variable rtol   {};  # relative tolerance (list)
  public variable dbname {};  # database name
  public variable dbcon  {};  # database connection
  public variable name   {};  # module name
  public variable vvar   {};  # global var name for the last measured value
  public variable verb   0;   # be verbose
  public variable rh     {};  # run handler
  variable v0 {}; # three last measured values
  variable v1 {};
  variable v2 {};
  variable dt  0; # time from previously saved point

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

    # shift old values, measure new one
    set v2 $v1
    set v1 $v0
    set v0 [get]

    # do we want to save the point?
    set save 1
    set dt [expr {$dt+$tmin}]
    if {$dt < $tmax &&\
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

    set res [join $v0 " "]
    set $vvar $res
    if {$save} {
      graphene::cmd $dbcon "put $dbname now $res"
      set dt 0
    }
    set rh [after $tmin $this start_run]
    if {$verb} {puts [list $this: del: $tmin save: $save val $res ]}
  }

  # stop the measurement
  method stop_run {} { after cancel $rh; set rh {} }

  # show pop-up window, edit module parameters, to be done
  method setup {} {
    if {$verb} {puts "$this: setup"}
    global setup1 setup2 setup3 setup4 setup5 setup6
    set setup1 [expr {$tmin / 1000.0}]
    set setup2 [expr {$tmax / 1000.0}]
    set setup3 [join $atol " "]
    set setup4 [join $rtol " "]
    set setup5 $dbname
    set setup6 $verb
    destroy .setup
    toplevel .setup
    grid [ label .setup.l0 -text $name ] -columnspan 2
    grid [ label .setup.l1 -text "measurement period, s:" ]\
         [ entry .setup.e1 -textvariable setup1]
    grid [ label .setup.l2 -text "max.save period, s:" ]\
         [ entry .setup.e2 -textvariable setup2 ]
    grid [ label .setup.l3 -text "absolute tolerance:" ]\
         [ entry .setup.e3 -textvariable setup3 ]
    grid [ label .setup.l4 -text "relative tolerance:" ]\
         [ entry .setup.e4 -textvariable setup4 ]
    grid [ label .setup.l5 -text "database name:" ]\
         [ entry .setup.e5 -textvariable setup5 ]
    grid [ label .setup.l6 -text "log events:" ]\
         [ checkbutton .setup.e6 -variable setup6 ]

    grid [ button .setup.b1 -text Apply  -state normal -command "$this close_setup 1" ]\
         [ button .setup.b2 -text Cancel -state normal -command "$this close_setup 0" ]
  }
  method close_setup {save} {
    if {$save} {
      global setup1 setup2 setup3 setup4 setup5 setup6
      set tmin   [expr {round($setup1 * 1000)}]
      set tmax   [expr {round($setup2 * 1000)}]
      set atol   [ regexp -all -inline {\S+} $setup3 ]
      set rtol   [ regexp -all -inline {\S+} $setup4 ]
      set dbname "$setup5"
      set verb   "$setup6"
    }
    destroy .setup
  }
}

######################################################################
itcl::class monitor {
  public variable verb  0;      # verbosity level
  public variable sync  60000;  # sync period
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
         -variable $cbv -command "$mod ch_state $$cbv" ]\
         [entry  .ent$i -textvariable $vvar ]\
         [button .btn$i -text Setup -command "$mod setup"]
  }

}

######################################################################
}