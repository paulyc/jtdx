! This source code file was last time modified by Igor UA3DJY on February 15th, 2015.
! All changes are shown in the patch file coming together with the full JTDX source code.

module jt65_mod9 
! used to store all callsigns seen in JT modes in the memory
! to check standard messages for false FTRSD decodes

  use prog_args
  
  parameter (MAXCALLS=40000)
  character*6 call1(MAXCALLS)
  character callsign*12
  character*180 line
  integer ncalls
  logical(1) first
  data first/.true./
  save first,call1,ncalls

end module jt65_mod9