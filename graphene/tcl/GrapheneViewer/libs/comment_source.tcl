package require Itcl
package require ParseOptions 1.0
package require BLT

# set graphene of text comment source
#
# Options:
#   -name    - file/db name
#   -conn    - graphene connection for a database source
#   -color   - color
#
# File source:
#  comments: #, %, ;

itcl::class CommentSource {
  variable name
  variable conn
  variable color

  variable verbose

  ######################################################################

  constructor {plot args} {
    # parse options
    set opts {
      -name    name    {}
      -conn    conn    {}
      -color   color   black
      -verbose verbose 1
    }
    if {[catch {parse_options "graphene::data_source" \
      $args $opts} err]} { error $err }

    if {$verbose} {
      puts "Add comment source \"$name\"" }

    bind $plot <Control-ButtonPress-1> "$this comment_operation %x %y %X %Y"
    bind scomment <Motion> "$this comment_create_drag %x %y"
    bind scomment <ButtonRelease-1> "$this comment_create_finish %x %y %X %Y"
    bind scomment <B1-ButtonPress-3> "$this comment_create_cancel %x %y"

    bind scomment <ButtonPress-2>   "$this com_move_start %x %y"
    bind scomment <B2-Motion>       "$this com_move_do %x %y"
    bind scomment <ButtonRelease-2> "$this com_move_end %x %y"

    bind scomment <B2-ButtonPress-3> "$this com_move_cancel %x %y"
    bind scomment <ButtonPress-3>    "$this com_edit %X %Y"
    xblt::bindtag::add $plot scomment
    $plot axis create zx -hide 1 -min 0 -max 1
  }

  method comment_operation {args} {  puts "comment_operation $args" }
  method comment_create_drag {args} {  puts "comment_create_drag $args" }
  method comment_create_finish {args} {  puts "comment_create_finish $args" }
  method comment_create_cancel {args} {  puts "comment_create_cancel $args" }
  method com_move_start {args} {  puts "com_move_start $args" }
  method com_move_do {args} {  puts "com_move_do $args" }
  method com_move_end {args} {  puts "com_move_end $args" }
  method com_move_cancel {args} {  puts "com_move_cancel $args" }
  method com_edit {args} {  puts "com_edit $args" }


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

  ######################################################################
  # update data
  method update_data {t1 t2 dt} {

    if {$t1 > $curr_tmin && $t2 < $curr_tmax && $dt > $curr_maxdt} {return}
    if {$verbose} {
      puts "update_data $t1 $t2 $dt $name" }


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
  method get_tmin  {} { return $tmin }
  method get_tmax  {} { return $tmax }
}

