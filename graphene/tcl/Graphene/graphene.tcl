package provide Graphene 1.0
package require ParseOptions 1.0

## Graphene database interface
##
## Usage:
##   set fd [graphene::open -addr ADDR -dbpath PATH]
##   graphene::cmd {fd cmdline}
##   graphene::close {fd}
##
## If ADDR is empty the local connection is opened
## otherwise ssh connection is used. Use ssh-config
## to set up handy name instead of address, use keys
## and ssh-add authentication agent to avoid typing
## passwords and passphrases.
##
## get first/last timestamp:
##   set tmin [lindex [graphene::cmd $conn "get_next $name"] 0 0]
##   set tmax [lindex [graphene::cmd $conn "get_prev $name"] 0 0]
## get many timestamps:
##   foreach line [graphene::cmd $conn "get_range $name $t1 $t2 $dt"] {
##     lappend t [lindex $line 0] }

namespace eval graphene {

proc open {args} {

  set opts {
    -ssh_addr ssh_addr {}
    -db_path  db_path  {}
  }
  if {[catch {parse_options "graphene::open" \
      $args $opts} err]} { error $err }

  set cmd [list]
  if { $ssh_addr ne "" } { lappend cmd ssh $ssh_addr }
  lappend cmd "graphene"
  if { $db_path  ne ""}  { lappend cmd -d $db_path }
  lappend cmd "interactive"
  set ret [::open "| $cmd" RDWR]
  fconfigure $ret -buffering line
  return $ret
}

proc cmd {fo str} {
  puts $fo $str

  set ret {}
  while {1} {
    set l [gets $fo]
    if { [regexp {^Error: (.*)} $l e ] } { error $e }
    if { [regexp {^OK$} $l] } { return $ret }
    lappend ret $l
  }
}

proc close {fo} { ::close $fo }

}
