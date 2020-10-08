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

  set x [lindex $data $col]
  if {$log} { set x [expr log($x)/log(10.0)] }

  set x1 [lindex tab 0]
  set y1 [lindex tab 1]
  foreach {x2 y2} $tab {
    if {($x>$x1 && $x<=$x2) || ($x<$x1 && $x>=$x2) } {
      set data [expr $y1 + {($x-$x1)/($x2-$x1)*($y2-$y1)}]
      return
    }
    set x1 $x2
    set y1 $y2
  }
  set data {NaN}
  return
}
