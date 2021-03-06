#!/usr/bin/tclsh

# Filter for skipping points.
# Arguments:
#   maxd - Noise level. Max difference between original and filtered data.
#   maxn - Max.filtering length, points (0 for no limit)
#   maxt - Max.filtering length, seconds (0 for no limit)
#   col  - Data column to use

proc flt_skip {maxd {maxn 0} {maxt 100} {col 0} {avrg 1}} {
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

  # Data to be returned. We can't modify $data,$time
  # before adding them o the buffer. Use ret_t/ret_d to keep values.
  set ret 0
  set ret_d 0
  set ret_t 0

  # If we have previous point:
  if {$st(t0) ne {} && $st(d0) ne {}} {

    # If the buffer contains 3 or more points
    # we can do 2-segment fit:
    if {$n>2} {

      # extract d-d0, t-t0 lists
      set dts {}; set dds {}
      foreach p $st(buf) {
        lappend dts [graphene_time_diff $st(t0) [lindex $p 0]]
        lappend dds [expr [lindex [lindex $p 1] $col]- $st(d0)]
      }

      # 2-segment fit: for each internal point 0<j<n
      # we have  A*t + (t>=tj)? B(t-tj):0
      # (t is measured from t0, d from d0)
      # We do the fit for all possible values of j and
      # choose the best one.

      # If we have flat noisy data than simple 2-segment fit
      # will give almost random position of the central point.
      # Without a good reason we do not want to have it close
      # to the beginning (to avoid too short segments), or to the end
      # (to avoid random slope of the second segment and bad evaluation
      # of the stopping condition).
      # I will multiply fit quality by function f(j) = 1 + 2j(j-n-1)/(n-1)^2
      # (quadratic function with f(0) = f(n-1) = 1, f((n-1)/2) = 1/2 )

      # best values:
      set opt_D Inf
      set opt_A Inf
      set opt_B Inf
      set opt_j 0
      set opt_tj 0
      for {set j 1} {$j<$n-1} {incr j} {
        set tj [lindex $dts $j]

        # Minimize sum_0..j (A*t - d)^2 + sum_j..n (A*t + B(t-tj) - d)^2:
        # a = sum(0..n)t^2
        # c = sum(0..n)t*d
        # b = d = sum(j..n)t(t-tj)
        # e = sum(j..n)(t-tj)(t-tj)
        # f = sum(j..n)(t-tj)*d
        # A = (ce-bf)/(ae-bd)
        # B = (af-cd)/(ae-bd)
        set a 0.0; set b 0.0; set c 0.0; set e 0.0; set f 0.0
        foreach dt $dts dd $dds {
          set a [expr {$a + $dt*$dt}]
          set c [expr {$c + $dt*$dd}]
          if {$dt <= $tj} continue
          set b [expr {$b + $dt*($dt-$tj)}]
          set e [expr {$e + ($dt-$tj)*($dt-$tj)}]
          set f [expr {$f + ($dt-$tj)*$dd}]
        }
        set det [expr $a*$e-$b*$b]
        if {$det == 0} continue

        set A [expr {($e*$c-$b*$f)/$det}]
        set B [expr {($a*$f-$b*$c)/$det}]

        # St.deviation from the fit
        set D 0.0
        foreach dt $dts dd $dds {
          if {$dt <= $tj} {
            set D [expr {$D + ($A*$dt - $dd)**2}]
          } else {
            set D [expr {$D + ($A*$dt + $B*($dt-$tj) - $dd)**2}]
          }
        }
        set D [expr sqrt($D/($n-1))]
        set D [expr $D*(1 + 2.0*$j*($j-$n-1)/($n-1)**2)]
        if {$D <= $opt_D} { # = is important if you have many points with same values!
          set opt_A $A
          set opt_B $B
          set opt_D $D
          set opt_j $j
          set opt_tj $tj
        }
      }

      # Now we have a 2-segment fit (A, B, tj) with
      # best st.deviation.
      # We can reset buffer and send central point
      # to the output based on the fit "quality",
      # deviation of the new point (which is not in the buffer yet).
      # With this method it is possible to catch small peaks.

      # There is one more stopping condition: 5*$j < ($n-1-$j)
      # If first segment become very short it means there is
      # A feature there. No need to add more points and make
      # the second segment longer. This works after sharp steps or pikes.

      set j $opt_j
      set tj $opt_tj
      set A $opt_A
      set B $opt_B
      set Q1 $opt_D

      set tn [graphene_time_diff $st(t0) $time]
      set dn [expr [lindex $data $col] - $st(d0)]
      set Q2 [expr {abs($A*$tn + $B*($tn-$tj) - $dn)}]

      if {$Q2 > $maxd ||\
          (5*$j < ($n-1-$j)) ||\
          ($maxn>0 && $n > $maxn) ||\
          ($maxt>0 && $dt > $maxt)} {
        set ret 1
        set ret_t [graphene_time_add $st(t0) $tj]
        set ret_d [expr {$A*$tj + $st(d0)}]

### debug: print 2-segment fits:
#puts "$st(t0) $st(d0)"
#puts "$ret_t $ret_d"
#puts "$time [expr {$A*$tn + $B*($tn-$tj) + $st(d0)}]"
#puts "$time [lindex $data $col]"
#puts ""

        set st(buf) [lreplace $st(buf) 0 $j-1]; # include j point to the new buffer!
        set st(d0) $ret_d
        set st(t0) $ret_t

        if {!$avrg} {
          set ret_d [lindex [lindex $st(buf) 0] 1]]
        }
      }
    }
  }\
  else {
    # no previous point, 1st point ever
    set ret 1
    set ret_t $time
    if {$avrg} {
      set ret_d [lindex $data $col]
    } else {
      set ret_d $data
    }
    set st(d0) $ret_d
    set st(t0) $ret_t
  }

  # add new point to the buffer
  lappend st(buf) [list $time $data]

  #update data
  if {$ret} {
    set time $ret_t
    set data $ret_d
  }

  # convert storage
  set ::storage [array get st]
#  return 0
  return $ret
}
