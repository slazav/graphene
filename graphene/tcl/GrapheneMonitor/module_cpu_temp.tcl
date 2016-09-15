# An example of graphene::monitor module.
# Monitoring CPU temperature (acpi -t)

itcl::class cpu_temp {
  inherit graphene::monitor_module
  constructor {} {
    set dbname cpu_temp
    set tmin   1
    set tmax   10
    set atol   0.1
    set name   "CPU temperature (acpi -t)"
    set cnames {"Tcpu, C"}
  }

  method get {} {
    regexp { ([0-9\.]*) deg} [exec acpi -t] a b
    return $b
  }
}
