# An example of graphene::monitor module.
# Monitoring 1, 5 and 10 min average CPU load
# (3 numbers from /proc/loadavg)

itcl::class cpu_load {
  inherit graphene::monitor_module
  constructor {} {
    set dbname cpu_load
    set tmin   1000
    set tmax   10000
    set atol   {0.01 0.01 0.01}
    set name   "CPU load (/proc/loadavg)"
  }

  method get {} {
    set fs [ open /proc/loadavg ]
    set l [gets $fs]
    close $fs
    set ll [regexp -all -inline {\S+} $l]
    return [lrange $ll 0 2]
  }

}
