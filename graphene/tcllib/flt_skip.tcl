#!/usr/bin/tclsh

# Filter for skipping points.
# Arguments:
#   maxd - Noise level. Max difference between original and filtered data.
#   maxn - Max.filtering length, points (0 for no limit)
#   maxn - Max.filtering length, seconds (0 for no limit)
#   col  - Data column to use

proc flt_skip {maxd {maxn 0} {maxt 100} {col 0}} {
  global time data

  # Convert storage to array:
  array set st $::storage

  # What do we have in the storage:
  #  - buf: data buffer list of {t d(col)}
  #  - t0, d0 - previously added point
  if {![info exists st(buf)]} { set st(buf) {} };
  if {![info exists st(t0)]} { set st(t0) {} };
  if {![info exists st(d0)]} { set st(d0) {} };
  set n [llength $st(buf)]

  # do we want to print a point?
  set ret 0
  set ret_d {}; # if differs from time/data
  set ret_t {};

  # We have previous point
  if {$st(t0) ne {} && $st(d0) ne {}} {

    # If the buffer contains 1 or more points
    # we can do linear fit:
    if {$n>0} {

      # extract d-d0, t-t0 lists
      set dts {}; set dds {}
      foreach p $st(buf) {
        lappend dts [graphene_time_diff $st(t0) [lindex $p 0]]
        lappend dds [expr [lindex $p 1] - $st(d0)]
      }

      # Fit buffer with a line going through the last point (t0,d0):
      # Minimize sum(A*(t-t0) - (d-d0))^2:
      # A = sum[(t-t0)*(d-d0)]/[sum(t-t0)^2]:
      set s1 0.0; set s2 0.0
      foreach dt $dts dd $dds {
        set s1 [expr {$s1 + $dt*$dd}]
        set s2 [expr {$s2 + $dt*$dt}]
      }
      if {$s2 > 0} {
        set A [expr {$s1/$s2}]

        # Calculate deviation of the new point
        set dt [graphene_time_diff $st(t0) $time]
        set dd [expr [lindex $data $col]-$st(d0)]
        set D [expr {abs($A*$dt - $dd)} ]

        # If current point is far from the fit,
        # return point at the last buffer time,
        # clear buffer
        if {$D > $maxd ||\
            ($maxn>0 && $n > $maxn) ||\
            ($maxt>0 && $dt > $maxt)} {
          set ret 1
          set ret_t [lindex [lindex $st(buf) end] 0]
          set dt [graphene_time_diff $st(t0) $ret_t]
#         set ret_d [expr {$dt*$A + $st(d0)}]
          set ret_d [lindex [lindex $st(buf) end] 1]
          set st(d0) $ret_d
          set st(t0) $ret_t
          set st(buf) {}
        }
      }
    }
  }\
  else {
    # no previous point, 1st point ever
    set ret 1
    set st(d0) [lindex $data $col]
    set st(t0) $time
  }

  # Add new value to the data buffer
  lappend st(buf) [list $time [lindex $data $col]]

  # modify returned point if needed
  if {$ret_d ne {}} {set data $ret_d}
  if {$ret_t ne {}} {set time $ret_t}

  # convert storage
  set ::storage [array get st]
  return $ret
}
