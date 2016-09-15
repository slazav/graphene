# graphene::monitor module.
# Temperature measurement for 2nd sound experiment
# AG34401A measures resistance (manual range needed?)
# HP34401 measures applied voltage (to know current on the resistor)

package require GpibLib

itcl::class therm2s {
  inherit graphene::monitor_module

  variable devR
  variable devU

  constructor {} {
    set dbname test/therm
    set tmin   1
    set tmax   10
    set atol   0
    set name   "Thermometer (R and U)"
    set cnames {R U}
    set devR [gpib_device dev -board 0 -address 17 -trimright "\r\n"]
    set devU [gpib_device hp3478a -board 0 -address 22 -trimright "\r\n"]
  }

  method get {} {
    set R [$devR cmd_read meas:res?]
    set U [$devU cmd_read F1RAZ1N5T3]
    return "$R $U"
  }
}
