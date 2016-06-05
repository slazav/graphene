#!/bin/sh -x
# interactive mode example

PATH=..:$PATH

stsdb -d . interactive <<EOF
create pressure INT16 "Some_text"
delete pressure

create pressure DOUBLE "Some_text"
rename pressure press
set_descr press "Measured_pressure"

info press
list

put press  1234567890000 0.1
put press  1234567900000 0.2
get press  1234567895000

put press now 0.1
put press now 0.2
put press now 0.3
get_prev press
get_next press
get_range press

delete press
EOF
