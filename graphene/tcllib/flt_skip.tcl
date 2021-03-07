# copy from https://github.com/slazav/data_filter

proc flt_skip {args} {
  ## parse options
  set col   0
  set maxn  100
  set maxt  0
  set minn  0
  set mint  0
  set noise 0
  set auto_noise 1

  foreach {n v} $args {
    switch -exact $n {
    --column {set col  $v}
    --maxn   {set maxn $v}
    --maxt   {set maxt $v}
    --minn   {set minn $v}
    --mint   {set mint $v}
    --noise  {set noise $v}
    --auto_noise {set auto_noise $v}
    default {error "Unknown option: $n"}
    }
  }

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


  ####
  # Noise level finder
  if {$auto_noise > 0} {

    # To calibrate noise we want to use $noise_n points
    # at the beginning of the main buffer. If the buffer is shorter,
    # we use a separate sliding buffer of $noise_n length.
    set noise_n 30;

    # One more thing in the storage: sliding buffer
    if {![info exists st(sbuf)]} { set st(sbuf) {} };

    # Add new point to the sliding buffer
    lappend st(sbuf) [lindex $data $col]
    set ns [llength $st(sbuf)]

    # Remove old points
    if {$ns > $noise_n} {
      set dn [expr {$ns - $noise_n - 1}]
      set st(sbuf) [lreplace $st(sbuf) 0 $dn]
      set ns [llength $st(sbuf)]
    }

    # In the beginning the sliding buffer can be shorter
    # then noise_n. We should use $ns below.

    # Collect squares of deviations of points from
    # their two neighbours.
    set dev {}
    for {set i 1} {$i<$ns-1} {incr i} {
      if {$n >= $ns} {
        # use main buffer
        set vm [lindex $st(buf) [expr $i-1]]
        set vp [lindex $st(buf) [expr $i+1]]
        set v0 [lindex $st(buf) $i]
        set dm [lindex [lindex $vm 1] $col]
        set dp [lindex [lindex $vp 1] $col]
        set d0 [lindex [lindex $v0 1] $col]
      }\
      else {
        # use sliding buffer
        set dm [lindex $st(sbuf) [expr $i-1]]
        set dp [lindex $st(sbuf) [expr $i+1]]
        set d0 [lindex $st(sbuf) $i]
      }
      lappend dev [expr ($d0-($dp+$dm)/2)**2]
    }

    # Skip three lagrest
    # values to reduce effect of steps and pikes.
    # Do not skip values if buffer is small
    set dev [lsort -real $dev]
    if {$ns == $noise_n} {
      set dev [lreplace $dev end-3 end]
    }

    # Calculate RMS deviation
    set sum 0
    set cnt 0
    foreach v $dev {
      set sum [expr {$sum + $v}]
      incr cnt
    }
    if {$cnt>1} {
      set sum [expr sqrt($sum/$cnt)]

      # Compensate removed points. Theory?!
      set sum [expr $sum*1.2]

      set noise [expr max($auto_noise*3*$sum, $noise)]
    }
  }



  # Data to be returned. We can't modify $data,$time
  # before adding them to the buffer. Use ret_t/ret_d to keep values.
  set ret 0
  set ret_t 0
  set ret_d 0


  # If we have previous point:
  if {$st(t0) ne {} && $st(d0) ne {}} {

    # If the buffer contains 3 or more points
    # and we can do 2-segment fit:
    if {$n>2} {

      # extract d-d0, t-t0 lists
      set dts {}; set dds {}
      foreach p $st(buf) {
        lappend dts [time_diff $st(t0) [lindex $p 0]]
        lappend dds [expr [lindex [lindex $p 1] $col]- $st(d0)]
      }

      # 2-segment fit: we want to find an internal point 0<j<n
      # with best RMS deviation from 2-segment line (t0,d0)-(tj,dj)-(tn,dn).
      # The only free parameter is index j.
      # The line formula:
      #    t<=tj: A*t,            A=$dj/$tj
      #    t>=tj: B*(t-tj) + dj,  B=($dn-dj)/($tn-$tj)
      # We will check all possible values of j and choose the best one.

      # best values:
      set opt_D Inf
      set opt_A Inf
      set opt_B Inf
      set opt_j 0
      # Last point (index n-1, not n!)
      set tn [lindex $dts [expr $n-1]]
      set dn [lindex $dds [expr $n-1]]
      for {set j 1} {$j<$n-1} {incr j} {
        set tj [lindex $dts $j]
        set dj [lindex $dds $j]
        set A [expr {$dj/$tj}]
        set B [expr {($dn-$dj)/($tn-$tj)}]

        # RMS deviation from the fit:
        set D 0.0
        foreach t $dts d $dds {
          if {$t <= $tj} {
            set D [expr {$D + ($A*$t - $d)**2}]
          } else {
            set D [expr {$D + ($B*($t-$tj) - ($d-$dj))**2}]
          }
        }
        set D [expr sqrt($D/($n-1))]

        # If we have flat noisy data than simple 2-segment fit
        # will give almost random position of the central point.
        # Without a good reason we do not want to have it close
        # to the beginning (to avoid too short segments), or to the end
        # (to avoid random slope of the second segment and bad evaluation
        # of the stopping condition).
        # I will multiply D by function f(j) = 1 + 2j(j-n-1)/(n-1)^2
        # (quadratic function with f(0) = f(n-1) = 1, f((n-1)/2) = 1/2 )
        set D [expr $D*(1 + 2.0*$j*($j-$n-1)/($n-1)**2)]

        if {$D <= $opt_D} { # "=" is important if you have many points with same values!
          set opt_D $D
          set opt_A $A
          set opt_B $B
          set opt_j $j
          set opt_tj $tj
        }
      }
      set j $opt_j
      set tj [lindex $dts $j]
      set dj [lindex $dds $j]
      set B $opt_B

      # Now we have a 2-segment fit with best RMS deviation.
      # We can reset buffer and send central point
      # to the output if some "stopping condition" is met.

      # There are a few stopping conditions
      # - too long buffer
      # - new point is too far from the fit
      # - max deviation in the fit is more then noise level

      # There is one more stopping condition: 4*$j < ($n-1-$j)
      # If first segment become very short it means there is
      # a feature there. No need to add more points and make
      # the second segment longer. This works after sharp steps or peaks.

      # Deviation of the new point (which is not in the buffer yet)
      set tx [time_diff $st(t0) $time]
      set dx [expr [lindex $data $col] - $st(d0)]
      set Q1 [expr {abs($dj + $B*($tx-$tj) - $dx)}]

      # Max absolute deviation of buffer points:
      set Q2 0
      foreach t $dts d $dds {
        if {$t <= $tj} {
          set v [expr {abs($A*$t - $d)}]
        } else {
          set v [expr {abs($B*($t-$tj) - ($d-$dj))}]
        }
        if {$Q2 < $v} {set Q2 $v}
      }

      # Calculate stopping condition:
      set stop 0
      if {($noise>0 && $Q1 > 2*$noise) ||\
          ($noise>0 && $Q2 > $noise) ||\
          (4*$j < ($n-1-$j)) ||\
          ($maxn>0 && $n > $maxn) ||\
          ($maxt>0 && $tn > $maxt)} {set stop 1}

      # Clear stopping condition if buffer is too short (--minn, --mint parameters)
      if {($minn>0 && $n < $minn) ||
          ($mint>0 && $tn < $mint)} {set stop 0}

      # let's send the point to output!
      if {$stop} {
        set ret 1
        set ret_t [lindex [lindex $st(buf) $tj] 0]
        set ret_d [lindex [lindex $st(buf) $tj] 1]
        # Crop beginning of the buffer, it should start with the sent point!
        set st(buf) [lreplace $st(buf) 0 $j-1];
        set st(d0) [lindex $ret_d $col]
        set st(t0) $ret_t
      }
    }
  }\
  else {
    # no previous point, 1st point ever
    set ret 1
    set ret_t $time
    set ret_d $data
    set st(d0) $ret_d
    set st(t0) $ret_t
  }

  # Add new point to the buffer
  lappend st(buf) [list $time $data]

  # Update data
  if {$ret} {
    set time $ret_t
    set data $ret_d
  }

  # Convert storage
  set ::storage [array get st]
  return $ret
}
