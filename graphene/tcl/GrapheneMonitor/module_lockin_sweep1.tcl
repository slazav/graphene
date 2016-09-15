# An example of graphene::monitor module.
# Sweep voltage and measure signal by SR830 lock-in


itcl::class lockin_sweep {
  inherit graphene::monitor_module

  variable dev

  constructor {} {
    set dbname test/lockin_sweep
    set tmin   5
    set tmax   10
    set atol   0
    set name   "Lock-In sweeper"
    set cnames {F X Y}

    set dev [gpib_device sr830 -board 0 -address 6 -trimright "\r\n"]

    set sweeper 1
    set smin 0.1
    set smax 5.0
    set ds 0.1
    set s0 0.1
    $dev write "SLVL $s0"
    after 2000
    set sdir 1
    set srun 1
    if {$s0 > $smax} {set sdir -1}
    if {$s0 < $smin} {set sdir +1}
  }


  method get {} {
    set U1 [regexp -all -inline {[^,]+} [$dev cmd_read "OUTP? 3"]]
    set U0 [$dev cmd_read "SLVL?"]
    set R0 1006000.0
    set R [expr {$R0*$U1/($U0-$U1)}]
    set W [expr {$U1*$U1/2.0/$R}]

    $dev write "SLVL $s0"
    return "$U0 $R $W"
  }
}
