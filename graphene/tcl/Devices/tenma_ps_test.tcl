#!/usr/bin/tclsh

lappend auto_path ..
package require Devices 1.0

set f [Devices::TenmaPS #auto /dev/ttyACM0]

puts [$f cmd *IDN?]
$f cmd "BEEP0"
$f cmd OUT1
$f cmd VSET1:1
$f cmd ISET1:0.223
$f cmd OUT1

puts [$f cmd *IDN?]


puts [$f cmd ISET1?]
puts [$f cmd IOUT1?]

