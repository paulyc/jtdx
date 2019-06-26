! This source code file was last time modified by Igor UA3DJY on August 21st, 2017
! All changes are shown in the patch file coming together with the full JTDX source code.

  use, intrinsic :: iso_c_binding, only: c_int, c_short, c_float, c_char, c_bool

  include 'constants.f90'

  !
  ! these structures must be kept in sync with ../commons.h
  !
  type, bind(C) :: params_block
     character(kind=c_char, len=20) :: datetime
     character(kind=c_char, len=12) :: mycall
     character(kind=c_char, len=12) :: hiscall
     character(kind=c_char, len=6) :: mygrid
     character(kind=c_char, len=6) :: hisgrid
     real(c_float) :: emedelay
     real(c_float) :: dttol
     integer(c_int) :: listutc(10)
     integer(c_int) :: nutc
     integer(c_int) :: ntr
     integer(c_int) :: nfqso
     integer(c_int) :: npts8
     integer(c_int) :: nfa
     integer(c_int) :: nfsplit
     integer(c_int) :: nfb
     integer(c_int) :: ntol
     integer(c_int) :: kin
     integer(c_int) :: nzhsym
     integer(c_int) :: ndepth
     integer(c_int) :: ntxmode
     integer(c_int) :: nmode
     integer(c_int) :: minw
     integer(c_int) :: nclearave
     integer(c_int) :: minsync
     integer(c_int) :: nlist
     integer(c_int) :: nranera
     integer(c_int) :: ntrials10
     integer(c_int) :: ntrialsrxf10
     integer(c_int) :: naggressive
     integer(c_int) :: nharmonicsdepth
     integer(c_int) :: ntopfreq65
     integer(c_int) :: nprepass
     integer(c_int) :: nsdecatt
     integer(c_int) :: nlasttx
     integer(c_int) :: ndelay
     logical(c_bool) :: ndiskdat
     logical(c_bool) :: newdat
     logical(c_bool) :: nagain
     logical(c_bool) :: nagainfil
     logical(c_bool) :: nswl
     logical(c_bool) :: nfilter
     logical(c_bool) :: nstophint
     logical(c_bool) :: nagcc
     logical(c_bool) :: nhint
     logical(c_bool) :: fmaskact
     logical(c_bool) :: showharmonics
 end type params_block

  type, bind(C) :: dec_data
     real(c_float) :: ss(184,NSMAX)
     real(c_float) :: savg(NSMAX)
     integer(c_short) :: id2(NMAX)
     real(c_float) :: dd2(NMAX)
     type(params_block) :: params
  end type dec_data
