package require Itcl
package require ParseOptions 1.0
package require BLT

# set graphene of text data source,
# update BLT vectors if needed.
#
# Options:
#   -name    - file/db name
#   -conn    - graphene connection for a database source
#   -ncols   - number of data columns (time column is not included)
#   -cnames  - list of unique names for all data columns (time is not included)
#   -ctitles - list of titles for all data columns
#   -ccolors - list of colors for all data columns
#   -cfmts   - list of format settings for all data columns
#   -chides  - list of hide settings for all data columns
#   -logs    - list of logscale settings for all data columns
#
# File source:
#  comments: #, %, ;

set MIN_DATA_SIZE 4096

itcl::class DataSource {
  variable name
  variable conn
  variable ncols
  variable cnames
  variable ctitles
  variable ccolors
  variable cfmts
  variable chides
  variable clogs

  # datasource values
  variable tmin
  variable tmax
  variable fsize

  # current values
  variable curr_tmin
  variable curr_tmax
  variable curr_maxdt

  variable verbose

  ######################################################################

  constructor {plot args} {
    # parse options
    set opts {
      -name    name    {}
      -conn    conn    {}
      -ncols   ncols    1
      -cnames  cnames  {}
      -ctitles ctitles {}
      -ccolors ccolors {}
      -cfmts   cfmts   {}
      -chides  chides  {}
      -clogs   clogs   {}
      -verbose verbose 1
    }
    if {[catch {parse_options "graphene::data_source" \
      $args $opts} err]} { error $err }

    if {$verbose} {
      puts "Add data source \"$name\" with $ncols columns" }


    # create automatic column names
    for {set i [llength $cnames]} {$i < $ncols} {incr i} {
      lappend cnames "$name:$i" }

    # create automatic column titles
    for {set i [llength $ctitles]} {$i < $ncols} {incr i} {
      lappend ctitles "$name:$i" }

    # create automatic column colors
    set defcolors {red green blue cyan magenta yellow}
    for {set i [llength $ccolors]} {$i < $ncols} {incr i} {
      set c [lindex $defcolors [expr {$i%[llength $defcolors]} ] ]
      lappend ccolors $c }

    # create automatic format settings
    for {set i [llength $cfmts]} {$i < $ncols} {incr i} {
      lappend cfmts "%g" }

    # show all columns by default
    for {set i [llength $chides]} {$i < $ncols} {incr i} {
      lappend chides 0 }

    # non-log scale for all columns by default
    for {set i [llength $clogs]} {$i < $ncols} {incr i} {
      lappend clogs 0 }

    # find tmin/tmax
    update_tlimits
    if {$verbose} {
      puts " tmin: $tmin, tmax: $tmax" }
    # we will load data by parts, so it is important to set global
    # limits into x axis scrollmin/scrollmax
#    $plot axis configure x -scrollmin $tmin -scrollmax $tmax

    # create BLT vectors for data
    blt::vector create "$this:T"
    for {set i 0} {$i < $ncols} {incr i} {
      set n [lindex $cnames $i]
      blt::vector create "$this:$i"
    }

    ## configure plot
    for {set i 0} {$i < $ncols} {incr i} {
      set n [lindex $cnames $i]
      set t [lindex $ctitles $i]
      set c [lindex $ccolors $i]
      set f [lindex $cfmts $i]
      set h [lindex $chides $i]
      set l [lindex $clogs $i]
      # create vertical axis and the element, bind them
      $plot axis create $n -title $t -titlecolor black -logscale $l
      $plot element create $n -mapy $n -symbol "" -color $c
      $plot element bind $n <Enter> [list $plot yaxis use [list $n]]
      # hide element if needed
      if {$h} {xblt::hielems::toggle_hide $plot $n}
      # set data vectors for the element
      $plot element configure $n -xdata "$this:T" -ydata "$this:$i"
      #
    }

  }

  ######################################################################
  # functions for reading files:

  # split columns
  method split_line {l} {
    # skip comments
    if {[regexp {^\s*[%#;]} $l]} {return {}}
    return [regexp -all -inline {\S+} $l]
  }

  # get next data line for the file position in the beginning of a line
  method get_line {fp} {
    while {[gets $fp line] >= 0} {
      set sl [split_line $line]
      if {$sl ne {}} { return $sl }
    }
    return {}
  }

  # get prev/next data line for an arbitrary file position
  method get_prev_line {fp} {
    while {[tell $fp] > 0} {
      # go one line back
      while { [read $fp 1] ne "\n" } { seek $fp -2 current }
      set pos [tell $fp]
      set sl [split_line [gets $fp]]
      if {$sl ne {}} { return $sl }\
      else { seek $fp [expr {$pos-2}] start }
    }
    return {}
  }
  method get_next_line {fp} {
    # find beginning of a line
    if { [tell $fp]>0 } {
       seek $fp -1 current
       while { [read $fp 1] ne "\n" } {}
    }
    return [get_line $fp]
  }


  ######################################################################
  # find tmin/tmax and fsize (for text sources)
  method update_tlimits {} {
    set tmin 0
    set tmax 0
    set curr_tmin  0
    set curr_tmax  0
    set curr_maxdt 0
    if {$conn ne {}} { ## graphene db
       set tmin [lindex [graphene::cmd $conn "get_next $name"] 0 0]
       set tmax [lindex [graphene::cmd $conn "get_prev $name"] 0 0]
    } else { ## file
      set fp [open $name r ]
      set tmin [lindex [get_next_line $fp] 0]
      seek $fp 0 end
      set fsize [tell $fp]
      set tmax [lindex [get_prev_line $fp] 0]
      close $fp
    }
  }

  ######################################################################
  # build position map for text sources
  # should be run after update_tlimits
  method update_pmap {} {
    set pmap_size 4096

    if {$conn ne {}} { return }

    # open file, get its size
    set fp [open $name r ]
    seek $fp 0 end
    set fsize [tell $fp]

    # roughly estimate number of lines in the file
    # if it is smaller then pmap_size - no need for a position map
    if {[expr {$fsize/30}] < $pmap_size } {close $fp; return}

    # max time step
    set dt [expr {($tmax-$tmin)/$pmap_size}]

    # get data
    seek $fp 0 start
    while {} {
      set l [get_next_line $fp]
    }
    for {set i 0} {$i < $pmap_size} {incr $i} {
      seek $fp [expr {$i*$fsize/$pmap_size}] start
    }

    close $fp
  }

  ######################################################################
  # update data
  method update_data {x1 x2 N} {
    # x1,x2 = 0..1
    set t1 [expr {int($tmin + ($tmax-$tmin)*$x1)}]
    set t2 [expr {int($tmin + ($tmax-$tmin)*$x2)}]
    set dt [expr {int(($t2-$t1)/$N)}]

    if {$t1 >= $curr_tmin && $t2 <= $curr_tmax && $dt >= $curr_maxdt} {return}
    if {$verbose} {
      puts "update_data $t1 $t2 $dt $name" }

    # expand the range:
    set t1 [expr {$t1 - ($t2-$t1)}]
    set t2 [expr {$t2 + ($t2-$t1)}]
    if {$t1 < $tmin} {set t1 $tmin}
    if {$t2 < $tmax} {set t2 $tmax}
    set curr_tmin $t1
    set curr_tmax $t2
    set curr_maxdt $dt

    # reset data vectors
    "$this:T" append 0
    "$this:T" delete 0:end
    for {set i 0} {$i < $ncols} {incr i} {
      "$this:$i" append 0
      "$this:$i" delete 0:end
    }

    ## for a graphene db
    if {$conn ne {}} { ## graphene db

      foreach line [graphene::cmd $conn "get_range $name $t1 $t2 $dt"] {
        # append data to vectors
        "$this:T" append [lindex $line 0]
        for {set i 0} {$i < $ncols} {incr i} {
          "$this:$i" append [lindex $line [expr $i+1]]
        }
      }

    ## for a text file
    } else {

      # open and read the file line by line
      set fp [open $name r ]
      set to {}
      while { [gets $fp line] >= 0 } {

        # skip comments
        set line [ split_line $line ]
        if {$line == {}} {continue}

        # check the range
        set t [lindex $line 0]
        if {$t < $t1 || $t > $t2 } {continue}
        if {$to ne {} && $t-$to < $dt} {continue}
        set to $t

        # append data to vectors
        "$this:T" append $t
        for {set i 0} {$i < $ncols} {incr i} {
          "$this:$i" append [lindex $line [expr $i+1]]
        }
      }
      close $fp
    }
  }

  ######################################################################
  method get_ncols {} { return $ncols }
  method get_tmin  {} { return $tmin }
  method get_tmax  {} { return $tmax }
}

