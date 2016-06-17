#!/usr/bin/wish

# example of monitoring program
lappend auto_path ..
package require Graphene
package require GrapheneMonitor

source module_cpu_load.tcl
source module_cpu_temp.tcl

# local connection to the database, current folder
graphene::monitor mon [graphene::open -db_path "."]
mon configure -verb 1

mon add_module [cpu_temp #auto]
mon add_module [cpu_load #auto]

