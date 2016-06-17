#!/usr/bin/tclsh

lappend auto_path ..
package require ParseOptions

proc myprog {args} {
  set parsetable \
    [list {-f -format} fmt  {} \
          {-d -descr}  desc {} \
          -color       color red]
  if {[catch {parse_options "myprog" \
      $args $parsetable} err]} { error $err }

  puts "fmt:   $fmt"
  puts "desc:  $desc"
  puts "color: $color"
}

myprog -f 1 -d descr -color blue

