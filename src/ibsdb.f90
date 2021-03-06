! **************************************************************************
! Note by F. Antoniou and F. Zimmermann March 2012
! Note that in this version the dispersion is corrected (multiplied by beta) 
! within the module as in the twiss table the disperion is given in the pt
! frame (dx/dpt) while for the ibs calculations the dx/dp is needed.
!
! 12/02/2019 A. Saa Hernandez. Formulas for coasting beam corrected
! ***************************************************************************

subroutine ibs
  use ibsdbfi
  use name_lenfi
  use math_constfi, only : zero, one, two, half
  use phys_constfi
  implicit none
  !----------------------------------------------------------------------*
  ! Purpose:                                                             *
  !   INTRABEAM SCATTERING, IBS Command                                  *
  !   These routines are a much reduced version of IBS as taken          *
  !   from the program ZAP, written by M. Zisman.                        *
  !   One should refer to the ZAP USERS MANUAL LBL-21270 UC-28.          *
  ! Attribute:                                                           *
  !   TABLE     (name)    Name of Twiss table.                           *
  !----------------------------------------------------------------------*

  integer :: step, i, j, flag, testtype, range(2), n
  double precision :: alx, alxbar, alxwtd, aly, alybar, alywtd
  double precision :: betax, betay, beteff, beteffy, bxbar, bxinv, bybar, byinv, bywtd
  double precision :: ax1, ax2, ay1, ay2, bx1, bx2, by1, by2
  double precision :: dx, dx1, dx2, dxbar, dxwtd, dpx, dpx1, dpx2, dpxbr, dpxwtd
  double precision :: dy, dy1, dy2, dybar, dywtd, dpy, dpy1, dpy2, dpybr, dpywtd
  double precision :: taul, taux, tauy, tavl, tavlc, tavx, tavxc, tavy, tavyc
  double precision :: tlbar, tlidc, tlwtd, txbar, txidc, txwtd, tybar, tyidc, tywtd
  double precision :: salxb, salyb, sbxb, sbxinv, sbyb, sbyinv, sdpxb, sdxb, sdpyb, sdyb
  double precision :: hscrpt, hscwtd, hscrpty, hscwtdy
  double precision :: s1, s2, ss2, l1, l2, ll2, const, dels, wnorm, sdum, tol

  integer, external :: get_option, double_from_table_row, restart_sequ, advance_to_pos
  double precision, external :: get_value
  
  ! ************* Get the parameters for the common blocks *************
  ! *************         /machin/ and /beamdb/            *************
  charge   = get_value('probe ', 'charge ')
  gammas   = get_value('probe ', 'gamma ')
  gamma    = get_value('probe ', 'gamma ')
  en0      = get_value('probe ', 'energy ')
  amass    = get_value('probe ', 'mass ')
  ex       = get_value('probe ', 'ex ')
  ey       = get_value('probe ', 'ey ')
  et       = get_value('probe ', 'et ')
  sigt     = get_value('probe ', 'sigt ')
  sige     = get_value('probe ', 'sige ')
  parnum   = get_value('probe ', 'npart ')
  circ     = get_value('probe ', 'circ ')
  currnt   = get_value('probe ', 'bcurrent ')
  betas    = get_value('probe ', 'beta ')
  beta     = get_value('probe ', 'beta ')
  arad     = get_value('probe ', 'arad ')
  alfa     = get_value('probe ', 'alfa ')
  freq0    = get_value('probe ', 'freq0 ')
  bunch    = get_value('probe ', 'kbunch ')
  
  ! NOTE:
  !****************************************************************
  ! Sige is the dE/E. dp/p needed as input for the IBS calculations
  ! dp/p= (dE/E)/beta**2
  !*****************************************************************
  sige    = sige/beta/beta
  print *, 'sige ', sige
  
  !---- Initialize variables to accumulate weighted average lifetimes.
  tavlc  = zero;  tavxc  = zero;  tavyc  = zero
  sbxb   = zero;  salxb  = zero;  sdxb   = zero;  sdpxb  = zero  
  sbyb   = zero;  salyb  = zero;  sdyb   = zero;  sdpyb  = zero
  sbxinv = zero;  sbyinv = zero
                  alxwtd = zero;  dxwtd  = zero;  dpxwtd = zero
  bywtd  = zero;  alywtd = zero;  dywtd  = zero;  dpywtd = zero
  hscwtd = zero;  hscwtdy= zero
  wnorm  = zero

  ! ****** Start new Twiss Table reading *****************
  !
  step = get_value('ibs ', 'steps ')
  tol  = get_value('ibs ', 'tolerance ')

  call table_range('twiss ', '#s/#e ', range)
  
  flag = double_from_table_row('twiss ', 's ',    range(1), s1);  if (flag .ne. 0)  goto 102
  flag = double_from_table_row('twiss ', 'l ',    range(1), l1);  if (flag .ne. 0)  goto 102
  flag = double_from_table_row('twiss ', 'betx ', range(1), bx1); if (flag .ne. 0)  goto 102
  flag = double_from_table_row('twiss ', 'bety ', range(1), by1); if (flag .ne. 0)  goto 102
  flag = double_from_table_row('twiss ', 'alfx ', range(1), ax1); if (flag .ne. 0)  goto 102
  flag = double_from_table_row('twiss ', 'alfy ', range(1), ay1); if (flag .ne. 0)  goto 102
  flag = double_from_table_row('twiss ', 'dx ',   range(1), dx1); if (flag .ne. 0)  goto 102
  flag = double_from_table_row('twiss ', 'dpx ',  range(1), dpx1);if (flag .ne. 0)  goto 102
  flag = double_from_table_row('twiss ', 'dy ',   range(1), dy1); if (flag .ne. 0)  goto 102
  flag = double_from_table_row('twiss ', 'dpy ',  range(1), dpy1);if (flag .ne. 0)  goto 102
  

  ! NOTE by F.A & F.Z
  ! ************************************************************************************
  ! Added 16.01.2012 to check if the twiss is taken at the center (testtype=2) or the 
  ! exit (testtype=1) of the elements.
  ! If testtype=1 linear interpolation is used to calculate the twiss at the center of 
  ! the elements.
  !*************************************************************************************

  j = restart_sequ()
  do i=range(1)+1, range(2)     
     j = advance_to_pos('twiss ', i)
     flag = double_from_table_row('twiss ', 's ', i, ss2); if (flag .ne. 0)  goto 102
     ! 2016-Feb-24  11:23:03  ghislain: 
     !flag = double_from_table_row('twiss ', 'l ', i, ll2); if (flag .gt. 0)  goto 102
     flag = double_from_table_row('twiss ', 'l ', i, ll2); if (flag .ne. 0)  goto 102
     if (ll2 .gt. 0.0001) exit ! break loop
  enddo
  
  if ((ss2-s1) .eq. ll2) then
     testtype = 1
     print *, 'Twiss was calculated at the exit of the elements.' 
     print *, 'Twiss functions at the center of the elements are calculated through linear interpolation'
  else if ((ss2-s1) .eq. (l1+ll2)/2) then
     testtype = 2
     print *, 'Twiss was calculated at the center of the elements. No interpolation is used'
  endif

  ! ************** Check if "ibs_table" required  ****************
  n = get_option('ibs_table ')

  ! ********** Start Do loop ***************
  j = restart_sequ()
  do i = range(1)+1, range(2)
     j = advance_to_pos('twiss ', i)
     flag = double_from_table_row('twiss ', 's ',    i, s2);   if (flag .ne. 0)  goto 102
     flag = double_from_table_row('twiss ', 'l ',    i, l2);   if (flag .ne. 0)  goto 102
     flag = double_from_table_row('twiss ', 'betx ', i, bx2);  if (flag .ne. 0)  goto 102
     flag = double_from_table_row('twiss ', 'bety ', i, by2);  if (flag .ne. 0)  goto 102
     flag = double_from_table_row('twiss ', 'alfx ', i, ax2);  if (flag .ne. 0)  goto 102
     flag = double_from_table_row('twiss ', 'alfy ', i, ay2);  if (flag .ne. 0)  goto 102
     flag = double_from_table_row('twiss ', 'dx ',   i, dx2);  if (flag .ne. 0)  goto 102
     flag = double_from_table_row('twiss ', 'dpx ',  i, dpx2); if (flag .ne. 0)  goto 102
     flag = double_from_table_row('twiss ', 'dy ',   i, dy2);  if (flag .ne. 0)  goto 102
     flag = double_from_table_row('twiss ', 'dpy ',  i, dpy2); if (flag .ne. 0)  goto 102

    
    !  NOTE by F.A & F.Z
    ! ************************************************************************************
    ! Dispersion and Dispersion prime is multiplied by beta, in order to be in the deltap 
    ! and not the pt frame. This correction is necessary for non-relativistic beams
    !*************************************************************************************
	
     if (testtype .eq. 1) then
	dels = s2-s1
	sdum = half * (s2 + s1)
        betax  = half * (bx2 + bx1)
	betay  = half * (by2 + by1)
	alx    = half * (ax2 + ax1)
	aly    = half * (ay2 + ay1)
	dx     = beta * half * (dx2 + dx1)
	dpx    = beta * half * (dpx2 + dpx1)
	dy     = beta * half * (dy2 + dy1)
	dpy    = beta * half * (dpy2 + dpy1)

     else if (testtype .eq. 2) then
	dels = l2
	sdum = s2
	betax  = bx2
	betay  = by2
	alx    = ax2
	aly    = ay2
	dx     = beta * dx2
	dpx    = beta * dpx2
	dy     = beta * dy2
	dpy    = beta * dpy2
    endif

    sbxb   = sbxb + betax * dels
    sbxinv = sbxinv + dels / betax
    sbyb   = sbyb + betay * dels
    sbyinv = sbyinv + dels / betay
    salxb  = salxb + alx * dels
    salyb  = salyb + aly * dels
    sdxb   = sdxb + dx * dels
    sdpxb  = sdpxb + dpx * dels
    sdyb   = sdyb + dy * dels
    sdpyb  = sdpyb + dpy * dels
 
    !*---- Calculate weighted average in region of non-zero DX's.
    !     These values are used to calculate "average" ring lifetimes
    !     in TWSINT.
    if (dx .gt. zero) then
       wnorm  = wnorm + dels
       dxwtd  = dxwtd + dels * dx
       dpxwtd = dpxwtd + dels * dpx
       dywtd  = dywtd + dels * dy
       dpywtd = dpywtd + dels * dpy
       bywtd  = bywtd + dels / sqrt(betay)
       alxwtd = alxwtd + dels * alx
       alywtd = alywtd + dels * aly
       hscrpt  = betax * dpx**2 + two * alx * dx * dpx + (one + alx**2) * dx**2 / betax
       hscrpty = betay * dpy**2 + two * aly * dy * dpy + (one + aly**2) * dy**2 / betay
       hscwtd  = hscwtd + dels * sqrt(hscrpt)
       hscwtdy = hscwtdy + dels * sqrt(hscrpty)
     endif

     !---- TWSINT calculates the Bjorken/Mtingwa integral.
     call twsint(betax, betay, alx, aly, dx, dpx, dy, dpy, txidc, tyidc, tlidc)

     !---- Accumulate contributions.
     tavlc = tavlc + tlidc * dels
     tavxc = tavxc + txidc * dels
     tavyc = tavyc + tyidc * dels

     ! *************** Fill "ibs_table" if required *********************

     if (n .ne. 0) then
        call string_to_table_curr('ibs ', 'name ', 'name ')
        call double_to_table_curr('ibs ','s ',    sdum)
        call double_to_table_curr('ibs ','dels ', dels)
        call double_to_table_curr('ibs ','tli ',  tlidc)
        call double_to_table_curr('ibs ','txi ',  txidc)
        call double_to_table_curr('ibs ','tyi ',  tyidc)        
        call double_to_table_curr('ibs ','betx ', betax)
        call double_to_table_curr('ibs ','alfx ', alx)
        call double_to_table_curr('ibs ','dx ',   dx)
        call double_to_table_curr('ibs ','dpx ',  dpx)
        call double_to_table_curr('ibs ','bety ', betay)
        call double_to_table_curr('ibs ','alfy ', aly)
        call double_to_table_curr('ibs ','dy ',   dy)
        call double_to_table_curr('ibs ','dpy ',  dpy)
        call augment_count('ibs ')
     endif

     ! *********** Make sure the following lines are not moved ******
     ! *********** by the compiler ***************
     s1   = s2
     bx1  = bx2
     by1  = by2
     ax1  = ax2
     ay1  = ay2
     dx1  = dx2
     dpx1 = dpx2
     dy1  = dy2	
     dpy1 = dpy2
     
  enddo

  !---- We have finished reading the lattice from MAD
  bxbar  = sbxb / s2
  bybar  = sbyb / s2
  alxbar = salxb / s2
  alybar = salyb / s2
  dxbar  = sdxb / s2
  dpxbr  = sdpxb / s2
  dybar  = sdyb / s2
  dpybr  = sdpyb / s2
  bxinv  = sbxinv / s2
  byinv  = sbyinv / s2

  dxwtd  = dxwtd / wnorm
  dpxwtd = dpxwtd / wnorm
  dywtd  = dywtd / wnorm
  dpywtd = dpywtd / wnorm
  bywtd  = bywtd / wnorm
  bywtd  = one / bywtd**2
  alxwtd = alxwtd / wnorm
  alywtd = alywtd / wnorm
  hscwtd = (hscwtd/wnorm)**2
  beteff = dxwtd**2 / hscwtd
  if (hscwtdy.ne.0.d0) then
     beteffy = dywtd**2 / hscwtdy
  else
     beteffy = bywtd
  endif
  ! ********** Compute beam sizes with average betas ************

  sigx = sqrt(ex * bxbar + (dx * sige)**2)
  sigy = sqrt(ey * bybar + (dy * sige)**2)

  call enprgl
  call enprem
  call cavprt()
  ! ************************************************************

  !---- Integral for averaged quantities.
  call twsint(bxbar,bybar,alxbar,alybar, dxbar,dpxbr, dybar,dpybr,txbar,tybar,tlbar)

  !---- Integral for effective quantities.
  call twsint(beteff,beteffy,alxwtd,alywtd,dxwtd,dpxwtd, dywtd,dpywtd,txwtd,tywtd,tlwtd)

  !---- Calculate the Coulomb logarithm.
  call twclog(bxbar, bybar, dxbar, dybar, const)

  !---- Output (weighted) average values.
  write (*,'(/a/)') " Ring average values (m) "

  write (*,'(5x,a,1pe13.5,4x,a,1pe13.5,4x,a,1pe12.5,4x,a,1pe12.5)') & 
       "betx   = ",bxbar,  "bety   = ",bybar, "Dx  = ",dxbar, "Dy  = ",dybar

  write (*,'(5x,a,1pe13.5,4x,a,1pe13.5,4x,a,1pe12.5,4x,a,1pe12.5)') & 
       "alfx   = ",alxbar, "alfy   = ",alybar, "Dpx = ",dpxbr, "Dpy = ",dpybr

  write (*,'(5x,a,1pe13.5,4x,a,1pe13.5)')  "1/betx = ",bxinv, "1/bety = ",byinv

  !---- Output averaged values.
  tavl   = tavlc * const / s2
  tavx   = tavxc * const / s2
  tavy   = tavyc * const / s2

  taul   = one / tavl
  taux   = one / tavx
  tauy   = one / tavy
  
  call set_variable('ibs.tx ',taux)
  call set_variable('ibs.ty ',tauy)
  call set_variable('ibs.tl ',taul)
  
  write (*,'(/5x,a)')           "(Weighted) average rates (1/sec):" 
  write (*,'( 5x,a,1p,es15.6)') "Longitudinal= ",tavl
  write (*,'( 5x,a,   es15.6)') "Horizontal  = ",tavx
  write (*,'( 5x,a,   es15.6)') "Vertical    = ",tavy
  write (*,'(/5x,a)')           "(Weighted) average lifetimes (sec):"
  write (*,'( 5x,a,1p,es15.6)') "Longitudinal= ",taul
  write (*,'( 5x,a,   es15.6)') "Horizontal  = ",taux
  write (*,'( 5x,a,   es15.6/)') "Vertical    = ",tauy

  return

102 call fort_fail('IBS: ', 'table value not found, rest skipped, program stops ')
       
end subroutine ibs

subroutine enprgl
  use ibsdbfi
  use math_constfi, only : zero, one
  use code_constfi
  implicit none
  !----------------------------------------------------------------------*
  ! Purpose:                                                             *
  !   Print global data for machine.                                     *
  !----------------------------------------------------------------------*
  logical :: radiate
  double precision :: eta, gamtr, t0

  double precision, external :: get_value
  
  radiate = get_value('probe ','radiate ') .ne. 0

  !---- Global parameters.
  gamtr = zero
  if (alfa .ne. zero) gamtr = sign(one,alfa) * sqrt( one / abs(alfa))

  t0 = one / freq0
  eta = alfa - one / gamma**2

  write (*,'(/,a,/)') " Global parameters for the machine: "

  write (*,'(a,l1,a/)') "radiate = ",radiate,":"

  write (*,'(t6,a,t16,f14.6,a,t46,a,t56,f14.6,a,t86,a,t96,f14.6,a)') &
       "C",circ," m",  "f0",freq0," MHz",   "T0",t0," microseconds"
  write (*,'(t6,a,t16,e18.6,t46,a,t56,e18.6,t86,a,t96,f14.6)') &
       "alfa",alfa, "eta",eta, "gamma(tr)",gamtr
  write (*,'(t6,a,t16,f14.6,a,t46,a,t56,i6,t86,a,t96,e18.6,a)') &
       "Bcurrent",currnt," A/bunch",   "Kbunch",bunch,    "Npart",parnum," per bunch"
  write (*,'(t6,a,t16,f14.6,a,t46,a,t56,f14.6,t86,a,t96,f14.6)') & 
       "E",en0," GeV",   "gamma",gamma,    "beta",beta

end subroutine enprgl

subroutine enprem
  use ibsdbfi
  use math_constfi, only : ten6p, ten3p
  implicit none
  !----------------------------------------------------------------------*
  ! Purpose:                                                             *
  !   Print emittances and sigmas.                                       *
  !----------------------------------------------------------------------*

  write (*,'(/a/)') " Emittances:"
  write (*,'(t6,a,t16,e16.6,a,t48,a,t58,f14.6,a)') & 
       "Ex",ten6p*ex," pi*mm*mrad","sigx",ten3p*sigx," mm"
  write (*,'(t6,a,t16,e16.6,a,t48,a,t58,f14.6,a)') & 
       "Ey",ten6p*ey," pi*mm*mrad","sigy",ten3p*sigy," mm"
  write (*,'(t6,a,t16,e16.6,a,t48,a,t58,f14.6,a,t88,a,t96,f14.6,a/)') &
       "Et",ten6p*et," pi*mm*mrad","sigt",ten3p*sigt, " mm", &
       "sigE",ten3p*sige," 1/1000"

end subroutine enprem

subroutine cavprt()
  use name_lenfi
  use code_constfi
  implicit none

  integer :: i, lg
  double precision :: el, rfv, rff, rfl, deltap  
  character(len=name_len) :: sequ_name, el_name

  integer, external :: get_string, restart_sequ, advance_node
  double precision, external ::  get_value, node_value

  lg = get_string('sequence ', 'name ', sequ_name)
  if (lg .gt. 0) write(*,'("sequence name: ",a/)') sequ_name(:lg)
  i = restart_sequ()

  do
     if (node_value('mad8_type ') .eq. code_rfcavity) then
        lg = get_string('element ', 'name ', el_name)
        el = node_value('l ')
        rfv = node_value('volt ')
        rff = node_value('freq ')
        rfl = node_value('lag ')
        deltap = get_value('probe ','deltap ')
        print '(a,5g14.6)', el_name(:lg), el, rfv, rff, rfl, deltap
     endif
     if  (advance_node().eq.0) exit
  end do

end subroutine cavprt

subroutine twclog(bxbar, bybar, dxbar, dybar, const)
  use ibsdbfi
  use math_constfi, only : zero, two, four, eight, pi
  use phys_constfi, only : hbar, clight, qelect
  implicit none
  !----------------------------------------------------------------------*
  ! Purpose:                                                             *
  !   Calculation of Coulomb logarithm (and print)                       *
  !   based on the formulae in AIP physics vade mecum p.264 (1981)       *
  ! Input:                                                               *
  !   BXBAR     (real)    Average horizontal beta.                       *
  !   BYBAR     (real)    Average vertical beta.                         *
  ! Output:                                                              *
  !   CONST     (real)    Constant in eq. (IV.9.1), ZAP user's manual.   *
  !----------------------------------------------------------------------*
  double precision :: bxbar, bybar, dxbar, dybar, const

  logical :: fbch
  double precision :: bgam, cbunch, coulog 
  double precision :: debyel, densty, etrans, pnbtot, qion, tempev, vol
  double precision :: rmax, rmin, rmincl, rminqm, sigtcm, sigxcm, sigycm

  double precision, external :: get_value

  double precision, parameter :: ot2=1d2, ft8=5d8, ot5=1d5, ttm3=2d-3
  double precision, parameter :: fac1=743.4d0, fac2=1.44d-7

  ! **************************** DB *********************
  fbch = get_value('probe ', 'bunched ') .ne. 0

  !---- Calculate transverse temperature as 2*P*X',
  !     i.e., assume the transverse energy is temperature/2.
  qion   = abs(charge)
  etrans = ft8 * (gammas * en0 - amass) * (ex / bxbar)
  tempev = two * etrans

  !---- Calculate beam volume to get density (in cm**-3).
  sigxcm = ot2 * sqrt(ex * bxbar + (dxbar * sige)**2)
  sigycm = ot2 * sqrt(ey * bybar + (dybar * sige)**2)
  sigtcm = ot2 * sigt
  if (fbch) then
     vol    = eight * sqrt(pi**3) * sigxcm * sigycm * sigtcm
     densty = parnum / vol
  else
     vol    = four * pi * sigxcm * sigycm * ot2 * circ
     densty = parnum / vol
  endif

  !---- Calculate RMAX as smaller of SIGXCM and DEBYE length.
  debyel = fac1 * sqrt(tempev/densty) / qion
  rmax   = min(sigxcm,debyel)

  !---- Calculate RMIN as larger of classical distance of closest approach
  !     or quantum mechanical diffraction limit from nuclear radius.
  rmincl = fac2 * qion**2 / tempev
  rminqm = hbar*clight*ot5 / (two*sqrt(ttm3*etrans*amass))
  rmin   = max(rmincl,rminqm)
  coulog = log(rmax/rmin)
  bgam = betas * gammas
  if (fbch) then
     const = parnum * coulog * arad**2 * clight / & 
          (eight * pi * betas **3 * gammas**4 * ex * ey * sige * sigt)
     cbunch = qion * parnum * qelect * betas * clight / circ
  else
     const = parnum * coulog * arad**2 * clight / &
          (four * sqrt(pi) * betas **3 * gammas**4 * ex * ey * sige * circ)
  endif

  write (*,'(/t6,a,1p,e14.6)')       "CONST               = ",const

  write (*,'(/5x,a,f14.6,a)')        "ENERGY              = ",en0," GeV"
  write (*,'( 5x,a,f14.6)')          "BETA                = ",betas
  write (*,'( 5x,a,f14.3)')          "GAMMA               = ",gammas
  write (*,'( 5x,a,f14.3)')          "COULOMB LOG         = ",coulog

  !---- Print warning here if Coulomb logarithm gave bad results.
  !     Usually this error is due to a starting guess far from
  !     the equilibrium value.
  if (coulog .lt. zero) &
     call fort_warn('TWCLOG: ', 'Coulomb logarithm gives invalid result --- check input parameters.')

  write (*,'(/5x,a,1p,e14.6,a)')     "X-emittance         = ",ex," m*rad"
  write (*, '(5x,a,1p,e14.6,a/)')    "Y-emittance         = ",ey," m*rad"

  if (fbch) then
     write (*,'(5x,a,1p,e14.6)')    "Momentum spread     = ",sige
     write (*,'(5x,a,0p,f14.6,a/)') "Bunch length        = ",sigt," m"
     write (*,'(5x,a,1p,e14.6)')    "Particles per bunch = ",parnum
     write (*,'(5x,a,1p,e14.6,a)')  "Bunch current       = ",cbunch," A"          
  else
     write (*,'(5x,a,1p,e14.6/)')   "Momentum spread     = ",sige
     write (*,'(5x,a,0p,f14.6,a)')  "Current             = ",currnt," A"
  endif

end subroutine twclog

subroutine twsint(betax, betay, alx, aly, dx, dpx, dy, dpy, txi, tyi, tli)
  use ibsdbfi
  !use physconsfi
  use math_constfi, only : zero, one, two, three, four, six, ten
  implicit none
  !----------------------------------------------------------------------*
  ! Purpose:                                                             *
  !   Subroutine uses Simpson's rule integration                         *
  !   to calculate Bjorken/Mtingwa integrals (eqn. 3.4)                  *
  !   Particle Accelerators 13, 115 (1983)                               *
  !                                                                      *
  !   The expressions found in Conte/Martini                             *
  !   Particle Accelerators 17, 1 (1985) contain two false               *
  !   terms in the expression for tau_x which have been corrected        *
  !   in this version of MADX;                                           *
  !   contributions from vertical dispersion were also added;            *
  !   AB Note by Frank Zimmermann be published (2005)                    *
  !                                                                      *
  !                                                                      *
  !   Integrals are broken into decades to optimize speed.               *
  !                                                                      *
  !   For the VAX, values may not exceed 10**33, therefore TSTLOG=33     *
  !   For the IBM, values may not exceed 10**74, therefore TSTLOG=74     *
  !   (PMG, March 1988)                                                  *
  !                                                                      *
  !   The integral is split into MAXDEC decades with NS steps /decade.   *
  !   TEST is used for testing convergence of the integral               *
  ! Input:                                                               *
  !   BETAX     (real)    Horizontal beta.                               *
  !   BETAY     (real)    Vertical beta.                                 *
  !   ALX       (real)    Horizontal alpha.                              *
  !   ALY       (real)    Vertical alpha.                                *
  !   DX        (real)    Horizontal dispersion.                         *
  !   DPX       (real)    Derivative of horizontal dispersion.           *
  !   DY        (real)    Vertical dispersion.                           *
  !   DPY       (real)    Derivative of vertical dispersion.             *
  ! Output:                                                              *
  !   TXI       (real)    Horizontal rate / const.                       *
  !   TYI       (real)    Vertical rate / const.                         *
  !   TLI       (real)    Longitudinal rate / const.                     *
  !----------------------------------------------------------------------*
  double precision, intent(IN) :: betax, betay,  alx, aly, dx, dpx, dy, dpy
  double precision, intent(OUT) :: txi, tyi, tli

  integer :: iiz, iloop
  integer, parameter :: maxdec=30, ns=50

  double precision :: a, b, am, c1, c2, c3, c1y, c2y, chy, cx, cy, cl, r1
  double precision :: cscale, chklog, cprime, ccy
  double precision :: zintl, zintx, zinty
  double precision :: td1, td2, tl1, tl2, tx1, tx2, ty1, ty2
  double precision :: al(31), bl(30), h, aloop
  double precision :: term, func, polyl, polyx, polyy, suml, sumx, sumy
  double precision :: cof, f, alam, phi, phiy, tmpl, tmpx, tmpy

  double precision, parameter :: onetominus20=1d-20
  double precision, parameter :: tstlog=74d0, power=-two/three, test=1d-7

  double precision :: coeff(2)
  data coeff / 2d0, 4d0 /

  phi    = dpx + (alx * dx / betax)
  phiy   = dpy + (aly * dy / betay)
  am     = one
  c1     = (gammas * dx)**2 / (ex * betax)
  c1y    = (gammas * dy)**2 / (ey * betay)
  c3     = betax / ex
  c2     = c3 * (gammas*phi)**2
  cx     = c1 + c2
  cl     = am * (gammas/sige)**2
  cy     = betay / ey
  c2y    = cy * (gammas*phiy)**2
  chy    = c1y + c2y
  r1     = three / cy
  a      = cx + cl + chy + c3 + cy
!--- corrected b 23.02.2011 
  b      = (c3 + cy) * (c1 + cl+c1y) + cy * c2 + c3 * c2y+ c3 * cy

  !---- Define CPRIME=C*CSCALE to try to keep the value.
  !     small enough for the VAX in single precision or
  !     IBM in double precision.
  !     Test LOG(C) to see if it needs scaling
  cscale = one
  chklog = log10(c3) + log10(cy) + log10(c1 + cl)
  if (chklog .gt. tstlog) cscale = ten**(tstlog-chklog)
  cprime = c3 * cy * cscale * (c1 + cl + c1y)

  !---- Split integral into decades, with NS steps per decade.
  !     variables to save integral segments
  zintl  = zero
  zintx  = zero
  zinty  = zero

  !---- Constants for integration loop.
  !     To keep the numbers reasonable, the numerator is
  !     scaled by 1/CPRIME and the denominator by 1/CPRIME**2.
  !     The extra factor of CPRIME is accounted for after integrating
  ccy    = cprime**power
  td1    = (a - cy) * ccy
  td2    = one / (sqrt(ccy) * cscale * cy)
  tl1    = (two * a - three *cy - three * c3) / cprime
  tl2    = (b - three * c3 * cy ) / cprime
!--- corrected ty1 23.02.2011 
  ty1    = (-a + three*cy - chy - chy/cy*(c3 - two*gammas**2/sige**2) &
       + two*chy*(cx+chy)/cy + six*c2y) / cprime
!--- corrected ty2 23.02.2011 
  ty2    = (b - c1y*(c3+cy) + chy*(cy+chy) & 
       + chy*ey*(one/ey + betax/(betay*ex)) * gammas**2/sige**2 &
       - chy*betax/ex*four + (one+(betax*ey)/(betay*ex))*cx*chy & 
       + (chy**2)*(betax*ey)/(betay*ex) - chy*ey*c2*c3/betay &
       - c2y*(cy+c3+chy) + three*c3*(two*c2y+c1y)) & 
       / cprime - r1 / cscale

!--- corrected tx1 23.02.2011 
  tx1    = (two*(a-c3-cy)*(cx-c3) - cy*cx &
       + c3*(c1y + six*c2 + c2y + two*c3 + cl - cy)) / cprime
!--- corrected tx2 23.02.2011 
  tx2    = (c3 + cx) * ( (b - c1y * (c3 + cy)) / cprime ) &
       - six / cscale + three*c3*cy*(cl/cprime) &
       + ( six*c3*cy*c1y + (betay/ey + betax/ex)*chy*cx + &
       chy*(c3**2-two*cy*c3)-c2y*cx*(cy+c3)+(two*cy*c3-c3*c3)* c2y ) / cprime

  al(1)  = zero

  do iloop = 1, maxdec
     bl(iloop) = ten**iloop
     al(iloop+1) = bl(iloop)
     h = (bl(iloop) - al(iloop)) / ns
     aloop = al(iloop)

     !---- Evaluate Simpson's rule summation for one interval.
     !     The integrand is calculated in the loop itself
     if (abs(cy+aloop).gt.onetominus20) then
        term = sqrt((cy+aloop)*ccy) * & 
             sqrt((aloop*ccy*aloop+td1*aloop+td2) + aloop*c2y*(c3-cy)*ccy/(cy+aloop))
     else
        term = sqrt((cy+aloop)*ccy) * sqrt((aloop*ccy*aloop+td1*aloop+td2))
     endif
     func = sqrt(aloop) / term**3
     polyl = tl1 * aloop + tl2
     polyx = tx1 * aloop + tx2
     polyy = ty1 * aloop + ty2
     suml = func * polyl
     sumx = func * polyx
     sumy = func * polyy

     do iiz = 1, ns
        alam = aloop + iiz * h
        cof = coeff(mod(iiz,2)+1)
        if (abs(cy+alam).gt.onetominus20) then
           term = sqrt((cy+alam)*ccy) * &
                sqrt((alam*ccy*alam+td1*alam+td2)+alam*c2y*(c3-cy)*ccy/(cy+alam))
        else
           term = sqrt((cy+alam)*ccy)*sqrt((alam*ccy*alam+td1*alam+td2))
        endif
        f = sqrt(alam) / term**3
        polyl = tl1 * alam + tl2
        polyx = tx1 * alam + tx2
        polyy = ty1 * alam + ty2

        suml = suml + cof * f * polyl
        sumx = sumx + cof * f * polyx
        sumy = sumy + cof * f * polyy
     enddo

     suml = suml - f * polyl
     sumx = sumx - f * polyx
     sumy = sumy - f * polyy
     tmpl = (suml / three) * h
     tmpx = (sumx / three) * h
     tmpy = (sumy / three) * h
     zintl = zintl + tmpl
     zintx = zintx + tmpx
     zinty = zinty + tmpy

     !---- Test to see if integral has converged.
     if ( abs(tmpl/zintl) .lt. test .and. &
          abs(tmpx/zintx) .lt. test .and. &
          abs(tmpy/zinty) .lt. test) goto 100
  enddo

  write (*,*) tmpl, zintl, tmpx, zintx, tmpy, zinty, test

  write (*, '(a,i3,a)') "Bjorken/Mtingwa integrals did not converge in ",maxdec," decades."
  call fort_fail('TWSINT: ', 'Problem with TWSINT, program stopped ')

100 continue

  !---- Divide answers by cprime to account for scaling.
  txi    =      (zintx / cprime)
  tli    = cl * (zintl / cprime)
  tyi    = cy * (zinty / cprime)  
  
end subroutine twsint

