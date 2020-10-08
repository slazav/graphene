# Input filter: averaging/skipping data.
#
# Data is skipped/averaged until reset condition is met.
# When a value is sent to the database.
#
# pars -- parameter array with following optional fields:
# * avrg    -- Do average or skip data. Default: 1
#              On the first step time and values (t,v) are saved as (t0,v0).
#              On each next step if reset condition is not met:
#                avrg=1: average all received times and values into (tx,vx).
#                avrg=0: use (t0,v0) as (tx,vx)
#                Data point is skipped.
#              If reset condition is met, (t0,v0) or (ta,va) is sent to
#              the database, (t,v) is saved as (t0,v0).
#
# * tdiff   -- Maximum time between points [s]. Default: 60
#              Filter resets if t-t0<0 || (tdiff>0 && t-t0>tdiff).
#
# * maxn    -- Max number of points which can be skipped. Default: 0
#              Filter resets if maxn>0 && n>=maxn (where n - number
#              of collected points since last reset).
#
# * adiff   -- Max absolute difference between point. Default 0.
#              Filter resets if adiff>0 && adist(v,vx)>adiff
#              Distance between data lists is calculated as
#              adist(v,vx) = sqrt(sum((v[i]-vx[i])^2)).
#
# * rdiff   -- Max relative difference between point. Default 0.
#              Filter resets if rdiff>0 && rdist(v,vx)>rdiff
#              dist(v,vx) = sqrt(sum(2*(v[i]-vx[i])^2/(v[i]+vx[i])^2))
#              (if v[i]+vx[i] == 0 for any i, then rdist=+inf and
#              the reset condition is met).
#
# * dcol    -- if dcol>=0, then use only this column to calculate distances
#              for adiff/rdiff reset condition
#

proc flt_skip_avrg {} {
  # Global variables provided by graphene:
  global data; # list of data values
  global time; # list of time values
  global storage; # storage which is kept between calls
  global pars;
  array set st $storage

  # Default parameters:

  set avrg  [expr {[info exists pars(avrg)]?  $pars(avrg):   1}]
  set tdiff [expr {[info exists pars(tdiff)]? $pars(tdiff): 60}]
  set maxn  [expr {[info exists pars(maxn)]?  $pars(maxn):   0}]
  set adiff [expr {[info exists pars(adiff)]? $pars(adiff):  0}]
  set rdiff [expr {[info exists pars(rdiff)]? $pars(rdiff):  0}]
  set dcol  [expr {[info exists pars(dcol)]?  $pars(dcol):  -1}]

  set tx {}
  set vx {}
  while 1 {
    # No (t0,v0) in the storage (probably, the first run)
    if {! [info exists st(t0)] ||
        ! [info exists st(v0)] ||
        ! [info exists st(n)]} {set reset 1; break}

    # Set tx,vx:
    # avrg=1: average value (excluding current point), t0+tsum/n, v0+vsum/n
    # avrg=0: t0,v0
    # We use avrg value from the st. If it's different from one from parameters,
    # tx,vx will be calculated correctly and returned, reset with new avrg value
    # will be done
    if {$st(avrg)} {
      if {! [info exists st(tsum)] ||
          ! [info exists st(vsum)] ||
          [llength $st(vsum)] != [llength $st(v0)] } { set reset 1; break}

      set tx [graphene_time_add $st(t0) [expr $st(tsum)/$st(n)]]
      set vx {}
      foreach xs $st(vsum) x0 $st(v0) {
        lappend vx [expr {$x0 + $xs/$st(n)}]
      }
    }\
    else {
      set tx $st(t0)
      set vx $st(v0)
    }

    # No avrg in the storage, or it is different from parameters
    if {! [info exists st(avrg)] ||
          $st(avrg) != $avrg} {set reset 1; break}

    # Time difference with t0.
    set dt [graphene_time_diff $st(t0) $time]
    if { $dt<0 || ($tdiff>0 && $dt>$tdiff)} {set reset 1; break}

    # Number of points
    if {$maxn>0 && $st(n) >= $maxn} {set reset 1; break}

    # Check if old array has same size
    if {[llength $vx] != [llength $data]} { set reset 1; break}

    # Calculate absolute and relative distance between data and vx
    if {$dcol<0} {
      set adist 0
      set rdist 0
      foreach x $vx v $data {
        set adist [expr {($v - $x)**2}]
        if {$v + $x == 0} { set rdist "inf" }
        if {$rdist ne "inf"} {
          set rdist [expr {2*($v - $x)**2/($v + $x)**2}] }
      }
      set adist [expr {sqrt($adist)}]
      if {$rdist ne "inf"} {
        set rdist [expr {sqrt($rdist)}]
      }
    }\
    else {
      set x [lindex $vx   $dcol]
      set v [lindex $data $dcol]
      set adist [expr {abs($v - $x)}]
      if {$v + $x == 0} { set rdist "inf" }\
      else { set rdist [expr {2*($v - $x)**2/($v + $x)**2}] }
    }

    if {$adiff > 0 && $adiff < $adist} { set reset 1; break}
    if {$rdiff > 0 && ($rdist eq "inf" || $rdiff < $rdist)} { set reset 1; break}

    set reset 0
    break
  }

  if {$reset} {
    # Reset storage, return old data if available
    set st(n) 1
    set st(t0) $time
    set st(v0) $data
    set st(avrg) $avrg
    if {$avrg} {
      set st(tsum) 0
      set st(vsum) {}
      foreach d $data { lappend st(vsum) 0 }
    }
    set time $tx
    set data $vx
    set ret [expr {$tx ne {} && $vx ne {}}]
  }\
  else {
    # update storage, return false
    incr st(n)
    if {$avrg} {
      set st(tsum) [expr {$st(tsum) + $dt}]
      for {set i 0} {$i<[llength $data]} {incr i} {
        set s  [lindex $st(vsum) $i]
        set d  [lindex $data $i]
        set d0 [lindex $st(v0) $i]
        lset st(vsum) $i [expr {$s + $d - $d0}]
      }
    }
    set ret 0
  }
  set storage [array get st]
  return $ret
}
