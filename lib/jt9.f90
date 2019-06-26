! This source code file was last time modified by Igor UA3DJY on January 31st, 2017
! All changes are shown in the patch file coming together with the full JTDX source code.

program jt9

! Decoder for JT9.  Can run stand-alone, reading data from *.wav files;
! or as the back end of wsjt-x, with data placed in a shared memory region.

  use options
  use prog_args
  use, intrinsic :: iso_c_binding
  use FFTW3
  use timer_module, only: timer
  use timer_impl, only: init_timer, fini_timer
  use readwav

  include 'jt9com.f90'

  integer(C_INT) iret
  type(wav_header) wav
  real*4 s(NSMAX)
  character c
  character(len=500) optarg, infile
  character wisfile*80
  integer :: arglen,stat,offset,remain,mode=0,flow=200,fsplit=2700,          &
       fhigh=4000,nrxfreq=1500,ntrperiod=1,ndepth=60001
  logical :: read_files = .true., tx9 = .false., display_help = .false.
  type (option) :: long_options(23) = [ &
    option ('help', .false., 'h', 'Display this help message', ''),          &
    option ('shmem',.true.,'s','Use shared memory for sample data','KEY'),   &
    option ('tr-period', .true., 'p', 'Tx/Rx period, default MINUTES=1',     &
        'MINUTES'),                                                          &
    option ('executable-path', .true., 'e',                                  &
        'Location of subordinate executables (KVASD) default PATH="."',      &
        'PATH'),                                                             &
    option ('data-path', .true., 'a',                                        &
        'Location of writeable data files, default PATH="."', 'PATH'),       &
    option ('temp-path', .true., 't',                                        &
        'Temporary files path, default PATH="."', 'PATH'),                   &
    option ('share-path', .true., 'r',                                        &
        'Share files path, default PATH="."', 'PATH'),                   &
    option ('lowest', .true., 'L',                                           &
        'Lowest frequency decoded (JT65), default HERTZ=200', 'HERTZ'),      &
    option ('highest', .true., 'H',                                          &
        'Highest frequency decoded, default HERTZ=4007', 'HERTZ'),           &
    option ('split', .true., 'S',                                            &
        'Lowest JT9 frequency decoded, default HERTZ=2700', 'HERTZ'),        &
    option ('rx-frequency', .true., 'f',                                     &
        'Receive frequency offset, default HERTZ=1500', 'HERTZ'),            &
    option ('patience', .true., 'w',                                         &
        'FFTW3 planing patience (0-4), default PATIENCE=1', 'PATIENCE'),     &
    option ('fft-threads', .true., 'm',                                      &
        'Number of threads to process large FFTs, default THREADS=1',        &
        'THREADS'),                                                          &
    option ('jt65', .false., '6', 'JT65 mode', ''),                          &
    option ('jt9', .false., '9', 'JT9 mode', ''),                            &
    option ('jt4', .false., '4', 'JT4 mode', ''),                            &
    option ('sub-mode', .true., 'b', 'Sub mode, default SUBMODE=A', 'A'),    &
    option ('depth', .true., 'd',                                            &
        'JT9 decoding depth (1-3), default DEPTH=1', 'DEPTH'),               &
    option ('tx-jt9', .false., 'T', 'Tx mode is JT9', ''),                   &
    option ('my-call', .true., 'c', 'my callsign', 'CALL'),                  &
    option ('my-grid', .true., 'G', 'my grid locator', 'GRID'),              &
    option ('his-call', .true., 'x', 'his callsign', 'CALL'),                &
    option ('his-grid', .true., 'g', 'his grid locator', 'GRID') ]

  type(dec_data), allocatable :: shared_data
  character(len=12) :: mycall, hiscall
  character(len=6) :: mygrid, hisgrid
!  common/decstats/ntry65a,ntry65b,n65a,n65b,num9,numfano

  nsubmode = 0

  do
     call getopt('hs:e:a:b:r:m:p:d:f:w:t:964TL:S:H:c:G:x:g:',long_options,c,   &
          optarg,arglen,stat,offset,remain,.true.)
     if (stat .ne. 0) then
        exit
     end if
     select case (c)
        case ('h')
           display_help = .true.
        case ('s')
           read_files = .false.
           shm_key = optarg(:arglen)
        case ('e')
           exe_dir = optarg(:arglen)
        case ('a')
           data_dir = optarg(:arglen)
        case ('r')
           share_dir = optarg(:arglen)
        case ('b')
           nsubmode = ichar (optarg(:1)) - ichar ('A')
        case ('t')
           temp_dir = optarg(:arglen)
        case ('m')
           read (optarg(:arglen), *) nthreads
        case ('p')
           read (optarg(:arglen), *) ntrperiod
        case ('d')
           read (optarg(:arglen), *) ndepth
        case ('f')
           read (optarg(:arglen), *) nrxfreq
        case ('L')
           read (optarg(:arglen), *) flow
        case ('S')
           read (optarg(:arglen), *) fsplit
        case ('H')
           read (optarg(:arglen), *) fhigh
        case ('4')
           mode = 4
        case ('6')
           if (mode.lt.65) mode = mode + 65
        case ('9')
           if (mode.lt.9.or.mode.eq.65) mode = mode + 9
        case ('T')
           tx9 = .true.
        case ('w')
           read (optarg(:arglen), *) npatience
        case ('c')
           read (optarg(:arglen), *) mycall
        case ('G')
           read (optarg(:arglen), *) mygrid
        case ('x')
           read (optarg(:arglen), *) hiscall
        case ('g')
           read (optarg(:arglen), *) hisgrid
     end select
  end do

  if (display_help .or. stat .lt. 0                      &
       .or. (.not. read_files .and. remain .gt. 0)       &
       .or. (read_files .and. remain .lt. 1)) then

     print *, 'Usage: jt9 [OPTIONS] file1 [file2 ...]'
     print *, '       Reads data from *.wav files.'
     print *, ''
     print *, '       jt9 -s <key> [-w patience] [-m threads] [-e path] [-a path] [-t path]'
     print *, '       Gets data from shared memory region with key==<key>'
     print *, ''
     print *, 'OPTIONS:'
     print *, ''
     do i = 1, size (long_options)
       call long_options(i) % print (6)
     end do
     go to 999
  endif

  iret=fftwf_init_threads()            !Initialize FFTW threading 

! Default to 1 thread, but use nthreads for the big ones
  call fftwf_plan_with_nthreads(1)

! Import FFTW wisdom, if available
  wisfile=trim(data_dir)//'/jt9_wisdom.dat'// C_NULL_CHAR
  iret=fftwf_import_wisdom_from_filename(wisfile)

  ntry65a=0
  ntry65b=0
  n65a=0
  n65b=0
  num9=0
  numfano=0

  if (.not. read_files) then
     call jt9a()          !We're running under control of WSJT-X
     go to 999
  endif

  allocate(shared_data)
  nflatten=0

  do iarg = offset + 1, offset + remain
     call get_command_argument (iarg, optarg, arglen)
     infile = optarg(:arglen)
     call wav%read (infile)
     i1=index(infile,'.wav')
     if(i1.lt.1) i1=index(infile,'.WAV')
     if(infile(i1-5:i1-5).eq.'_') then
        read(infile(i1-4:i1-1),*,err=1) nutc
     else
        read(infile(i1-6:i1-3),*,err=1) nutc
     endif
     go to 2
1    nutc=0
2    nsps=0
     if(ntrperiod.eq.1)  then
        nsps=6912
        shared_data%params%nzhsym=181
     else if(ntrperiod.eq.2)  then
        nsps=15360
        shared_data%params%nzhsym=178
     else if(ntrperiod.eq.5)  then
        nsps=40960
        shared_data%params%nzhsym=172
     else if(ntrperiod.eq.10) then
        nsps=82944
        shared_data%params%nzhsym=171
     else if(ntrperiod.eq.30) then
        nsps=252000
        shared_data%params%nzhsym=167
     endif
     if(nsps.eq.0) stop 'Error: bad TRperiod'

     kstep=nsps/2
     k=0
     nhsym0=-999
     npts=(60*ntrperiod-6)*12000
     if(iarg .eq. offset + 1) then
        call init_timer (trim(data_dir)//'/timer.out')
        call timer('jt9     ',0)
     endif

     shared_data%id2=0          !??? Why is this necessary ???

     do iblk=1,npts/kstep
        k=iblk*kstep
        call timer('read_wav',0)
        read(unit=wav%lun,end=3) shared_data%id2(k-kstep+1:k)
        go to 4
3       call timer('read_wav',1)
        print*,'EOF on input file ',infile
        exit
4       call timer('read_wav',1)
        nhsym=(k-2048)/kstep
        if(nhsym.ge.1 .and. nhsym.ne.nhsym0) then
           if(mode.eq.9 .or. mode.eq.74) then
! Compute rough symbol spectra for the JT9 decoder
              ingain=0
              call timer('symspec ',0)
              nminw=1
              call symspec(shared_data,k,ntrperiod,nsps,ingain,nminw,pxdb,s,df3,   &
                   ihsym,npts8)
              call timer('symspec ',1)
           endif
           nhsym0=nhsym
           if(nhsym.ge.181) exit
        endif
     enddo
     close(unit=wav%lun)
     shared_data%params%nutc=nutc
     shared_data%params%ndiskdat=.true.
     shared_data%params%ntr=60
     shared_data%params%nfqso=nrxfreq
     shared_data%params%newdat=.true.
     shared_data%params%npts8=74736
     shared_data%params%nfa=flow
     shared_data%params%nfsplit=fsplit
     shared_data%params%nfb=fhigh
     shared_data%params%ntol=20
     shared_data%params%kin=64800
     shared_data%params%nzhsym=181
     shared_data%params%ndepth=ndepth
     shared_data%params%dttol=3.

     shared_data%params%minsync=-1      !### TEST ONLY
     shared_data%params%nfqso=1500      !### TEST ONLY
     mycall="K1ABC       "              !### TEST ONLY

     shared_data%params%naggressive=10
     shared_data%params%nranera=8                      !### ntrials=10000
     if(ndepth.eq.4) shared_data%params%nranera=10     !### ntrials=100,000
     shared_data%params%mycall=mycall
     shared_data%params%mygrid=mygrid
     shared_data%params%hiscall=hiscall
     shared_data%params%hisgrid=hisgrid
     shared_data%params%nswl=.false.
     shared_data%params%nfilter=.false.
     shared_data%params%nagcc=.false.
     shared_data%params%nlasttx=0
     shared_data%params%nprepass=2
     if (shared_data%params%mycall == '') shared_data%params%mycall='K1ABC'
     if (shared_data%params%hiscall == '') shared_data%params%hiscall='W9XYZ'
     if (shared_data%params%hisgrid == '') shared_data%params%hiscall='EN37'
     if (tx9) then
        shared_data%params%ntxmode=9
     else
        shared_data%params%ntxmode=65
     end if
     if (mode.eq.0) then
        shared_data%params%nmode=65+9
     else
        shared_data%params%nmode=mode
     end if
     shared_data%params%datetime="2013-Apr-16 15:13" !### Temp
     if(mode.eq.9 .and. fsplit.ne.2700) shared_data%params%nfa=fsplit
     call multimode_decoder(shared_data%ss,shared_data%id2,shared_data%params)
  enddo

  call timer('jt9     ',1)
  call timer('jt9     ',101)

999 continue
  ! Output decoder statistics
  call fini_timer ()
  open (unit=12, file=trim(data_dir)//'/timer.out', status='unknown', position='append')
  write(12,1100) n65a,ntry65a,n65b,ntry65b,numfano,num9
1100 format(58('-')/'   JT65_1  Tries_1  JT65_2 Tries_2    JT9   Tries'/  &
            58('-')/6i8)

! Save wisdom and free memory
  iret=fftwf_export_wisdom_to_filename(wisfile)
  call four2a(a,-1,1,1,1)
  call filbig(-1.,0,0,0,0)        !used for FFT plans
  call fftwf_cleanup_threads()
  call fftwf_cleanup()
end program jt9