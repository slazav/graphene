# Calculate (small) difference between timestamps (t2-t1),
# keeping ns precision.
# - both timestamps should be positive numbers.
# - if difference is large then precision is lost
proc time_diff {t1 t2} {
  # split into seconds and fractional parts, do subtraction separately
  set t1l [split $t1 {.}]
  set t2l [split $t2 {.}]
  if {[llength $t1l]<2} {lappend t1l 0}
  if {[llength $t2l]<2} {lappend t2l 0}
  set dts [expr {[lindex $t2l 0] - [lindex $t1l 0]}]
  set dtf [expr {"0.[lindex $t2l 1]" - "0.[lindex $t1l 1]"}]
  return [format "%.9f" [expr {$dts + $dtf}]]
}

# Add (small, positive or negative) value to a graphene timestamp,
# keeping ns precision.
proc time_add {t dt} {
  # split into seconds and fractional parts
  set tl [split $t {.}]
  if {[llength $tl]<2} {lappend tl 0}

  set ts [lindex $tl 0]
  set tf [expr {"0.[lindex $tl 1]" + $dt}]
  set ts [expr {$ts + int($tf) - ($tf<0 ? 1:0)}]
  set tf [expr {$tf - int($tf) + ($tf<0 ? 1:0)}]
  return [format "%d.%09d" $ts [expr round(1e9*$tf)]]
}
