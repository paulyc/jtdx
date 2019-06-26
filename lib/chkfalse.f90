! This source code file is last time modified by Igor UA3DJY on February 15th, 2017


subroutine chkfalse(decoded,falsedec)

  use jt65_mod9 ! used to store all callsigns seen in JT modes in the memory
 
  character*22 decoded
  logical(1) falsedec

  if(first) then
     open(23,file=trim(share_dir)//'/ALLCALL.TXT',status='unknown')
     icall=0
     j=0
     do i=1,MAXCALLS
        read(23,1002,end=10) line
1002    format(a80)
        if(line(1:4).eq.'ZZZZ') exit
        if(line(1:2).eq.'//') cycle
        i1=index(line,',')
        if(i1.lt.4 .or. i1.gt.7) cycle
        callsign=line(1:i1-1)
        j=j+1
        call1(j)=callsign(1:6)
     enddo
10   ncalls=j
     if(ncalls.lt.1) print *, 'ALLCALL.TXT is too short or missed?'
     close(23)
     first=.false.
  endif

     if(ncalls.lt.16000) return 
     i2=index(decoded,' ')
     i3=index(decoded((i2+1):),' ')
     falsedec=.true.
     if(i2.lt.4 .or. i3.lt.4) return
     do i=1,ncalls
        if(call1(i).eq.decoded(1:(i2-1)) .or. call1(i).eq.decoded((i2+1):(i2+i3-1))) &
           falsedec=.false.
     enddo

return
end subroutine chkfalse
