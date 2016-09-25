#!/usr/bin/tclsh

lappend auto_path .

package require GpibLib 1.2

set dev [gpib_device xx -board 0 -address 9 -trimright \n -readymask 16]
puts $dev

puts "buffer length: [xx cget -bufferlen]"
puts "trim left: |[xx cget -trimleft]|"

$dev remote_enable
$dev write *STB?
puts "ready: [xx ready]"
#xx sleep 0.1
xx waitready
puts "poll: [xx poll] [xx poll 16] [xx poll 12]"
puts "ready: [xx ready]"
puts [$dev read]
puts "ready: [xx ready]"

$dev configure -trimleft + -trimright ""
puts [$dev cmd_read *STB?]
puts "trim left: |[xx cget -trimleft]|"
puts "timeout: [xx cget -timeout]"

$dev configure -trimright \n\r -waitready {1 1}
puts [$dev cmd_wait_read system:date?]

after idle {puts "I am async"}
#after 500 [subst {gpib_device delete $dev; puts "I am bad async"}] 

xx configure -timeout 0.1
puts "start wait"
$dev sleep 1
puts "end wait"
puts "timeout: [xx cget -timeout]"

#gpib_device yy -address 10
#yy write qqq
#yy poll

puts "go to local"
xx go_to_local
puts "check it!"
after 3000
puts "deleting device"
gpib_device delete $dev
