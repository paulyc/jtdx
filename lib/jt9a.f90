! This source code file was last time modified by Igor UA3DJY on September 2nd, 2017
! All changes are shown in the patch file coming together with the full JTDX source code.

subroutine jt9a()
  use, intrinsic :: iso_c_binding, only: c_f_pointer
  use prog_args
  use timer_module, only: timer
  use timer_impl, only: init_timer !, limtrace
  use jt65_mod6
  include 'jt9com.f90'

! These routines connect the shared memory region to the decoder.
  interface
     function address_jtdxjt9()
       use, intrinsic :: iso_c_binding, only: c_ptr
       type(c_ptr) :: address_jtdxjt9
     end function address_jtdxjt9
  end interface

  integer*1 attach_jtdxjt9
!  integer*1 lock_jt9,unlock_jt9
  integer size_jtdxjt9
! Multiple instances:
  character*80 mykey
  type(dec_data), pointer :: shared_data
  type(params_block) :: local_params
  logical fileExists

! Multiple instances:
  i0 = len(trim(shm_key))

  call init_timer (trim(data_dir)//'/timer.out')
!  open(23,file=trim(data_dir)//'/CALL3.TXT',status='unknown')

!  limtrace=-1                            !Disable all calls to timer()

! Multiple instances: set the shared memory key before attaching
  mykey=trim(repeat(shm_key,1))
  i0 = len(mykey)
  i0=setkey_jtdxjt9(trim(mykey))

  i1=attach_jtdxjt9()

10 inquire(file=trim(temp_dir)//'/.lock',exist=fileExists)
  if(fileExists) then
     call sleep_msec(100)
     go to 10
  endif

  inquire(file=trim(temp_dir)//'/.quit',exist=fileExists)
  if(fileExists) then
     i1=detach_jtdxjt9()
     go to 999
  endif
  if(i1.eq.999999) stop                  !Silence compiler warning

  nbytes=size_jtdxjt9()
  if(nbytes.le.0) then
     print*,'jt9a: Shared memory mem_jtdxjt9 does not exist.'
     print*,"Must start 'jtdxjt9 -s <thekey>' from within WSJT-X."
     go to 999
  endif
  call c_f_pointer(address_jtdxjt9(),shared_data)
  local_params=shared_data%params !save a copy because wsjtx carries on accessing
  call flush(6)

  if(local_params%ndiskdat) then
     dd(1:NPTS)=shared_data%id2(1:NPTS)
  else
     rms=sum(abs(shared_data%dd2(1:10)))+sum(abs(shared_data%dd2(300000:300010)))+ &
         sum(abs(shared_data%dd2(623991:624000)))
     if(rms.gt.0.001) then 
        dd(1:NPTS)=shared_data%dd2(1:NPTS)
!print *,'win7',rms
     else ! workaround for zero data values of dd2 array under WinXP
        dd(1:NPTS)=shared_data%id2(1:NPTS)
!print *, 'winxp',rms
     endif
  endif

!  call timer('decoder ',0)
  call multimode_decoder(local_params)
!  call timer('decoder ',1)

100 inquire(file=trim(temp_dir)//'/.lock',exist=fileExists)
  if(fileExists) go to 10
  call sleep_msec(100)
  go to 100

999 call timer('decoder ',101)

  return
end subroutine jt9a
