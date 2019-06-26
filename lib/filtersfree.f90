! This source code file was last time modified by Igor UA3DJY on February 10th, 2017
! All changes are shown in the patch file coming together with the full JTDX source code.

subroutine filtersfree(decoded,falsedec)
  
  real datacorr
  character decoded*22
  logical(1) falsedec

  ndot=0
  nsign=0
  nother=0

  if(decoded(11:12).eq.'73') go to 4
  if(decoded(12:13).eq.'73') go to 4
  if(decoded(1:1).eq.'/') go to 2

  do i=1,13

     if(decoded(i:(i+1)).eq.'73' .or. decoded(i:(i+1)).eq.'TU' .or. decoded(i:(i+1)).eq.'GL') go to 4

     if(decoded(i:(i+2)).eq.'QSL' .or. decoded(i:(i+2)).eq.'TNX' .or. decoded(i:(i+1)).eq.'RR' .or.   &
        decoded(i:(i+2)).eq.'QSO' .or. decoded(i:(i+2)).eq.'CFM' .or. decoded(i:(i+3)).eq.'LOTW' .or. &
        decoded(i:(i+2)).eq.'BND' .or. decoded(i:(i+2)).eq.'QSY' .or. decoded(i:(i+3)).eq.'BAND') go to 4

     if(decoded(i:(i+2)).eq.'MAS' .or. decoded(i:(i+2)).eq.'HNY' .or. decoded(i:(i+2)).eq.'HPY' .or.   &
        decoded(i:(i+2)).eq.'XMS' .or. decoded(i:(i+5)).eq.'EASTER') go to 4

     if(decoded(i:(i+4)).eq.'/HYBR' .or. decoded(i:(i+4)).eq.'/HTML' .or. &
        decoded(i:(i+4)).eq.'/PHOT') go to 4

     if(decoded(i:i).eq.'.') ndot=ndot+1
     if(decoded(i:i).eq.'-' .or. decoded(i:i).eq.'+' .or. decoded(i:i).eq.'?') nsign=nsign+1
     if(decoded(i:i).eq.'/' .or. decoded(i:i).eq.'?') nother=nother+1

  enddo

  if(ndot.ge.2 .or. nsign.ge.2 .or. nother.ge.2) go to 2
  if(decoded(1:2).eq.'-0')  go to 2
  if(decoded(1:1).ne.' ' .and. decoded(2:2).eq.' ') go to 2

  if(((decoded(13:13).ge.'A' .and. decoded(13:13).le.'Z') .or.  &
     (decoded(13:13).ge.'0' .and. decoded(13:13).le.'9')) .and. &
     decoded(12:12).eq.' ') go to 2

  if(decoded(1:1).eq.'.' .or. decoded(1:1).eq.'+' .or. decoded(1:1).eq.'?' &
     .or. decoded(1:1).eq.'/' .or. decoded(13:13).eq.'.' .or. decoded(13:13).eq.'+' &
     .or. decoded(13:13).eq.'-' .or. decoded(13:13).eq.'/') go to 2

  if(decoded(2:2).eq.'-' .or. decoded(2:2).eq.'+' .or. decoded(12:12).eq.'.' .or. &
     decoded(12:12).eq.'+' .or. decoded(12:12).eq.'-') go to 2

  if(decoded(12:12).ge.'0' .and. decoded(12:12).le.'9' .and. &
     decoded(13:13).ge.'0' .and. decoded(13:13).le.'9') then

     if(decoded(12:13).ne.'55' .and. decoded(12:13).ne.'73' .and. decoded(12:13).ne.'88') go to 2

  endif
 
  if(decoded(1:1).eq.'0' .and. decoded(2:2).ne.'.') go to 2

  do i=1,12
     if(i.lt.12 .and. decoded(i:i).eq."?" .and. decoded(i+1:i+1).ne." ") go to 2
     if(i.lt.12 .and. decoded(i:i).ne." " .and. decoded(i+1:i+1).eq."+" &
        .and. (decoded(i+2:i+2).le."0" .or. decoded(i+2:i+2).ge."9")) go to 2

     if(decoded(i:i).ge.'A' .and. decoded(i:i).le.'Z' .and. decoded(i+1:i+1).eq.'.' .and. &
        decoded(i+2:i+2).ge.'A' .and. decoded(i+2:i+2).le.'Z') go to 2

     if(decoded(i+1:i+2).eq.'/ ' .or. decoded(i+1:i+2).eq.' /') go to 2

     if(decoded(i:i).ge.'0' .and. decoded(i:i).le.'9' .and. decoded(i+1:i+1).eq.'/' .and. &
        decoded(i+2:i+2).ge.'0' .and. decoded(i+2:i+2).le.'9') go to 2

     if(decoded(i:i).eq.' ' .and. decoded(i+1:i+1).ge.'A' .and. decoded(i+1:i+1).le.'Z' .and.     &
        decoded(i+1:i+1).ne.'F' .and. decoded(i+1:i+1).ne.'G' .and. decoded(i+1:i+1).ne.'I' .and. &
        decoded(i+2:i+2).eq.'/') go to 2

     if(decoded(i:i).eq.' ' .and. decoded(i+1:i+1).ge.'A' .and. decoded(i+1:i+1).le.'Z' .and.     &
        (decoded(i+2:i+2).eq.'.' .or. decoded(i+2:i+2).eq.'+' .or. decoded(i+2:i+2).eq.'-')) go to 2

  enddo

  if(decoded(1:1).eq.'-' .and. decoded(2:2).le.'1' .and. decoded(2:2).ge.'9') go to 2
 
  if(decoded(1:1).ge.'1' .and. decoded(1:1).le.'9' .and.  &
     decoded(2:2).ge.'0' .and. decoded(2:2).le.'9' .and.  &
     decoded(3:3).ge.'0' .and. decoded(3:3).le.'9' .and.  &
     decoded(4:4).ne.'W') go to 2
 
  if(decoded(1:1).ge.'1' .and. decoded(1:1).le.'9' .and.  &
     decoded(2:2).ge.'0' .and. decoded(2:2).le.'9' .and.  &
     decoded(3:3).ne.'W') go to 2

  if(decoded(1:1).ge.'1' .and. decoded(1:1).le.'9' .and.  &
     decoded(2:3).ne.'EL') go to 2

  if(decoded(10:10).ne.'/' .and. decoded(11:11).ne.'/' .and. decoded(12:12).ge.'A' .and. &
     decoded(12:12).le.'Z' .and. decoded(13:13).ge.'0' .and. decoded(12:12).le.'9') go to 2

  call datacor(datapwr,datacorr)

! ban free message if datacorr is less than 1.55
  if(datacorr.lt.1.55) go to 2

  ncount=0
  nchar=0

  do i=1,13
     if(decoded(i:i).eq.' ') exit
     if(decoded(i:i).eq.'/' .or. decoded(i:i).eq.'-' .or. decoded(i:i).eq.'.') nchar=nchar+1
     ncount=ncount+1
  enddo

  if(ncount.eq.13 .and. nchar.ge.0) go to 2  

!print *,"passed"
!print *,decoded

  go to 4

2 falsedec=.true.
!print *,"stopped"
!print *,decoded
 return

4 return
end subroutine filtersfree
