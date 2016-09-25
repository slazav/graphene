#!/usr/bin/wish

source autoupdater.tcl

# defauls updater: write a message every second
set updater1 [autoupdater #auto ]

# all options: write a message, control updater state and interval
proc u2 {} {puts "update!"}

set state 0
set interval 500

set updater2 [autoupdater #auto \
  -state_var ::state\
  -int_var   ::interval\
  -update_proc u2\
]


label .label1 -text "update interval:"
set interval_e $interval
entry .int -textvariable interval_e
bind  .int <Key-Return> {set interval $interval_e}
# bind  .int <Key-Return> {$updater2 set_interval $interval_e}; ## same
grid .label1 .int

label .label2 -text "updater state:"
checkbutton .btn -variable state
grid .label2 .btn



