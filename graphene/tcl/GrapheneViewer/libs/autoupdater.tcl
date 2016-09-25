package require Itcl
package require ParseOptions 1.0

# Class runs a function regularly.
#
# Options:
#  -state_var     -- variable which switches autoupdater
#  -update_proc   -- function to be run regularly
#  -interval      -- interval, ms
#
# slazav, 2016-09-20


itcl::class autoupdater {

  variable update_id;    # updater handler
  variable state;        # internal variable for autoupdater status
  variable interval;     # internal variable for update interval, ms
  variable state_var;    # variable for autoupdater status
  variable int_var;      # variable for update interval, ms
  variable update_proc;  # update function

  ######################################################################
  constructor {args} {
    set state     1
    set interval  1000
    set update_id {}

    # parse options
    set opts {
      -state_var   state_var   state
      -update_proc update_proc test_proc
      -int_var     int_var     interval
    }
    if {[catch {parse_options "autoupdater" \
      $args $opts} err]} { error $err }

    # trace control variables
    trace add variable $state_var write "$this auto_update"
    trace add variable $int_var   write "$this auto_update"

    # run auto update
    auto_update {} {} {}
  }

  method auto_update {name1 name2 op} {
    after cancel $update_id

    set s [set $state_var]
    set i [set $int_var]

    if { $s } {
      $update_proc
      set update_id [after $i "$this auto_update {} {} {}"]
    }
  }

  # default update proc
  method test_proc {} {
    puts "update $this, interval [set $int_var]"
  }

  # get/set methods
  method get_interval {}  { return [set $int_var] }
  method set_interval {v} {
    puts "-- $v [set $int_var]"
    return [set $int_var $v] }

  method get_state {}  { return [set $state_var] }
  method set_state {v} { return [set $state_var $v] }

}

