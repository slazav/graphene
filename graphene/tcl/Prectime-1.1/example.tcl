#!/usr/bin/tclsh

# note: library should be install into system
package require Prectime

set t1 [prectimetoday]
set t2 [prectime]

puts "prectimetoday: $t1"
puts "prectime:      $t2"
