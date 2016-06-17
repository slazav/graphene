#!/usr/bin/tclsh

lappend auto_path ..
package require Graphene 1.0

#set gr [graphene::open -db_path "." -ssh_addr 127.0.0.1]
set gr [graphene::open -db_path "."]

set db test
graphene::cmd $gr "create $db"
graphene::cmd $gr "put $db now 123"
after 1
graphene::cmd $gr "put $db now 345"

puts [graphene::cmd $gr "get_range $db"]
graphene::cmd $gr "delete $db"
graphene::close $gr

#puts $l