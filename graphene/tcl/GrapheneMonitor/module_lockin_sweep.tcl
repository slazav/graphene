# An example of graphene::monitor module.
# Sweep frequency and measure signal by SR830 lock in

itcl::class lockin_sweep {
  inherit graphene::monitor_module

  variable dev

  constructor {} {
    set dbname he4s/lockin_sweep
    set tmin   0.5
    set tmax   10
    set atol   0
    set name   "Lock-In sweeper"
    set cnames {F X Y}

    set dev [gpib_device sr830 -board 0 -address 6 -trimright "\r\n"]

    set sweeper 1
    set smin 500
    set smax 700
    set ds 0.5
#    set s0 [$dev cmd_read "FREQ?"]
    set s0 $smin
    $dev write "FREQ $s0"
    set sdir 1
    set srun 1
    if {$s0 > $smax} {set sdir -1}
    if {$s0 < $smin} {set sdir +1}
  }


  method get {} {
    set res [regexp -all -inline {[^,]+} [$dev cmd_read "SNAP? 9,1,2"]]
    $dev write "FREQ $s0"
    return $res
  }
}
