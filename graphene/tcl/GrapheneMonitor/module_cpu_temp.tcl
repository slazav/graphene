# An example of graphene::monitor module.
# Monitoring CPU temperature (acpi -t)

itcl::class cpu_temp {
  inherit graphene::monitor_module
  constructor {} {
    set dbname cpu_temp
    set tmin   1000
    set tmax   10000
    set atol   0.1
    set name   "CPU temperature (acpi -t)"
  }

  method get {} {
    regexp { ([0-9\.]*) deg} [exec acpi -t] a b
    return $b
  }
}
