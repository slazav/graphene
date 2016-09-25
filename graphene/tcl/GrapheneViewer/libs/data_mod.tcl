package require Itcl
package require ParseOptions 1.0
package require BLT

# shift and rescale plots

# 2nd button on plot: shift and rescale

itcl::class DataMod {
  variable blt_plot
  variable elemop

  ######################################################################
  constructor {plot args} {
    set blt_plot $plot
    bind elemop <ButtonPress-2> "$this elemopstart %x %y"
    bind elemop <B2-Motion> "$this elemopdo %x %y"
    bind elemop <ButtonRelease-2> "$this elemopend %x %y"
    xblt::bindtag::add $blt_plot elemop
    set elemop(started) 0
  }

  ######################################################################
  method elemopstart {xp yp} {
    $blt_plot element closest $xp $yp e -interpolate yes
    set elemop(es) $e(name)
    if {$elemop(es) == ""} return
    xblt::crosshairs::show $blt_plot 0
    set elemop(xpi) $xp
    set elemop(yi) [$blt_plot axis invtransform $elemop(es) $yp]
    set elemop(sc) 1.0
    set elemop(started) 1
    return -code break
  }

  method elemopdo {xp yp} {
    if {! $elemop(started)} return
    set v $elemop(es)
    set yi $elemop(yi)
    set y [$blt_plot axis invtransform $v $yp]
    set y1 [lindex [$blt_plot axis limits $v] 0]
    set y2 [lindex [$blt_plot axis limits $v] 1]
    set sc [expr {exp(($xp - $elemop(xpi)) / 100.0)}]

    if {[$blt_plot axis cget $v -logscale]} {
      set newmin [expr {exp(log($yi) - (log($y) - log($y1))*$elemop(sc)/$sc)}]
      set newmax [expr {exp(log($yi) + (log($y2) - log($y))*$elemop(sc)/$sc)}]
    } else {
      set newmin [expr {$yi - ($y - $y1)*$elemop(sc)/$sc}]
      set newmax [expr {$yi + ($y2 - $y)*$elemop(sc)/$sc}]
    }
    $blt_plot axis configure $v -min $newmin -max $newmax
    set elemop(sc) $sc
    return -code break
  }

  method elemopend {xp yp} {
    if {! $elemop(started)} return
    set elemop(started) 0
    xblt::crosshairs::show $blt_plot 1
    return -code break
  }

}

