# An example of graphene::monitor module.
# Monitoring 1, 5 and 10 min average CPU load
# (3 numbers from /proc/loadavg)

itcl::class cpu_load {
  inherit graphene::monitor_module
  constructor {} {
    set dbname cpu_load
    set tmin   1
    set tmax   10
    set atol   {0.01 0.01 0.01}
    set name   "CPU load (/proc/loadavg)"
    set cnames {"1 min CPU load " "5 min CPU load" "10 min CPU load"}
  }

  method get {} {
    set fs [ open /proc/loadavg ]
    set l [gets $fs]
    close $fs
    set ll [regexp -all -inline {\S+} $l]
    return [lrange $ll 0 2]
  }

}
