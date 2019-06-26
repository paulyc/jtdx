! This source code file was last time modified by Igor UA3DJY on August 21st, 2017
! All changes are shown in the patch file coming together with the full JTDX source code.

subroutine multimode_decoder(params)

  !$ use omp_lib
  use prog_args
!  use timer_module, only: timer
  use jt65_decode
  use jt9_decode
  use jt9s_decode
  use jt10_decode
  use jt65_mod6
  use jt9_mod1

  include 'jt9com.f90'
  include 'timer_common.inc'

  type, extends(jt65_decoder) :: counting_jt65_decoder
     integer :: decoded
  end type counting_jt65_decoder

  type, extends(jt9_decoder) :: counting_jt9_decoder
     integer :: decoded
  end type counting_jt9_decoder

  type, extends(jt9s_decoder) :: counting_jt9s_decoder
     integer :: decoded
  end type counting_jt9s_decoder
  
  type, extends(jt10_decoder) :: counting_jt10_decoder
     integer :: decoded
  end type counting_jt10_decoder

  logical baddata
  integer nutc,ndelay
  type(params_block) :: params
  data ndelay/0/
  save
  
  real rnd
  logical newdat65,newdat9,nagainjt9,nagainjt9s,nagainjt10

  type(counting_jt65_decoder) :: my_jt65
  type(counting_jt9_decoder) :: my_jt9
  type(counting_jt9s_decoder) :: my_jt9s
  type(counting_jt10_decoder) :: my_jt10
  
  ! initialize decode counts
  my_jt65%decoded = 0
  my_jt9%decoded = 0
  my_jt9s%decoded = 0
  my_jt10%decoded = 0

  nagainjt9=.false.;  nagainjt9s=.false.;  nagainjt10=.false.
  if(.not.params%nagain) ndelay=params%ndelay

  if(ndelay.gt.0) then ! receive incomplete interval
     print *,'partial loss of data'
     numsamp=ndelay*12000
     dd(numsamp+1:624000)=dd(1:(624000-numsamp))
     do i=1,numsamp
     call random_number(rnd)
        dd(i)=10.0*rnd-5.
     enddo
  endif

  ntrials=params%nranera

  if (params%nagain .or. (params%nagainfil .and. (params%nmode.eq.65 .or. &
      params%nmode.eq.(65+9)))) then
     open(13,file=trim(temp_dir)//'/decoded.txt',status='unknown',position='append')
  else
     open(13,file=trim(temp_dir)//'/decoded.txt',status='unknown')
  endif

  rms1=sqrt(dot_product(dd(100000:100099),dd(100000:100099))/100.0)
  rms2=sqrt(dot_product(dd(200000:200099),dd(200000:200099))/100.0)
  rms3=sqrt(dot_product(dd(300000:300099),dd(300000:300099))/100.0)
  rms4=sqrt(dot_product(dd(400000:400099),dd(400000:400099))/100.0)
  rms5=sqrt(dot_product(dd(500000:500099),dd(500000:500099))/100.0)
  rms=(rms1+rms2+rms3+rms4+rms5)/5.0
  if(rms.lt.2.0) then
     nsynced=0
     ndecoded=0
     print *,'input signal low rms'
     go to 800
  endif

  if(baddata()) then
     print *,'audio gap detected'
     j=0
     do i=1,490
        sq=0.
        do n=1,1200,60
           j=j+60
           x=dd(j)
           sq=sq + abs(x)
        enddo
        if(sq.lt.1.0) then
           dd(j)=1.0; dd(j-600)=1.0
        endif
     enddo
  endif

!n1000=0  ! attempt to make T10 noise blanker, some degradation in decoding
!do i=2,623998
!!if(abs(dd(i)-dd(i-1)).gt.1000) then; n1000=n1000+1; print *,i; endif
!if((dd(i+1)-dd(i)).gt.1000.0) dd(i+1)=dd(i)+(dd(i)+dd(i-1))/2.0
!if((dd(i)-dd(i+1)).gt.1000.0) dd(i+1)=dd(i)-(dd(i)+dd(i-1))/2.0
!enddo

! signal input level diagnostics
!rrr=0.
!ddd=0.
!do i=1,624000
!if (abs(dd(i)).gt.rrr) rrr=abs(dd(i))
!enddo
!ddd=20*log10(rrr)
!print *,''
!print *,'max possible sample level is 32767'
!print *,'max RX sample level',int(rrr)
!print *,''
!print *,'max possible dynamic range is 90dB'
!print *,'RX signal dynamic range',nint(ddd)
!print *,''
! end of signal input level diagnostics

  newdat65=params%newdat
  newdat9=params%newdat
  nshift=26000
  if(.not.params%nagain) nutc=params%nutc
  if(params%nagain .and. .not.params%nagainfil) newdat9=.true.

  if(.not.params%nagcc) dd9(0:NPTS9-1)=dd(1:NPTS9) ! NPTS9=618624 for decoding on 52nd second, NPTS9=597888
  if(params%nagcc .and. (params%nmode.eq.10 .or. params%nmode.eq.9)) then
     call agcc(params%nmode,params%ntxmode)
  endif

  if(params%nmode.ne.10 .and. params%nmode.ne.9) then
!    dd(1+nshift:npts)=dd(1:npts-nshift) ! NPTS=624000
     dd(26001:624000)=dd(1:598000)
!    dd(1:nshift)=0.0
     dd(1:26000)=0.0

     if(params%nagcc) then
        call agcc(params%nmode,params%ntxmode)
        if(params%nmode.eq.65+9) then
!          id2(0:580464-1)=nint(dd(1+nshift:580464+nshift))
           dd9(0:597999)=dd(26001:624000)
           if(params%nzhsym.eq.173) dd9(597888:597999)=0.
           dd9(598000:NPTS9-1)=0.
        endif
     endif
  endif

  if(.not.params%nagainfil .and. params%ntxmode.eq.9 .and. params%nagain) nagainjt9=.true.
  if(.not.params%nagainfil .and. params%nmode.eq.9 .and. params%nagain) nagainjt9s=.true.
  if(.not.params%nagainfil .and. params%nagain) nagainjt10=.true.
  if(params%nagainfil) nagainjt10=.false.

  if(params%nmode.eq.10) then
     call my_jt10%decode(jt10_decoded,nutc,params%nfqso,newdat9,params%npts8,   &
          params%nfa,params%nfb,params%ntol,params%nzhsym,nagainjt10,params%nagainfil,params%ntrials10, &
          params%ntrialsrxf10,params%nfilter,params%nswl,params%nagcc,params%nhint,params%nstophint, &
          params%nlasttx,params%mycall,params%hiscall,params%hisgrid)
     go to 800
  endif

  if(params%nmode.eq.9) then
     call my_jt9s%decode(jt9s_decoded,nutc,params%nfqso,newdat9,params%npts8,   &
          params%nfa,params%nfb,params%nzhsym,params%nfilter,params%nswl,   &
          nagainjt9s,params%nagainfil,params%ndepth,params%nhint,params%nstophint, &
          params%nlasttx,params%mycall,params%hiscall,params%hisgrid)
     go to 800
  endif

  !$call omp_set_dynamic(.true.)
  !$omp parallel sections num_threads(2) copyin(/timer_private/) shared(ndecoded) if(.true.) !iif() needed on Mac

  !$omp section
  if(params%nmode.eq.65 .or. (params%nmode.eq.(65+9) .and. params%ntxmode.eq.65)) then
     ! We're in JT65 mode, or should do JT65 first

     nf1=params%nfa
     nf2=params%nfb

!     call timer('jt65a   ',0)
     call my_jt65%decode(jt65_decoded,nutc,nf1,nf2,params%nfqso,  &
          logical(params%nagainfil),ntrials,params%naggressive,params%nhint,params%mycall, &
          params%hiscall,params%hisgrid,params%nprepass,params%nswl,params%nfilter,params%nstophint, &
          params%nlasttx,params%nsdecatt,params%fmaskact,params%ntxmode,params%ntopfreq65, &
          params%nharmonicsdepth,params%showharmonics)
!     call timer('jt65a   ',1)
  else if(params%nmode.eq.(65+9) .and. params%ntxmode.eq.9) then
     ! We're in JT9 mode, or should do JT9 first
!     call timer('decjt9  ',0)
     call my_jt9%decode(jt9_decoded,nutc,params%nfqso,newdat9,params%npts8,   &
          params%nfa,params%nfsplit,params%nfb,params%ntol,params%nzhsym,                   &
          nagainjt9,params%nagainfil,params%ndepth,params%nmode,params%nhint,params%nstophint, &
          params%nlasttx,params%mycall,params%hiscall,params%hisgrid,params%ntxmode)
!     call timer('decjt9  ',1)
  endif

  !$omp section
  if(params%nmode.eq.(65+9)) then          !Do the other mode (we're in dual mode)
     if (params%ntxmode.eq.9) then
        if(.not.nagainjt9) then
           nf1=params%nfa
           nf2=params%nfb
!           call timer('jt65a   ',0)
           call my_jt65%decode(jt65_decoded,nutc,nf1,nf2,params%nfqso, &
           logical(params%nagainfil),ntrials,params%naggressive,params%nhint,params%mycall,     &
           params%hiscall,params%hisgrid,params%nprepass,params%nswl,params%nfilter,params%nstophint, &
           params%nlasttx,params%nsdecatt,params%fmaskact,params%ntxmode,params%ntopfreq65, &
           params%nharmonicsdepth,params%showharmonics)
!           call timer('jt65a   ',1)
        endif
     else
        if (params%ntxmode.eq.65 .and. params%nagain) go to 2
!        call timer('decjt9  ',0)
        call my_jt9%decode(jt9_decoded,nutc,params%nfqso,newdat9,params%npts8,&
             params%nfa,params%nfsplit,params%nfb,params%ntol,params%nzhsym,                &
             nagainjt9,params%nagainfil,params%ndepth,params%nmode,params%nhint,params%nstophint, &
             params%nlasttx,params%mycall,params%hiscall,params%hisgrid,params%ntxmode)
!        call timer('decjt9  ',1)
2       continue
     end if
  endif

  !$omp end parallel sections

  ! JT65 is not yet producing info for nsynced, ndecoded.
  ndecoded = my_jt65%decoded + my_jt9%decoded + my_jt9s%decoded
800 write(*,1010) nsynced,ndecoded
1010 format('<DecodeFinished>',2i4)
  call flush(6)
  close(13) 

  return

contains

  subroutine jt65_decoded (this, utc, sync, snr, dt, freq, drift, decoded, ft, servis)
    use jt65_decode
    implicit none

    class(jt65_decoder), intent(inout) :: this
    integer, intent(in) :: utc
    real, intent(in) :: sync
    integer, intent(in) :: snr
    real, intent(in) :: dt
    integer, intent(in) :: freq
    integer, intent(in) :: drift
    character(len=22), intent(in) :: decoded
    integer, intent(in) :: ft
    character(len=1), intent(in) :: servis

    real dtshift
    dtshift=(real(nshift))/12000.0
    !$omp critical(decode_results)
    write(*,1010) utc,snr,dt-dtshift,freq,decoded,servis

1010 format(i4.4,i4,f5.1,i5,1x,'#',1x,a22,a1)
    write(13,1012) utc,nint(sync),snr,dt-dtshift,float(freq),drift,decoded,ft

1012 format(i4.4,i4,i5,f6.1,f8.0,i4,3x,a22,' JT65',i4)
    call flush(6)

    !$omp end critical(decode_results)
    select type(this)
    type is (counting_jt65_decoder)
       this%decoded = this%decoded + 1
    end select
 end subroutine jt65_decoded

  subroutine jt9_decoded (this, utc, sync, snr, dt, freq, drift, decoded, servis9)
    use jt9_decode
    implicit none

    class(jt9_decoder), intent(inout) :: this
    integer, intent(in) :: utc
    real, intent(in) :: sync
    integer, intent(in) :: snr
    real, intent(in) :: dt
    real, intent(in) :: freq
    integer, intent(in) :: drift
    character(len=22), intent(in) :: decoded
    character(len=1), intent(in) :: servis9

    !$omp critical(decode_results)
    write(*,1000) utc,snr,dt,nint(freq),decoded,servis9
1000 format(i4.4,i4,f5.1,i5,1x,'@',1x,a22,a1)
    write(13,1002) utc,nint(sync),snr,dt,freq,drift,decoded
1002 format(i4.4,i4,i5,f6.1,f8.0,i4,3x,a22,' JT9')
    call flush(6)
    !$omp end critical(decode_results)
    select type(this)
    type is (counting_jt9_decoder)
       this%decoded = this%decoded + 1
    end select
  end subroutine jt9_decoded

  subroutine jt9s_decoded (this, utc, sync, snr, dt, freq, drift, decoded, servis9)
    use jt9s_decode
    implicit none

    class(jt9s_decoder), intent(inout) :: this
    integer, intent(in) :: utc
    real, intent(in) :: sync
    integer, intent(in) :: snr
    real, intent(in) :: dt
    real, intent(in) :: freq
    integer, intent(in) :: drift
    character(len=22), intent(in) :: decoded
    character(len=1), intent(in) :: servis9

    !$omp critical(decode_results)
    write(*,1000) utc,snr,dt,nint(freq),decoded,servis9
1000 format(i4.4,i4,f5.1,i5,1x,'@',1x,a22,a1)
    write(13,1002) utc,nint(sync),snr,dt,freq,drift,decoded
1002 format(i4.4,i4,i5,f6.1,f8.0,i4,3x,a22,' JT9')
    call flush(6)
    !$omp end critical(decode_results)
    select type(this)
    type is (counting_jt9s_decoder)
       this%decoded = this%decoded + 1
    end select
  end subroutine jt9s_decoded

  subroutine jt10_decoded (this, utc, sync, snr, dt, freq, drift, decoded, servis9)
    use jt10_decode
    implicit none

    class(jt10_decoder), intent(inout) :: this
    integer, intent(in) :: utc
    real, intent(in) :: sync
    integer, intent(in) :: snr
    real, intent(in) :: dt
    real, intent(in) :: freq
    integer, intent(in) :: drift
    character(len=22), intent(in) :: decoded
    character(len=1), intent(in) :: servis9

    write(*,1000) utc,snr,dt,nint(freq),decoded,servis9
1000 format(i4.4,i4,f5.1,i5,1x,'~',1x,a22,a1)
    write(13,1002) utc,nint(sync),snr,dt,freq,drift,decoded
1002 format(i4.4,i4,i5,f6.1,f8.0,i4,3x,a22,' T10')
    call flush(6)
    select type(this)
    type is (counting_jt10_decoder)
       this%decoded = this%decoded + 1
    end select
  end subroutine jt10_decoded

end subroutine multimode_decoder
