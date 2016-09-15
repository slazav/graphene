# Monitoring pt1000 thermometer via HP34401 multimeter, 4-wire
package require GpibLib

itcl::class pt1000 {
  inherit graphene::monitor_module
  variable dev

  constructor {} {
    set dbname pt1000
    set tmin   1
    set tmax   60
    set atol   0.01
    set name   "pt1000"
    set dev [gpib_device dev -board 0 -address 17 -trimright "\r\n"]
  }

  method get {} {
    return [$dev cmd_read meas:fres?]
  }
}
