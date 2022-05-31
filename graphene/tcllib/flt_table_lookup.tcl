# Table lookup with linear interpolation.
# Arguments:
#   tab -- even-sized array {x1 y1 x2 y3 ...}, monotonic on x
#   x   -- input value
#   log -- values in the table are log10(x) instead of x
# Return:
#   y   -- output value

proc table_lookup {tab x {log 0}} {
  if {$log} {
    if {$x<=0} { return {NaN} }
    set x [expr log($x)/log(10.0)]
  }
  set x1 [lindex $tab 0]
  set y1 [lindex $tab 1]
  if {$x != $x} {return {NaN}}
  if {$x == $x1} { return $y1 }
  foreach {x2 y2} $tab {
    if {($x>$x1 && $x<=$x2) || ($x<$x1 && $x>=$x2) } {
      if {$x1!=$x1 || $x2!=$x2} {return {NaN}}
      if {$y1!=$y1 || $y2!=$y2} {return {NaN}}
      return [expr $y1 + {($x-$x1)/($x2-$x1)*($y2-$y1)}]
    }
    set x1 $x2
    set y1 $y2
  }
  return {NaN}
}


# Table lookup output filter. Get a single column from data,
# apply calibraition according with the table and replace the
# whole data array with it.
#
# Arguments:
#  tab -- even-sized array {x1 y1 x2 y3 ...}, monotonic on x
#  col -- which data column to use (default 0)
#  log -- values in the table are log10(x) instead of x
#
proc flt_table_lookup {tab {col 0} {log 0}} {
  global data
  set data [table_lookup $tab [lindex $data $col] $log]
  return
}
