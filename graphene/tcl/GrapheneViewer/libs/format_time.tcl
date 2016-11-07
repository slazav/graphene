# this code is from ROTA's MonitorViewer

proc format_time {fmt p t} {
  # we cannot use just 'clock format' command since t can contain fractional
  # seconds
  set ti [expr {int($t/1000)}]
  set fsec [string range [string trimright [format %.6f [expr {$t/1000.0-$ti}]] 0] 1 end]
  if {[string match . $fsec]} {
    clock format $ti -format $fmt
  } else {
    string map [list "\001" $fsec] [clock format $ti -format [string map {%S "%S\001"} $fmt]]
  }
}

proc format_time_x {t} {
  format_time "%Y-%m-%d %H:%M:%S" . $t
}

proc format_time_z {t} {
  format_time "%Y-%m-%d %H:%M:%S %Z" . $t
}

###

proc time2str {x} {
  set ret [ clock format [expr {int($x/1000)}] -format %H:%M:%S -gmt yes]
  set ms [expr int($x)%1000]
  if { $ms > 0 } {set ret "$ret.$ms"}
  return $ret
}
