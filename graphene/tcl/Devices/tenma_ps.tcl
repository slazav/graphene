package provide Devices 1.0
package require Itcl

# Korad/Velleman/Tenma power supply
# See https://sigrok.org/wiki/Korad_KAxxxxP_series

namespace eval Devices {

itcl::class TenmaPS {
  variable dev

  # open/close the device
  constructor {fname} { set dev [::open $fname {RDWR NONBLOCK}] }
  destructor { ::close $dev}

  # run command, read response if needed
  method cmd {c} {
    puts -nonewline $dev $c
    flush $dev
    after 50
    if {[string index $c end] == "?"} {
      set res [read $dev 1024]
      after 50
      return $res
    }
    return {}
  }

}
}

# commands:
# *IDN?    -- Request identification from device.
# STATUS?  -- Request the actual status. The only reliable bits are:
#              0x40 (Output mode: 1:on, 0:off),
#              0x20 (OVP and/or OCP mode: 1:on, 0:off)
#              0x01 (CV/CC mode: 1:CV, 0:CC)
# VSET1?   -- Request the voltage as set by the user.
# VOUT1?   -- Request the actual voltage output.
# ISET1?   -- Request the current as set by the user. (BUG?)
# IOUT1?   -- Request the actual output current.

# VSET1:<val> -- Set the maximum output voltage.
# ISET1:<val> -- Set the maximum output current.
# OUT1        -- Enable the power output.
# OUT0        -- Disable the power output.
# OVP1        -- Enable the "Over Voltage Protection"
# OVP0        -- Disable the "Over Voltage Protection".
# OCP1        -- Enable the "Over Current Protection"
# OCP0        -- Disable the "Over Current Protection".
# TRACK<N>    -- Set multichannel mode,
#                 0 independent, 1 series, 2 parallel
#                (from Velleman protocol v1.3 documentation). 
# RCL<N>      -- Recalls voltage and current limits from memory, 1 to 5
# SAV<N>      -- Saves voltage and current limits to memory, 1 to 5

# BEEP1
# BEEP0
