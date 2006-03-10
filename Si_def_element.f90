!The Polymorphic Tracking Code
!Copyright (C) Etienne Forest and Frank Schmidt
! See file A_SCRATCH_SIZE.F90

MODULE S_DEF_ELEMENT
  USE S_DEF_KIND
  USE USER_kind1
  USE USER_kind2
  USE sagan_WIGGLER

  IMPLICIT NONE
  public
  logical(lp),PARAMETER::BERZ=.TRUE.,ETIENNE=.NOT.BERZ
  logical(lp) :: USE_TPSAFIT=.TRUE.  ! USE GLOBAL ARRAY INSTEAD OF PERSONAL ARRAY
  logical(lp), target :: set_tpsafit=.false.
  real(dp) , target :: scale_tpsafit=1.0_dp
  real(dp), target :: tpsafit(lnv) !   used for fitting with tpsa in conjunction with pol_block
  PRIVATE copy_el_elp,copy_elp_el,copy_el_el
  PRIVATE cop_el_elp,cop_elp_el,cop_el_el
  private ZERO_EL,ZERO_ELP
  PRIVATE MAGPSTATE,MAGSTATE
  PRIVATE SETFAMILYR,SETFAMILYP
  PRIVATE ADDR_ANBN,ADDP_ANBN,bL_0,EL_BL,ELp_BL,COPY_BL,UNARYP_BL
  PRIVATE ELp_POL,bLPOL_0
  PRIVATE work_0,work_r,ELp_WORK,EL_WORK,WORK_EL,WORK_ELP,BL_EL,BL_ELP,unaryw_w
  PRIVATE ZERO_ANBN,ZERO_ANBN_R,ZERO_ANBN_P
  private null_EL,null_ELp
  logical(lp), PRIVATE :: VERBOSE = .FALSE.
  logical(lp), PRIVATE :: GEN = .TRUE.
  logical(lp),TARGET :: ALWAYS_EXACTMIS=.TRUE.
  logical(lp),TARGET :: FEED_P0C=.FALSE.
  integer, TARGET :: np_pol=0
  !  logical(lp) :: isomorphism_MIS=.TRUE.  !Not needed anymore always should be true
  private put_aperture_el,put_aperture_elp

  TYPE MUL_BLOCK
     ! stuff for setting multipole
     real(dp) AN(NMAX),BN(NMAX)
     INTEGER NMUL,NATURAL,ADD
  END TYPE MUL_BLOCK


  ! Old home for element and elementp, now in sh_def_kind






  INTERFACE EQUAL
     MODULE PROCEDURE copy_el_elp                              ! need upgrade
     MODULE PROCEDURE copy_elp_el                              ! need upgrade
     MODULE PROCEDURE copy_el_el                ! need upgrade
  end  INTERFACE

  INTERFACE COPY
     MODULE PROCEDURE cop_el_elp                              ! need upgrade
     MODULE PROCEDURE cop_elp_el                              ! need upgrade
     MODULE PROCEDURE cop_el_el                ! need upgrade
     MODULE PROCEDURE COPY_BL
  end  INTERFACE

  INTERFACE ADD
     MODULE PROCEDURE ADDR_ANBN
     MODULE PROCEDURE ADDP_ANBN
  end  INTERFACE

  INTERFACE ZERO_ANBN
     MODULE PROCEDURE ZERO_ANBN_R
     MODULE PROCEDURE ZERO_ANBN_P
  end  INTERFACE


  INTERFACE OPERATOR (+)
     MODULE PROCEDURE unaryP_BL
  END INTERFACE

  INTERFACE OPERATOR (+)
     MODULE PROCEDURE unaryw_w
  END INTERFACE


  INTERFACE SETFAMILY
     MODULE PROCEDURE SETFAMILYR                              ! need upgrade
     MODULE PROCEDURE SETFAMILYP                              ! need upgrade
  end  INTERFACE

  INTERFACE null_ELEment
     MODULE PROCEDURE null_EL                               ! need upgrade
     MODULE PROCEDURE null_ELp                              ! need upgrade
  end  INTERFACE

  INTERFACE put_aperture
     MODULE PROCEDURE put_aperture_el                               ! need upgrade
     MODULE PROCEDURE put_aperture_elp                              ! need upgrade
  end  INTERFACE


  INTERFACE ASSIGNMENT (=)
     MODULE PROCEDURE ZERO_EL                 ! NEED UPGRADE
     MODULE PROCEDURE ZERO_ELP                  ! NEED UPGRADE
     MODULE PROCEDURE MAGSTATE              ! need upgrade IF STATES EXPANDED
     MODULE PROCEDURE MAGPSTATE             ! need upgrade IF STATES EXPANDED
     ! Multipole block setting
     MODULE PROCEDURE BL_0
     MODULE PROCEDURE EL_BL
     MODULE PROCEDURE ELp_BL
     MODULE PROCEDURE BL_EL
     MODULE PROCEDURE BL_ELP
     ! polymorphism
     MODULE PROCEDURE bLPOL_0
     MODULE PROCEDURE ELp_POL
     ! energy/mass retrieving
     MODULE PROCEDURE work_0
     MODULE PROCEDURE work_r
     MODULE PROCEDURE ELp_WORK
     MODULE PROCEDURE EL_WORK
     MODULE PROCEDURE WORK_EL
     MODULE PROCEDURE WORK_ELP
  END INTERFACE





CONTAINS

  SUBROUTINE  work_0(S2,S1)
    implicit none
    type (work),INTENT(inOUT):: S2
    INTEGER,INTENT(IN):: S1

    S2%BETA0=one
    S2%energy=zero
    S2%kinetic=zero
    S2%p0c=zero
    S2%brho=zero
    S2%mass=zero
    S2%gamma0I=zero
    S2%gambet=zero
    if(s1/=0) then
       S2%rescale=.true.
       s2%power=s1
    else
       S2%rescale=.false.
       s2%power=0
    endif
  END SUBROUTINE work_0



  SUBROUTINE  work_r(S2,S1)
    implicit none
    type (work),INTENT(inOUT):: S2
    real(dp),INTENT(IN):: S1

    !    S2%energy=-(S2%energy+s1)
    !  VERBOSE = .FALSE.
    IF(FEED_P0C) THEN
       call find_energy(s2,P0C=S1+S2%P0C)
    ELSE
       call find_energy(s2,ENERGY=S1+S2%energy)
    ENDIF
    !  VERBOSE = .TRUE.
  END SUBROUTINE work_r


  function  unaryw_w(S1)
    implicit none
    type (WORK),INTENT(IN):: S1
    TYPE(WORK) unaryw_w
    unaryw_w=s1
    unaryw_w%rescale=.false.

  end   function  unaryw_w

  SUBROUTINE  ELp_WORK(S2,S1)
    implicit none
    type (WORK),INTENT(IN):: S1
    TYPE(ELEMENTP),INTENT(inOUT):: S2
    integer i

    if(s1%rescale) then
       if(s2%p%nmul/=0) then
          do i=1,s2%P%nmul
             s2%bn(i)=s2%bn(i)*(S2%P%P0C/S1%P0C)**S1%power
             s2%an(i)=s2%an(i)*(S2%P%P0C/S1%P0C)**S1%power
          enddo
          CALL ADD(s2,1,1,zero)
       endif
       if(associated(s2%B_sol))  s2%B_sol=s2%B_sol*(S2%P%P0C/S1%P0C)**S1%power

       if(s2%kind==kinduser1) call scale_user1(s2%u1,S2%P%P0C,S1%P0C,S1%power)
       if(s2%kind==kinduser2) call scale_user2(s2%u2,S2%P%P0C,S1%P0C,S1%power)
       if(s2%kind==KINDwiggler) call scale_sagan(s2%wi,S2%P%P0C,S1%P0C,S1%power)

    endif

    if(S1%power/=-1) then       ! just rescaling  -1=ramping
       S2%P%BETA0=S1%BETA0
       S2%P%GAMMA0I=S1%GAMMA0I
       S2%P%GAMBET=S1%GAMBET
       S2%P%P0C=S1%P0C
    endif

  END SUBROUTINE ELp_WORK

  SUBROUTINE  EL_WORK(S2,S1)
    implicit none
    type (WORK),INTENT(IN):: S1
    TYPE(ELEMENT),INTENT(inOUT):: S2
    integer i

    if(s1%rescale) then
       if(s2%p%nmul/=0) then
          do i=1,s2%P%nmul
             s2%bn(i)=s2%bn(i)*(S2%P%P0C/S1%P0C)**S1%power
             s2%an(i)=s2%an(i)*(S2%P%P0C/S1%P0C)**S1%power
          enddo
          CALL ADD(s2,1,1,zero)
       endif
       if(associated(s2%B_sol))  s2%B_sol=s2%B_sol*(S2%P%P0C/S1%P0C)**S1%power
       if(s2%kind==kinduser1) call scale_user1(s2%u1,S2%P%P0C,S1%P0C,S1%power)
       if(s2%kind==kinduser2) call scale_user2(s2%u2,S2%P%P0C,S1%P0C,S1%power)
       if(s2%kind==KINDwiggler) call scale_sagan(s2%wi,S2%P%P0C,S1%P0C,S1%power)
    endif


    if(S1%power/=-1) then       ! just rescaling  -1=ramping
       S2%P%BETA0=S1%BETA0
       S2%P%GAMMA0I=S1%GAMMA0I
       S2%P%GAMBET=S1%GAMBET
       S2%P%P0C=S1%P0C
    endif


  END SUBROUTINE EL_WORK


  SUBROUTINE  WORK_EL(S1,S2)
    implicit none
    type (WORK),INTENT(inOUT):: S1
    TYPE(ELEMENT),INTENT(IN):: S2

    S1=S1%POWER

    !    S1%P0C=-S2%P%P0C
    !  VERBOSE = .FALSE.
    call find_energy(s1,P0C=S2%P%P0C)
    !  VERBOSE = .TRUE.

  END SUBROUTINE WORK_EL

  SUBROUTINE  WORK_ELp(S1,S2)
    implicit none
    type (WORK),INTENT(inOUT):: S1
    TYPE(ELEMENTP),INTENT(IN):: S2

    S1=S1%POWER

    !    S1%P0C=-S2%P%P0C
    !  VERBOSE = .FALSE.
    call find_energy(s1,P0C=S2%P%P0C)
    !  VERBOSE = .TRUE.

  END SUBROUTINE WORK_ELp


  integer function mod_n(i,j)
    implicit none
    integer, intent(in) :: i,j
    integer k
    if(j<=0) then
       w_p=0
       w_p%nc=1
       w_p%fc='(1((1X,A72)))'
       write(w_p%c(1),'(A4,1X,I4)') "j = ",j
       CALL WRITE_E(812)
    endif
    k=i
    if(i<1) then
       do while(k<1)
          k=k+j
       enddo
    endif
    mod_n=mod(k,j)
    if(mod_n==0) mod_n=j
  end function  mod_n

  SUBROUTINE  bL_0(S2,S1)
    implicit none
    type (MUL_BLOCK),INTENT(OUT):: S2
    INTEGER,INTENT(IN):: S1
    INTEGER I

    IF(S1>=0.OR.S1<=nmax) THEN
       do i = 1,nmax
          s2%aN(i)=zero
          s2%bN(i)=zero
       enddo
       s2%natural=1
       s2%nmul=S1
       s2%ADD=0
    ELSEIF(S1>NMAX) THEN
       w_p=0
       w_p%nc=1
       w_p%fc='(1((1X,A72)))'
       write(w_p%c(1),'(A38,1X,I4)') " NMAX NOT BIG ENOUGH: PLEASE INCREASE ",NMAX
       CALL WRITE_E(100)
    ELSE
       w_p=0
       w_p%nc=1
       w_p%fc='(1((1X,A72)))'
       w_p%c(1) = " UNDEFINED  ASSIGNMENT IN BL_0"
       CALL WRITE_E(101)
    ENDIF

  END SUBROUTINE bL_0

  SUBROUTINE  bLPOL_0(S2,S1)
    implicit none
    type (POL_BLOCK),INTENT(OUT):: S2
    INTEGER,INTENT(IN):: S1
    INTEGER I

    !    IF(S1>=0.and.S1<=nmax) THEN
    do i = 1,nmax
       s2%SAN(i)=one
       s2%SBN(i)=one
       s2%IaN(i)=0
       s2%IbN(i)=0
    enddo
    S2%user1=0
    S2%user2=0
    S2%SAGAN=0
    S2%SVOLT=one
    S2%SFREQ=one
    S2%SPHAS=one
    S2%SB_SOL=one
    S2%IVOLT=0
    S2%IFREQ=0
    S2%IPHAS=0
    S2%IB_SOL=0
    s2%npara=S1
    !     s2%NMUL=0
    s2%NAME=' '
    s2%N_NAME=0
    s2%VORNAME=' '
    !    s2%CHECK_NMUL=.TRUE.
    nullify(s2%tpsafit);nullify(s2%set_tpsafit);
    IF(USE_TPSAFIT) then
       s2%tpsafit=>tpsafit
       s2%set_tpsafit=>set_tpsafit
    endif

    if(s1>0) then
       c_%npara_fpp=0   ! backwards compatible
    endif

  END SUBROUTINE bLPOL_0

  SUBROUTINE  BL_EL(S1,S2)
    implicit none
    type (MUL_BLOCK),INTENT(out):: S1
    TYPE(ELEMENT),INTENT(IN):: S2
    INTEGER I

    IF(S2%P%NMUL>NMAX) THEN
       w_p=0
       w_p%nc=1
       w_p%fc='(1((1X,A72)))'
       write(w_p%c(1),'(A21,1X,I4,1X,I4)')  " NMAX NOT BIG ENOUGH ", S2%P%NMUL,NMAX
       CALL WRITE_E(456)
    ENDIF
    S1=S2%P%NMUL

    DO I=1,S2%P%NMUL
       s1%AN(I)=s2%AN(I)
       s1%BN(I)=s2%BN(I)
    ENDDO

  END SUBROUTINE BL_EL

  SUBROUTINE  BL_ELP(S1,S2)
    implicit none
    type (MUL_BLOCK),INTENT(out):: S1
    TYPE(ELEMENTP),INTENT(IN):: S2
    INTEGER I

    IF(S2%P%NMUL>NMAX) THEN
       w_p=0
       w_p%nc=1
       w_p%fc='(1((1X,A72)))'
       write(w_p%c(1),'(A21,1X,I4,1X,I4)')  " NMAX NOT BIG ENOUGH ", S2%P%NMUL,NMAX
       CALL WRITE_E(456)
    ENDIF
    S1=S2%P%NMUL

    DO I=1,S2%P%NMUL
       s1%AN(I)=s2%AN(I)
       s1%BN(I)=s2%BN(I)
    ENDDO

  END SUBROUTINE BL_ELP

  SUBROUTINE  EL_BL(S2,S1)
    implicit none
    type (MUL_BLOCK),INTENT(IN):: S1
    TYPE(ELEMENT),INTENT(inOUT):: S2
    INTEGER I

    IF(S2%P%NMUL>NMAX) THEN
       w_p=0
       w_p%nc=1
       w_p%fc='(1((1X,A72)))'
       write(w_p%c(1),'(A21,1X,I4,1X,I4)')  " NMAX NOT BIG ENOUGH ", S2%P%NMUL,NMAX
       CALL WRITE_E(456)
    ENDIF
    IF(s1%nmul>s2%P%nmul) CALL ADD(s2,s1%nmul,1,zero)

    DO I=1,S2%P%NMUL
       s2%AN(I)=S1%ADD*s2%AN(I)+s1%AN(I)
       s2%BN(I)=S1%ADD*s2%BN(I)+s1%BN(I)
    ENDDO
    CALL ADD(s2,1,1,zero)

  END SUBROUTINE EL_BL

  SUBROUTINE  ELp_BL(S2,S1)
    implicit none
    type (MUL_BLOCK),INTENT(IN):: S1
    TYPE(ELEMENTP),INTENT(inOUT):: S2
    INTEGER I

    IF(S2%P%NMUL>NMAX) THEN
       w_p=0
       w_p%nc=1
       w_p%fc='(1((1X,A72)))'
       write(w_p%c(1),'(A21,1X,I4,1X,I4)')  " NMAX NOT BIG ENOUGH ", S2%P%NMUL,NMAX
       CALL WRITE_E(456)
    ENDIF

    IF(s1%nmul>s2%P%nmul) CALL ADD(s2,s1%nmul,1,zero)
    DO I=1,S2%P%NMUL
       s2%AN(I)=S1%ADD*s2%AN(I)+s1%AN(I)
       s2%BN(I)=S1%ADD*s2%BN(I)+s1%BN(I)
    ENDDO
    CALL ADD(s2,1,1,zero)


  END SUBROUTINE ELp_BL

  SUBROUTINE  ELp_POL(S2,S1)
    implicit none
    type (POL_BLOCK),INTENT(IN):: S1
    TYPE(ELEMENTP),INTENT(inOUT):: S2
    INTEGER I,S1NMUL
    logical(lp) DOIT,DONEIT                    !,checkname
    CHARACTER(nlp) S1NAME
    CHARACTER(vp)    S1VORNAME

    IF(S2%P%NMUL>NMAX) THEN
       w_p=0
       w_p%nc=1
       w_p%fc='(1((1X,A72)))'
       write(w_p%c(1),'(A21,1X,I4,1X,I4)')  " NMAX NOT BIG ENOUGH ", S2%P%NMUL,NMAX
       CALL WRITE_E(456)
    ENDIF

    S1NAME=S1%name
    S1VORNAME=S1%VORname
    CALL CONTEXT(S1name)
    CALL CONTEXT(S1vorname)
    CALL CONTEXT(S2%name)
    CALL CONTEXT(S2%vorname)

    DOIT=.TRUE.
    IF(S1NAME/=' ') THEN
       if(s1%n_name==0) then
          IF(S1NAME/=S2%NAME) DOIT=.FALSE.
       else
          IF(S1NAME(1:s1%n_name)/=S2%NAME(1:s1%n_name)) DOIT=.FALSE.
       endif
    ENDIF


    IF(S1VORNAME/=' ') THEN
       IF(S1VORNAME/=S2%VORNAME.or.S1NAME/=S2%NAME) DOIT=.FALSE.
    ENDIF


    IF(DOIT) THEN
       IF(.not.S1%SET_TPSAFIT) THEN
          if(s2%knob) then
             write(6,'(A45,A16)')" BE CAREFUL USING A POL_BLOCK ON SAME MAGNET ",S2%NAME
          ENDIF
       endif
       s2%knob=.TRUE.
       call ELp_POL_force(S2,S1)
    ENDIF


  END SUBROUTINE ELp_POL

  SUBROUTINE  ELp_POL_force(S2,S1)
    implicit none
    type (POL_BLOCK),INTENT(IN):: S1
    TYPE(ELEMENTP),INTENT(inOUT):: S2
    INTEGER I,S1NMUL
    logical(lp) DOIT,DONEIT                    !,checkname

    IF(S2%P%NMUL>NMAX) THEN
       w_p=0
       w_p%nc=1
       w_p%fc='(1((1X,A72)))'
       write(w_p%c(1),'(A21,1X,I4,1X,I4)')  " NMAX NOT BIG ENOUGH ", S2%P%NMUL,NMAX
       CALL WRITE_E(456)
    ENDIF


    DOIT=.TRUE.



    s2%knob=.TRUE.

    !       IF(S1%NPARA>=4.AND.S1%NPARA<=6) THEN
    DONEIT=.FALSE.

    !        IF(S1%CHECK_NMUL) THEN
    S1NMUL=0
    DO I=NMAX,1,-1
       IF(s1%IAN(I)/=0.OR.s1%IBN(I)/=0)  THEN
          S1NMUL=I
          if(s1%IAN(I)>c_%np_pol) c_%np_pol=s1%IAN(I)
          if(s1%IBN(I)>c_%np_pol) c_%np_pol=s1%IBN(I)
          GOTO 100
       ENDIF
    ENDDO
100 CONTINUE
    !          CALL SET_FALSE(S1%CHECK_NMUL)
    !        ENDIF

    IF(S1NMUL>S2%P%NMUL) then
       CALL ADD(S2,S1NMUL,1,zero)  !etienne
    endif
    DO I=1,S1NMUL
       IF(S1%IAN(I)>0) THEN
          s2%AN(I)%I=S1%IAN(I)+S1%NPARA
          s2%AN(I)%S=S1%SAN(I)
          s2%AN(I)%KIND=3
          DONEIT=.TRUE.
          IF(S1%SET_TPSAFIT) THEN
             s2%aN(I)%R=s2%aN(I)%R+scale_tpsafit*s2%AN(I)%S*s1%TPSAFIT(S1%IAN(I))
          ENDIF
       ENDIF
       IF(S1%IBN(I)>0) THEN
          s2%BN(I)%I=S1%IBN(I)+S1%NPARA
          s2%BN(I)%S=S1%SBN(I)
          s2%BN(I)%KIND=3
          DONEIT=.TRUE.
          IF(S1%SET_TPSAFIT) THEN
             s2%BN(I)%R=s2%BN(I)%R+scale_tpsafit*s2%BN(I)%S*s1%TPSAFIT(S1%IBN(I))
          ENDIF
       ENDIF
    ENDDO
    IF(DONEIT.AND.S1%SET_TPSAFIT) THEN
       CALL ADD(S2,1,1,zero)     !etienne
    ENDIF
    IF(S2%KIND==KIND4) THEN    ! CAVITY
       DONEIT=.FALSE.                     ! NOT USED HERE
       IF(S1%IVOLT>0) THEN
          s2%VOLT%I=S1%IVOLT+S1%NPARA
          s2%VOLT%S=S1%SVOLT
          s2%VOLT%KIND=3
          DONEIT=.TRUE.
          if(S1%IVOLT>c_%np_pol) c_%np_pol=S1%IVOLT
          IF(S1%SET_TPSAFIT) THEN
             s2%VOLT%R=s2%VOLT%R+scale_tpsafit*s2%VOLT%S*s1%TPSAFIT(S1%IVOLT)
          ENDIF
       ENDIF
       IF(S1%IFREQ>0) THEN
          s2%FREQ%I=S1%IFREQ+S1%NPARA
          s2%FREQ%S=S1%SFREQ
          s2%FREQ%KIND=3
          if(S1%IFREQ>c_%np_pol) c_%np_pol=S1%IFREQ
          IF(S1%SET_TPSAFIT) THEN
             s2%FREQ%R=s2%FREQ%R+scale_tpsafit*s2%FREQ%S*s1%TPSAFIT(S1%IFREQ)
          ENDIF
          DONEIT=.TRUE.
       ENDIF
       IF(S1%IPHAS>0) THEN
          s2%PHAS%I=S1%IPHAS+S1%NPARA
          s2%PHAS%S=S1%SPHAS
          s2%PHAS%KIND=3
          DONEIT=.TRUE.
          if(S1%IPHAS>c_%np_pol) c_%np_pol=S1%IPHAS
          IF(S1%SET_TPSAFIT) THEN
             s2%PHAS%R=s2%PHAS%R+scale_tpsafit*s2%PHAS%S*s1%TPSAFIT(S1%IPHAS)
          ENDIF
       ENDIF
    ENDIF
    IF(S2%KIND==KIND21) THEN    ! CAVITY
       DONEIT=.FALSE.                     ! NOT USED HERE
       IF(S1%IVOLT>0) THEN
          s2%VOLT%I=S1%IVOLT+S1%NPARA
          s2%VOLT%S=S1%SVOLT
          s2%VOLT%KIND=3
          if(S1%IVOLT>c_%np_pol) c_%np_pol=S1%IVOLT
          DONEIT=.TRUE.
          IF(S1%SET_TPSAFIT) THEN
             s2%VOLT%R=s2%VOLT%R+scale_tpsafit*s2%VOLT%S*s1%TPSAFIT(S1%IVOLT)
          ENDIF
       ENDIF
       IF(S1%IFREQ>0) THEN
          s2%FREQ%I=S1%IFREQ+S1%NPARA
          s2%FREQ%S=S1%SFREQ
          s2%FREQ%KIND=3
          if(S1%IFREQ>c_%np_pol) c_%np_pol=S1%IFREQ
          IF(S1%SET_TPSAFIT) THEN
             s2%FREQ%R=s2%FREQ%R+scale_tpsafit*s2%FREQ%S*s1%TPSAFIT(S1%IFREQ)
          ENDIF
          DONEIT=.TRUE.
       ENDIF
       IF(S1%IPHAS>0) THEN
          s2%PHAS%I=S1%IPHAS+S1%NPARA
          s2%PHAS%S=S1%SPHAS
          s2%PHAS%KIND=3
          if(S1%IPHAS>c_%np_pol) c_%np_pol=S1%IPHAS
          DONEIT=.TRUE.
          IF(S1%SET_TPSAFIT) THEN
             s2%PHAS%R=s2%PHAS%R+scale_tpsafit*s2%PHAS%S*s1%TPSAFIT(S1%IPHAS)
          ENDIF
       ENDIF
    ENDIF
    IF(S2%KIND==KIND5) THEN    ! SOLENOID
       DONEIT=.FALSE.
       IF(S1%IB_SOL>0) THEN
          s2%B_SOL%I=S1%IB_SOL+S1%NPARA
          s2%B_SOL%S=S1%SB_SOL
          s2%B_SOL%KIND=3
          DONEIT=.TRUE.
          if(S1%IB_SOL>c_%np_pol) c_%np_pol=S1%IB_SOL
          IF(S1%SET_TPSAFIT) THEN
             s2%B_SOL%R=s2%B_SOL%R+scale_tpsafit*s2%B_SOL%S*s1%TPSAFIT(S1%IB_SOL)
          ENDIF
       ENDIF
    ENDIF
    IF(S2%KIND==kinduser1) THEN    ! new element
       DONEIT=.FALSE.                     ! NOT USED HERE
       call ELp_POL_user1(S2%u1,S1,DONEIT)
    ENDIF
    IF(S2%KIND==kinduser2) THEN    ! new element
       DONEIT=.FALSE.                     ! NOT USED HERE
       call ELp_POL_user2(S2%u2,S1,DONEIT)
    ENDIF
    IF(S2%KIND==KINDWIGGLER) THEN    ! new element
       DONEIT=.FALSE.                     ! NOT USED HERE
       call ELp_POL_SAGAN(S2%WI,S1,DONEIT)
    ENDIF



  END SUBROUTINE ELp_POL_force




  SUBROUTINE  COPY_BL(S1,S2)
    implicit none
    type (MUL_BLOCK),INTENT(IN):: S1
    TYPE(MUL_BLOCK),INTENT(OUT):: S2
    INTEGER I

    DO I=1,NMAX
       s2%AN(I)=s1%AN(I)
       s2%BN(I)=S1%BN(I)
    ENDDO

    S2%NMUL     =S1%NMUL
    S2%ADD      =S1%ADD
    S2%NATURAL  =S1%NATURAL

  END SUBROUTINE COPY_BL


  FUNCTION  UNARYP_BL(S1)
    implicit none
    type (MUL_BLOCK),INTENT(IN):: S1
    type (MUL_BLOCK) UNARYP_BL

    CALL COPY(S1,UNARYP_BL)
    UNARYP_BL%ADD=1

  END FUNCTION UNARYP_BL




  SUBROUTINE SETFAMILYR(EL,T,t_ax,t_ay,NTOT,ntot_rad,NTOT_REV,ntot_rad_REV,ND2)
    IMPLICIT NONE
    TYPE(ELEMENT), INTENT(INOUT) ::EL
    INTEGER,OPTIONAL :: NTOT,ntot_rad,NTOT_REV,ntot_rad_REV,ND2
    type(tree_element),OPTIONAL :: T(:),t_ax(:),t_ay(:)

    SELECT CASE(EL%KIND)
    CASE(KIND1)
       if(.not.ASSOCIATED(EL%D0))ALLOCATE(EL%D0)
       EL%D0%P=>EL%P
       EL%D0%L=>EL%L
    CASE(KIND2)
       IF(EL%P%EXACT) THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)=" ERROR IN SETFAMILYR "
          write(w_p%c(2),'(A37,1x,I4)') " EXACT OPTION NOT SUPPORTED FOR KIND ", EL%KIND
          CALL WRITE_E(222)
       ENDIF
       if(.not.ASSOCIATED(EL%K2))ALLOCATE(EL%K2)
       EL%K2%P=>EL%P
       EL%K2%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%K2%AN=>EL%AN
       EL%K2%BN=>EL%BN
       EL%K2%FINT=>EL%FINT
       EL%K2%HGAP=>EL%HGAP
       EL%K2%H1=>EL%H1
       EL%K2%H2=>EL%H2
    CASE(KIND3)
       if(.not.ASSOCIATED(EL%K3)) THEN
          ALLOCATE(EL%K3)
          el%K3=0
       ELSE
          el%K3=-1
          el%K3=0
       ENDIF
       EL%K3%P=>EL%P
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%K3%AN=>EL%AN
       EL%K3%BN=>EL%BN
       ALLOCATE(EL%K3%thin_h_foc);EL%K3%thin_h_foc=0
       ALLOCATE(EL%K3%thin_v_foc);EL%K3%thin_v_foc=0
       ALLOCATE(EL%K3%thin_h_angle);EL%K3%thin_h_angle=0
       ALLOCATE(EL%K3%thin_v_angle);EL%K3%thin_v_angle=0
    CASE(KIND4)
       if(.not.ASSOCIATED(EL%C4)) THEN
          ALLOCATE(EL%C4)
          el%C4=0
       ELSE
          el%C4=-1
          el%C4=0
       ENDIF
       EL%C4%P=>EL%P
       EL%C4%L=>EL%L
       EL%C4%VOLT=>EL%VOLT
       EL%C4%FREQ=>EL%FREQ
       EL%C4%PHAS=>EL%PHAS
       !       EL%C4%P0C=>EL%P0C
       EL%C4%DELTA_E=>EL%DELTA_E
       EL%C4%THIN=>EL%THIN
       ALLOCATE(EL%C4%N_BESSEL);EL%C4%N_BESSEL=0
    CASE(KIND21)
       if(.not.ASSOCIATED(EL%CAV21)) THEN
          ALLOCATE(EL%CAV21)
          el%CAV21=0
       ELSE
          el%CAV21=-1
          el%CAV21=0
       ENDIF
       EL%CAV21%P=>EL%P
       EL%CAV21%L=>EL%L
       EL%CAV21%VOLT=>EL%VOLT
       EL%CAV21%FREQ=>EL%FREQ
       EL%CAV21%PHAS=>EL%PHAS
       !       EL%C4%P0C=>EL%P0C
       EL%CAV21%DELTA_E=>EL%DELTA_E
       EL%CAV21%THIN=>EL%THIN
       ALLOCATE(EL%CAV21%PSI);EL%CAV21%PSI=ZERO
       ALLOCATE(EL%CAV21%DPHAS);EL%CAV21%DPHAS=ZERO
    CASE(KIND5)
       if(.not.ASSOCIATED(EL%S5))ALLOCATE(EL%S5)
       EL%S5%P=>EL%P
       EL%S5%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%S5%AN=>EL%AN
       EL%S5%BN=>EL%BN
       EL%S5%FINT=>EL%FINT      ! added may 31st 2004
       EL%S5%HGAP=>EL%HGAP
       EL%S5%H1=>EL%H1
       EL%S5%H2=>EL%H2
       EL%S5%B_SOL=>EL%B_SOL
    CASE(KIND6)
       IF(EL%P%EXACT.AND.EL%P%B0/=zero) THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)=" ERROR IN SETFAMILYR "
          write(w_p%c(2),'(A37,1x,I4)') " EXACT OPTION NOT SUPPORTED FOR KIND ", EL%KIND
          CALL WRITE_E(777)
       ENDIF
       if(.not.ASSOCIATED(EL%T6)) THEN
          ALLOCATE(EL%T6)
          el%T6=0
       ELSE
          el%T6=-1
          el%T6=0
       ENDIF
       EL%T6%P=>EL%P
       EL%T6%L=>EL%L
       IF(EL%P%NMUL==0)       THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)= " ERROR IN SETFAMILYR "
          w_p%c(2)= " ERROR ON T6: SLOW THICK "
          call write_e(0)
       ENDIF
       EL%T6%AN=>EL%AN
       EL%T6%BN=>EL%BN
       EL%T6%FINT=>EL%FINT
       EL%T6%HGAP=>EL%HGAP
       EL%T6%H1=>EL%H1
       EL%T6%H2=>EL%H2
       nullify(EL%T6%MATX);ALLOCATE(EL%T6%MATX(2,3));
       nullify(EL%T6%MATY);ALLOCATE(EL%T6%MATY(2,3));
       nullify(EL%T6%LX);ALLOCATE(EL%T6%LX(6));
       nullify(EL%T6%LY);ALLOCATE(EL%T6%LY(3));
    CASE(KIND7)
       IF(EL%P%EXACT.AND.EL%P%B0/=zero) THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)=" ERROR IN SETFAMILYR "
          write(w_p%c(2),'(A37,1x,I4)') " EXACT OPTION NOT SUPPORTED FOR KIND ", EL%KIND
          CALL WRITE_E(777)
       ENDIF
       !       if(.not.ASSOCIATED(EL%T7))ALLOCATE(EL%T7)
       if(.not.ASSOCIATED(EL%T7)) THEN
          ALLOCATE(EL%T7)
          EL%T7=0
       ELSE
          EL%T7=-1
          EL%T7=0
       ENDIF
       EL%T7%P=>EL%P
       EL%T7%L=>EL%L
       IF(EL%P%NMUL==0)       THEN
          w_p=0
          w_p%nc=1
          w_p%fc='((1X,A72))'
          w_p%c(1)= "ERROR ON T7: FAST THICK "
          call write_e(0)
       ENDIF
       EL%T7%AN=>EL%AN
       EL%T7%BN=>EL%BN
       EL%T7%FINT=>EL%FINT
       EL%T7%HGAP=>EL%HGAP
       EL%T7%H1=>EL%H1
       EL%T7%H2=>EL%H2
       nullify(EL%T7%MATX);ALLOCATE(EL%T7%MATX(2,3));
       nullify(EL%T7%MATY);ALLOCATE(EL%T7%MATY(2,3));
       nullify(EL%T7%LX);ALLOCATE(EL%T7%LX(3));
       nullify(EL%T7%RMATX);ALLOCATE(EL%T7%RMATX(2,3));
       nullify(EL%T7%RMATY);ALLOCATE(EL%T7%RMATY(2,3));
       nullify(EL%T7%RLX);ALLOCATE(EL%T7%RLX(3));
       IF(GEN) call GETMAT7(EL%T7)
    CASE(KIND8)
       if(.not.ASSOCIATED(EL%S8))ALLOCATE(EL%S8)
       EL%S8%P=>EL%P
       IF(EL%P%NMUL==0)       THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)= " ERROR IN SETFAMILYR "
          w_p%c(2)= "ERROR ON S8:  NORMAL SMI "
          CALL WRITE_E(0)
       ENDIF
       EL%S8%BN=>EL%BN
    CASE(KIND9)
       if(.not.ASSOCIATED(EL%S9))ALLOCATE(EL%S9)
       EL%S9%P=>EL%P
       IF(EL%P%NMUL==0)       THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)= " ERROR IN SETFAMILYR "
          w_p%c(2)= "ERROR ON S9: SKEW SMI "
          CALL WRITE_E(0)
       ENDIF
       EL%S9%AN=>EL%AN
    CASE(KIND10)
       IF(.not.EL%P%EXACT) THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)=" ERROR IN SETFAMILYR "
          write(w_p%c(2),'(A37,1x,I4)') " EXACT OPTION NOT SUPPORTED FOR KIND ", EL%KIND
          CALL WRITE_E(777)
       ENDIF
       if(.not.ASSOCIATED(EL%TP10)) THEN
          ALLOCATE(EL%TP10)
          EL%TP10=0
       ELSE
          EL%TP10=-1
          EL%TP10=0
       ENDIF
       EL%TP10%P=>EL%P
       EL%TP10%L=>EL%L
       IF(EL%P%NMUL==0.OR.EL%P%NMUL>SECTOR_NMUL_MAX)       THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)= " ERROR IN SETFAMILYR "
          w_p%c(2)= "ERROR ON TP10: TEAPOT "
          CALL WRITE_E(0)
       ENDIF
       EL%TP10%AN=>EL%AN
       EL%TP10%BN=>EL%BN
       EL%TP10%FINT=>EL%FINT
       EL%TP10%HGAP=>EL%HGAP
       EL%TP10%H1=>EL%H1
       EL%TP10%H2=>EL%H2
       NULLIFY(EL%TP10%BF_X);ALLOCATE(EL%TP10%BF_X(S_B(EL%P%NMUL)%N_MONO))
       NULLIFY(EL%TP10%BF_Y);ALLOCATE(EL%TP10%BF_Y(S_B(EL%P%NMUL)%N_MONO))
       NULLIFY(EL%TP10%DRIFTKICK);ALLOCATE(EL%TP10%DRIFTKICK);EL%TP10%DRIFTKICK=.true.;
       call GETANBN(EL%TP10)
    CASE(KIND11:KIND14)
       if(.not.ASSOCIATED(EL%MON14)) THEN
          ALLOCATE(EL%MON14)
          el%MON14=0
       ELSE
          el%MON14=-1
          el%MON14=0
       ENDIF
       EL%MON14%P=>EL%P
       EL%MON14%L=>EL%L
       nullify(EL%MON14%X);ALLOCATE(EL%MON14%X);EL%MON14%X=zero;
       nullify(EL%MON14%Y);ALLOCATE(EL%MON14%Y);EL%MON14%Y=zero
    CASE(KIND15)
       if(.not.ASSOCIATED(EL%SEP15))ALLOCATE(EL%SEP15)
       EL%SEP15%P=>EL%P
       EL%SEP15%L=>EL%L
       EL%SEP15%VOLT=>EL%VOLT
       EL%SEP15%PHAS=>EL%PHAS
    CASE(KIND16,KIND20)
       if(.not.ASSOCIATED(EL%K16)) THEN
          ALLOCATE(EL%K16)
          el%K16=0
       ELSE
          el%K16=-1
          el%K16=0
       ENDIF
       EL%K16%P=>EL%P
       EL%K16%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%K16%AN=>EL%AN
       EL%K16%BN=>EL%BN
       EL%K16%FINT=>EL%FINT
       EL%K16%HGAP=>EL%HGAP
       EL%K16%H1=>EL%H1
       EL%K16%H2=>EL%H2
       NULLIFY(EL%K16%DRIFTKICK);ALLOCATE(EL%K16%DRIFTKICK);EL%K16%DRIFTKICK=.true.;
       NULLIFY(EL%K16%LIKEMAD);ALLOCATE(EL%K16%LIKEMAD);EL%K16%LIKEMAD=.false.;
    CASE(KIND17)
       call SET_IN
       IF(EL%P%EXACT.AND.EL%P%B0/=zero) THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)=" ERROR IN SETFAMILYR "
          write(w_p%c(2),'(A42,1x,I4)') " EXACT BEND OPTION NOT SUPPORTED FOR KIND ", EL%KIND
          CALL WRITE_E(777)
       ENDIF
       if(.not.ASSOCIATED(EL%S17)) THEN
          ALLOCATE(EL%S17)
          el%S17=0
       ELSE
          el%S17=-1
          el%S17=0
       ENDIF
       EL%S17%P=>EL%P
       EL%S17%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,2)
       EL%S17%AN=>EL%AN
       EL%S17%BN=>EL%BN
       EL%S17%FINT=>EL%FINT
       EL%S17%HGAP=>EL%HGAP
       EL%S17%H1=>EL%H1
       EL%S17%H2=>EL%H2
       EL%S17%B_SOL=>EL%B_SOL
       nullify(EL%S17%MAT);ALLOCATE(EL%S17%MAT(4,4));
       nullify(EL%S17%LXY);ALLOCATE(EL%S17%LXY(0:10));
    CASE(KIND18)
       if(.not.ASSOCIATED(EL%RCOL18)) THEN
          ALLOCATE(EL%RCOL18)
          EL%RCOL18=0
       ELSE
          EL%RCOL18=-1
          EL%RCOL18=0
       ENDIF
       EL%RCOL18%P=>EL%P
       EL%RCOL18%L=>EL%L
       nullify(EL%RCOL18%A);ALLOCATE(EL%RCOL18%A);CALL ALLOC(EL%RCOL18%A)
    CASE(KIND19)
       if(.not.ASSOCIATED(EL%ECOL19)) THEN
          ALLOCATE(EL%ECOL19)
          EL%ECOL19=0
       ELSE
          EL%ECOL19=-1
          EL%ECOL19=0
       ENDIF
       EL%ECOL19%P=>EL%P
       EL%ECOL19%L=>EL%L
       nullify(EL%ECOL19%A);ALLOCATE(EL%ECOL19%A);CALL ALLOC(EL%ECOL19%A)
    CASE(KIND22)
       if(.not.ASSOCIATED(EL%M22)) THEN
          ALLOCATE(EL%M22)
          el%M22=0
       ELSE
          el%M22=-1
          el%M22=0
       ENDIF
       EL%M22%P=>EL%P
       !       CALL NULL_TREE(EL%M22%T)
       allocate(EL%M22%DELTAMAP)
       EL%M22%DELTAMAP=.TRUE.
       if(NTOT/=0) then
          allocate(EL%M22%T)
          CALL ALLOC_TREE(EL%M22%T,NTOT,ND2)
       endif
       if(NTOT_rad/=0) then
          allocate(EL%M22%T_rad)
          CALL ALLOC_TREE(EL%M22%T_rad,NTOT_rad,ND2)
       endif
       if(NTOT_REV/=0) then
          allocate(EL%M22%T_REV)
          CALL ALLOC_TREE(EL%M22%T_REV,NTOT_REV,ND2)
       endif
       if(NTOT_rad_REV/=0) then
          allocate(EL%M22%T_rad_REV)
          CALL ALLOC_TREE(EL%M22%T_rad_REV,NTOT_rad_REV,ND2)
       endif
    CASE(KINDUSER1)
       if(.not.ASSOCIATED(EL%U1)) THEN
          ALLOCATE(EL%U1)
          EL%U1=0
       ELSE
          EL%U1=-1
          EL%U1=0
       ENDIF
       EL%U1%P=>EL%P
       EL%U1%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%U1%AN=>EL%AN
       EL%U1%BN=>EL%BN
       CALL POINTERS_USER1(EL%U1)
    CASE(KINDUSER2)
       if(.not.ASSOCIATED(EL%U2)) THEN
          ALLOCATE(EL%U2)
          EL%U2=0
       ELSE
          EL%U2=-1
          EL%U2=0
       ENDIF
       EL%U2%P=>EL%P
       EL%U2%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%U2%AN=>EL%AN
       EL%U2%BN=>EL%BN
       CALL POINTERS_USER2(EL%U2)
    CASE(KINDWIGGLER)
       if(.not.ASSOCIATED(EL%WI)) THEN
          ALLOCATE(EL%WI)
          EL%WI=0
       ELSE
          EL%WI=-1
          EL%WI=0
       ENDIF
       EL%WI%P=>EL%P
       EL%WI%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%WI%AN=>EL%AN
       EL%WI%BN=>EL%BN
       CALL POINTERS_SAGAN(EL%WI)
    CASE(KINDpa)
       if(.not.ASSOCIATED(EL%pa)) THEN
          ALLOCATE(EL%pa)
          EL%PA=0
       ELSE
          EL%pa=-1
          EL%pa=0
       ENDIF
       EL%pa%P=>EL%P
       EL%pa%L=>EL%L
       !       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       !       EL%mu%AN=>EL%AN
       !       EL%mu%BN=>EL%BN
       CALL POINTERS_pancake(EL%pa,T,t_ax,t_ay)
    END SELECT
  END SUBROUTINE SETFAMILYR


  SUBROUTINE SETFAMILYP(EL,T,t_ax,t_ay,NTOT,ntot_rad,NTOT_REV,ntot_rad_REV,ND2)
    IMPLICIT NONE
    TYPE(ELEMENTP), INTENT(INOUT) ::EL
    INTEGER,OPTIONAL :: NTOT,ntot_rad,NTOT_REV,ntot_rad_REV,ND2
    type(tree_element),OPTIONAL :: T(:),t_ax(:),t_ay(:)

    SELECT CASE(EL%KIND)
    CASE(KIND1)
       if(.not.ASSOCIATED(EL%D0))ALLOCATE(EL%D0)
       EL%D0%P=>EL%P
       EL%D0%L=>EL%L
    CASE(KIND2)
       IF(EL%P%EXACT) THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)=" ERROR IN SETFAMILYP "
          write(w_p%c(2),'(A37,1x,I4)') " EXACT OPTION NOT SUPPORTED FOR KIND ", EL%KIND
          CALL WRITE_E(222)
       ENDIF
       if(.not.ASSOCIATED(EL%K2))ALLOCATE(EL%K2)
       EL%K2%P=>EL%P
       EL%K2%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%K2%AN=>EL%AN
       EL%K2%BN=>EL%BN
       EL%K2%FINT=>EL%FINT
       EL%K2%HGAP=>EL%HGAP
       EL%K2%H1=>EL%H1
       EL%K2%H2=>EL%H2
    CASE(KIND3)
       if(.not.ASSOCIATED(EL%K3)) THEN
          ALLOCATE(EL%K3)
          el%K3=0
       ELSE
          el%K3=-1
          el%K3=0
       ENDIF
       EL%K3%P=>EL%P
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%K3%AN=>EL%AN
       EL%K3%BN=>EL%BN
       ALLOCATE(EL%K3%thin_h_foc);CALL ALLOC(EL%K3%thin_h_foc);EL%K3%thin_h_foc=ZERO
       ALLOCATE(EL%K3%thin_v_foc);CALL ALLOC(EL%K3%thin_v_foc);EL%K3%thin_v_foc=ZERO
       ALLOCATE(EL%K3%thin_h_angle);CALL ALLOC(EL%K3%thin_h_angle);EL%K3%thin_h_angle=ZERO
       ALLOCATE(EL%K3%thin_v_angle);CALL ALLOC(EL%K3%thin_v_angle);EL%K3%thin_v_angle=ZERO
    CASE(KIND4)
       if(.not.ASSOCIATED(EL%C4)) THEN
          ALLOCATE(EL%C4)
          el%C4=0
       ELSE
          el%C4=-1
          el%C4=0
       ENDIF
       EL%C4%P=>EL%P
       EL%C4%L=>EL%L
       EL%C4%VOLT=>EL%VOLT
       EL%C4%FREQ=>EL%FREQ
       EL%C4%PHAS=>EL%PHAS
       !       EL%C4%P0C=>EL%P0C
       EL%C4%DELTA_E=>EL%DELTA_E
       EL%C4%THIN=>EL%THIN
       ALLOCATE(EL%C4%N_BESSEL);EL%C4%N_BESSEL=0
    CASE(KIND21)
       if(.not.ASSOCIATED(EL%CAV21)) THEN
          ALLOCATE(EL%CAV21)
          el%CAV21=0
       ELSE
          el%CAV21=-1
          el%CAV21=0
       ENDIF
       EL%CAV21%P=>EL%P
       EL%CAV21%L=>EL%L
       EL%CAV21%VOLT=>EL%VOLT
       EL%CAV21%FREQ=>EL%FREQ
       EL%CAV21%PHAS=>EL%PHAS
       !       EL%C4%P0C=>EL%P0C
       EL%CAV21%DELTA_E=>EL%DELTA_E
       EL%CAV21%THIN=>EL%THIN
       ALLOCATE(EL%CAV21%PSI);CALL ALLOC(EL%CAV21%PSI);EL%CAV21%PSI=ZERO
       ALLOCATE(EL%CAV21%DPHAS);CALL ALLOC(EL%CAV21%DPHAS);EL%CAV21%DPHAS=ZERO
    CASE(KIND5)
       if(.not.ASSOCIATED(EL%S5))ALLOCATE(EL%S5)
       EL%S5%P=>EL%P
       EL%S5%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%S5%AN=>EL%AN
       EL%S5%BN=>EL%BN
       EL%S5%FINT=>EL%FINT      ! added may 31st 2004
       EL%S5%HGAP=>EL%HGAP
       EL%S5%H1=>EL%H1
       EL%S5%H2=>EL%H2
       EL%S5%B_SOL=>EL%B_SOL
    CASE(KIND6)
       IF(EL%P%EXACT.AND.EL%P%B0/=zero) THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)=" ERROR IN SETFAMILYP "
          write(w_p%c(2),'(A37,1x,I4)') " EXACT OPTION NOT SUPPORTED FOR KIND ", EL%KIND
          CALL WRITE_E(777)
       ENDIF
       if(.not.ASSOCIATED(EL%T6)) THEN
          ALLOCATE(EL%T6)
          el%T6=0
       ELSE
          el%T6=-1
          el%T6=0
       ENDIF
       EL%T6%P=>EL%P
       EL%T6%L=>EL%L
       IF(EL%P%NMUL==0)       THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)= " ERROR IN SETFAMILYP "
          w_p%c(2)= "ERROR ON T6: SLOW THICK "
          CALL WRITE_E(0)
       ENDIF
       EL%T6%AN=>EL%AN
       EL%T6%BN=>EL%BN
       EL%T6%FINT=>EL%FINT
       EL%T6%HGAP=>EL%HGAP
       EL%T6%H1=>EL%H1
       EL%T6%H2=>EL%H2
       nullify(EL%T6%MATX);ALLOCATE(EL%T6%MATX(2,3));
       nullify(EL%T6%MATY);ALLOCATE(EL%T6%MATY(2,3));
       nullify(EL%T6%LX);ALLOCATE(EL%T6%LX(6));
       nullify(EL%T6%LY);ALLOCATE(EL%T6%LY(3));
    CASE(KIND7)
       IF(EL%P%EXACT.AND.EL%P%B0/=zero) THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)=" ERROR IN SETFAMILYP "
          write(w_p%c(2),'(A37,1x,I4)') " EXACT OPTION NOT SUPPORTED FOR KIND ", EL%KIND
          CALL WRITE_E(777)
       ENDIF
       if(.not.ASSOCIATED(EL%T7)) THEN
          ALLOCATE(EL%T7)
          EL%T7=0
       ELSE
          EL%T7=-1
          EL%T7=0
       ENDIF
       EL%T7%P=>EL%P
       EL%T7%L=>EL%L
       IF(EL%P%NMUL==0)       THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)= " ERROR IN SETFAMILYP "
          w_p%c(2)= "ERROR ON T7: FAST THICK "
          CALL WRITE_E(0)
       ENDIF
       EL%T7%AN=>EL%AN
       EL%T7%BN=>EL%BN
       EL%T7%FINT=>EL%FINT
       EL%T7%HGAP=>EL%HGAP
       EL%T7%H1=>EL%H1
       EL%T7%H2=>EL%H2
       nullify(EL%T7%MATX);  ALLOCATE(EL%T7%MATX(2,3));
       nullify(EL%T7%MATY);  ALLOCATE(EL%T7%MATY(2,3));
       nullify(EL%T7%LX);    ALLOCATE(EL%T7%LX(3));
       nullify(EL%T7%RMATX); ALLOCATE(EL%T7%RMATX(2,3));
       nullify(EL%T7%RMATY); ALLOCATE(EL%T7%RMATY(2,3));
       nullify(EL%T7%RLX);   ALLOCATE(EL%T7%RLX(3));
       CALL ALLOC(EL%T7)
       IF(GEN) call GETMAT7(EL%T7)
    CASE(KIND8)
       if(.not.ASSOCIATED(EL%S8))ALLOCATE(EL%S8)
       EL%S8%P=>EL%P
       IF(EL%P%NMUL==0)       THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)= " ERROR IN SETFAMILYP "
          w_p%c(2)= "ERROR ON S8:  NORMAL SMI "
          CALL WRITE_E(0)
       ENDIF
       EL%S8%BN=>EL%BN
    CASE(KIND9)
       if(.not.ASSOCIATED(EL%S9))ALLOCATE(EL%S9)
       EL%S9%P=>EL%P
       IF(EL%P%NMUL==0)       THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)= " ERROR IN SETFAMILYP "
          w_p%c(2)= "ERROR ON S9: SKEW SMI "
          CALL WRITE_E(0)
       ENDIF
       EL%S9%AN=>EL%AN
    CASE(KIND10)
       IF(.not.EL%P%EXACT) THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)=" ERROR IN SETFAMILYP "
          write(w_p%c(2),'(A37,1x,I4)') " EXACT OPTION NOT SUPPORTED FOR KIND ", EL%KIND
          CALL WRITE_E(777)
       ENDIF
       if(.not.ASSOCIATED(EL%TP10)) THEN
          ALLOCATE(EL%TP10)
          EL%TP10=0
       ELSE
          EL%TP10=-1
          EL%TP10=0
       ENDIF
       EL%TP10%P=>EL%P
       EL%TP10%L=>EL%L
       IF(EL%P%NMUL==0.OR.EL%P%NMUL>SECTOR_NMUL_MAX)       THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)= " ERROR IN SETFAMILYP "
          w_p%c(2)= "ERROR ON TP10: TEAPOT "
          CALL WRITE_E(0)
       ENDIF
       EL%TP10%AN=>EL%AN
       EL%TP10%BN=>EL%BN
       EL%TP10%FINT=>EL%FINT
       EL%TP10%HGAP=>EL%HGAP
       EL%TP10%H1=>EL%H1
       EL%TP10%H2=>EL%H2
       NULLIFY(EL%TP10%BF_X);ALLOCATE(EL%TP10%BF_X(S_B(EL%P%NMUL)%N_MONO))
       NULLIFY(EL%TP10%BF_Y);ALLOCATE(EL%TP10%BF_Y(S_B(EL%P%NMUL)%N_MONO))
       NULLIFY(EL%TP10%DRIFTKICK);ALLOCATE(EL%TP10%DRIFTKICK);EL%TP10%DRIFTKICK=.true.;
       CALL ALLOC(EL%TP10)
       call GETANBN(EL%TP10)
    CASE(KIND11:KIND14)
       if(.not.ASSOCIATED(EL%MON14)) THEN
          ALLOCATE(EL%MON14)
          el%MON14=0
       ELSE
          el%MON14=-1
          el%MON14=0
       ENDIF
       EL%MON14%P=>EL%P
       EL%MON14%L=>EL%L
       nullify(EL%MON14%X);ALLOCATE(EL%MON14%X);EL%MON14%X=zero;
       nullify(EL%MON14%Y);ALLOCATE(EL%MON14%Y);EL%MON14%Y=zero
    CASE(KIND15)
       if(.not.ASSOCIATED(EL%SEP15))ALLOCATE(EL%SEP15)
       EL%SEP15%P=>EL%P
       EL%SEP15%L=>EL%L
       EL%SEP15%VOLT=>EL%VOLT
       EL%SEP15%PHAS=>EL%PHAS
    CASE(KIND16,KIND20)
       if(.not.ASSOCIATED(EL%K16)) THEN
          ALLOCATE(EL%K16)
          el%K16=0
       ELSE
          el%K16=-1
          el%K16=0
       ENDIF
       EL%K16%P=>EL%P
       EL%K16%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%K16%AN=>EL%AN
       EL%K16%BN=>EL%BN
       EL%K16%FINT=>EL%FINT
       EL%K16%HGAP=>EL%HGAP
       EL%K16%H1=>EL%H1
       EL%K16%H2=>EL%H2
       NULLIFY(EL%K16%DRIFTKICK);ALLOCATE(EL%K16%DRIFTKICK);EL%K16%DRIFTKICK=.true.;
       NULLIFY(EL%K16%LIKEMAD);ALLOCATE(EL%K16%LIKEMAD);EL%K16%LIKEMAD=.false.;
    CASE(KIND17)
       call SET_IN
       IF(EL%P%EXACT.AND.EL%P%B0/=zero) THEN
          w_p=0
          w_p%nc=2
          w_p%fc='((1X,A72,/,1X,A72))'
          w_p%c(1)=" ERROR IN SETFAMILYP "
          write(w_p%c(2),'(A42,1x,I4)') " EXACT BEND OPTION NOT SUPPORTED FOR KIND ", EL%KIND
          CALL WRITE_E(777)
       ENDIF
       if(.not.ASSOCIATED(EL%S17)) THEN
          ALLOCATE(EL%S17)
          el%S17=0
       ELSE
          el%S17=-1
          el%S17=0
       ENDIF
       EL%S17%P=>EL%P
       EL%S17%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,2)
       EL%S17%AN=>EL%AN
       EL%S17%BN=>EL%BN
       EL%S17%FINT=>EL%FINT
       EL%S17%HGAP=>EL%HGAP
       EL%S17%H1=>EL%H1
       EL%S17%H2=>EL%H2
       EL%S17%B_SOL=>EL%B_SOL
       nullify(EL%S17%MAT);ALLOCATE(EL%S17%MAT(4,4));
       nullify(EL%S17%LXY);ALLOCATE(EL%S17%LXY(0:10));
    CASE(KIND18)
       if(.not.ASSOCIATED(EL%RCOL18)) THEN
          ALLOCATE(EL%RCOL18)
          EL%RCOL18=0
       ELSE
          EL%RCOL18=-1
          EL%RCOL18=0
       ENDIF
       EL%RCOL18%P=>EL%P
       EL%RCOL18%L=>EL%L
       nullify(EL%RCOL18%A);ALLOCATE(EL%RCOL18%A);CALL ALLOC(EL%RCOL18%A)
    CASE(KIND19)
       if(.not.ASSOCIATED(EL%ECOL19)) THEN
          ALLOCATE(EL%ECOL19)
          EL%ECOL19=0
       ELSE
          EL%ECOL19=-1
          EL%ECOL19=0
       ENDIF
       EL%ECOL19%P=>EL%P
       EL%ECOL19%L=>EL%L
       nullify(EL%ECOL19%A);ALLOCATE(EL%ECOL19%A);CALL ALLOC(EL%ECOL19%A)
    CASE(KIND22)
       if(.not.ASSOCIATED(EL%M22)) THEN
          ALLOCATE(EL%M22)
          el%M22=0
       ELSE
          el%M22=-1
          el%M22=0
       ENDIF
       EL%M22%P=>EL%P
       allocate(EL%M22%DELTAMAP)
       EL%M22%DELTAMAP=.TRUE.
       if(NTOT/=0) then
          allocate(EL%M22%T)
          CALL ALLOC_TREE(EL%M22%T,NTOT,ND2)
       endif
       if(NTOT_rad/=0) then
          allocate(EL%M22%T_rad)
          CALL ALLOC_TREE(EL%M22%T_rad,NTOT_rad,ND2)
       endif
       if(NTOT_REV/=0) then
          allocate(EL%M22%T_REV)
          CALL ALLOC_TREE(EL%M22%T_REV,NTOT_REV,ND2)
       endif
       if(NTOT_rad_REV/=0) then
          allocate(EL%M22%T_rad_REV)
          CALL ALLOC_TREE(EL%M22%T_rad_REV,NTOT_rad_REV,ND2)
       endif
    CASE(KINDUSER1)
       if(.not.ASSOCIATED(EL%U1)) THEN
          ALLOCATE(EL%U1)
          EL%U1=0
       ELSE
          EL%U1=-1
          EL%U1=0
       ENDIF
       EL%U1%P=>EL%P
       EL%U1%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%U1%AN=>EL%AN
       EL%U1%BN=>EL%BN
       CALL POINTERS_USER1(EL%U1)
       CALL ALLOC(EL%U1)
    CASE(KINDUSER2)
       if(.not.ASSOCIATED(EL%U2)) THEN
          ALLOCATE(EL%U2)
          EL%U2=0
       ELSE
          EL%U2=-1
          EL%U2=0
       ENDIF
       EL%U2%P=>EL%P
       EL%U2%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%U2%AN=>EL%AN
       EL%U2%BN=>EL%BN
       CALL POINTERS_USER2(EL%U2)
       CALL ALLOC(EL%U2)
    CASE(KINDWIGGLER)
       if(.not.ASSOCIATED(EL%WI)) THEN
          ALLOCATE(EL%WI)
          EL%WI=0
       ELSE
          EL%WI=-1
          EL%WI=0
       ENDIF
       EL%WI%P=>EL%P
       EL%WI%L=>EL%L
       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       EL%WI%AN=>EL%AN
       EL%WI%BN=>EL%BN
       CALL POINTERS_SAGAN(EL%WI)
       CALL ALLOC(EL%WI)
    CASE(KINDpa)
       if(.not.ASSOCIATED(EL%pa)) THEN
          ALLOCATE(EL%pa)
          EL%PA=0
       ELSE
          EL%pa=-1
          EL%pa=0
       ENDIF
       EL%pa%P=>EL%P
       EL%pa%L=>EL%L
       !       IF(EL%P%NMUL==0) CALL ZERO_ANBN(EL,1)
       !       EL%mu%AN=>EL%AN
       !       EL%mu%BN=>EL%BN
       CALL POINTERS_pancake(EL%pa,T,t_ax,t_ay)
       CALL ALLOC(EL%pa%SCALE)
    END SELECT

  END SUBROUTINE SETFAMILYP


  SUBROUTINE MIS_(EL,X)
    IMPLICIT NONE
    TYPE(ELEMENT), INTENT(inOUT) ::EL
    real(dp), INTENT(IN) ::X(6)
    INTEGER I

    IF(.NOT.ASSOCIATED(EL%D)) ALLOCATE(EL%D(3))
    IF(.NOT.ASSOCIATED(EL%R)) ALLOCATE(EL%R(3))


    DO I=1,3
       EL%D(I)=X(I)
       EL%R(I)=X(3+I)
    ENDDO

  END SUBROUTINE MIS_

  SUBROUTINE MIS_p(EL,X)
    IMPLICIT NONE
    TYPE(ELEMENTp), INTENT(inOUT) ::EL
    real(dp), INTENT(IN) ::X(6)
    INTEGER I

    IF(.NOT.ASSOCIATED(EL%D)) ALLOCATE(EL%D(3))
    IF(.NOT.ASSOCIATED(EL%R)) ALLOCATE(EL%R(3))


    DO I=1,3
       EL%D(I)=X(I)
       EL%R(I)=X(3+I)
    ENDDO

  END SUBROUTINE MIS_p

  SUBROUTINE ZERO_ANBN_R(EL,N)
    IMPLICIT NONE
    TYPE(ELEMENT), INTENT(INOUT) ::EL
    INTEGER, INTENT(IN) ::N
    INTEGER I

    IF(N<=0) RETURN
    IF(ASSOCIATED(EL%AN)) DEALLOCATE(EL%AN)
    IF(ASSOCIATED(EL%BN)) DEALLOCATE(EL%BN)
    EL%p%NMUL=N
    ALLOCATE(EL%AN(EL%p%NMUL),EL%BN(EL%p%NMUL))

    DO I=1,EL%P%NMUL
       EL%AN(I)=zero
       EL%BN(I)=zero
    ENDDO

  END SUBROUTINE ZERO_ANBN_R

  SUBROUTINE ZERO_ANBN_P(EL,N)
    IMPLICIT NONE
    TYPE(ELEMENTP), INTENT(INOUT) ::EL
    INTEGER, INTENT(IN) ::N

    IF(N<=0) RETURN
    IF(ASSOCIATED(EL%AN)) DEALLOCATE(EL%AN)
    IF(ASSOCIATED(EL%BN)) DEALLOCATE(EL%BN)
    EL%P%NMUL=N
    ALLOCATE(EL%AN(EL%P%NMUL),EL%BN(EL%P%NMUL))
    CALL ALLOC(EL%AN,EL%P%NMUL);CALL ALLOC(EL%BN,EL%P%NMUL);

  END SUBROUTINE ZERO_ANBN_P

  SUBROUTINE ADDR_ANBN(EL,NM,F,V)
    IMPLICIT NONE
    TYPE(ELEMENT), INTENT(INOUT) ::EL
    real(dp), INTENT(IN) ::V
    INTEGER, INTENT(IN) ::NM,F
    INTEGER I,N
    real(dp), ALLOCATABLE,dimension(:)::AN,BN

    N=NM
    IF(NM<0) N=-N
    ! ALREADY THERE
    IF(EL%P%NMUL>=N) THEN
       IF(NM>0) THEN
          EL%BN(N)= F*EL%BN(N)+V
       ELSE
          EL%AN(N)= F*EL%AN(N)+V
       ENDIF
       if(el%kind==kind10) then
          call GETANBN(EL%TP10)
       endif
       if(el%kind==kind7) then
          call GETMAT7(EL%T7)
       endif
       RETURN
    ENDIF

    allocate(AN(N),BN(N))
    DO I=1,EL%P%NMUL
       AN(I)=EL%AN(I)
       BN(I)=EL%BN(I)
    ENDDO
    DO I=EL%P%NMUL+1,N
       AN(I)=zero
       BN(I)=zero
    ENDDO
    IF(NM<0) THEN
       AN(N)=V
    ELSE
       BN(N)=V
    ENDIF


    IF(ASSOCIATED(EL%AN)) DEALLOCATE(EL%AN)
    IF(ASSOCIATED(EL%BN)) DEALLOCATE(EL%BN)
    EL%P%NMUL=N
    ALLOCATE(EL%AN(EL%P%NMUL),EL%BN(EL%P%NMUL))

    DO I=1,EL%P%NMUL
       EL%AN(I)=AN(I)
       EL%BN(I)=BN(I)
    ENDDO

    DEALLOCATE(AN);DEALLOCATE(BN);

    SELECT CASE(EL%KIND)
    CASE(KIND2,KIND3,KIND5,KIND6,KIND17)
       select case(EL%KIND)
       case(kind2)
          EL%K2%AN=>EL%AN
          EL%K2%BN=>EL%BN
       case(kind3)
          EL%K3%AN=>EL%AN
          EL%K3%BN=>EL%BN
       case(kind5)
          EL%S5%AN=>EL%AN
          EL%S5%BN=>EL%BN
       case(kind6)
          EL%T6%AN=>EL%AN
          EL%T6%BN=>EL%BN
       case(kind17)
          EL%S17%AN=>EL%AN
          EL%S17%BN=>EL%BN
       end select
    CASE(KIND7)
       EL%T7%AN=>EL%AN
       EL%T7%BN=>EL%BN
       call GETMAT7(EL%T7)
    CASE(KIND8)
       EL%S8%BN=>EL%BN
    CASE(KIND9)
       EL%S9%AN=>EL%AN
    CASE(KIND10)
       EL%TP10%AN=>EL%AN
       EL%TP10%BN=>EL%BN
       call GETANBN(EL%TP10)
    CASE(KIND16,KIND20)
       EL%K16%AN=>EL%AN
       EL%K16%BN=>EL%BN
    CASE(KINDuser1)
       EL%U1%AN=>EL%AN
       EL%U1%BN=>EL%BN
    CASE(KINDuser2)
       EL%U2%AN=>EL%AN
       EL%U2%BN=>EL%BN
    CASE(KINDWIGGLER)
       EL%WI%AN=>EL%AN
       EL%WI%BN=>EL%BN
    case default
       w_p=0
       w_p%nc=1
       w_p%fc='((1X,A72,/,1X,A72))'
       write(w_p%c(1),'(A13,A24,A27)')" THIS MAGNET ", MYTYPE(EL%KIND), " CANNOT ACCEPT ANs AND BNs "
       CALL WRITE_E(988)
    END SELECT


    !    if(el%kind==kind10) then
    !    call GETANBN(EL%TP10)
    !    endif
    !    if(el%kind==kind7) then
    !       call GETMAT7(EL%T7)
    !    endif

  END SUBROUTINE ADDR_ANBN

  SUBROUTINE ADDP_ANBN(EL,NM,F,V)
    IMPLICIT NONE
    TYPE(ELEMENTP), INTENT(INOUT) ::EL
    real(dp), INTENT(IN) ::V
    INTEGER, INTENT(IN) ::NM,F
    INTEGER I,N
    TYPE(REAL_8), ALLOCATABLE,dimension(:)::AN,BN

    N=NM
    IF(NM<0) N=-N
    ! ALREADY THERE
    IF(EL%P%NMUL>=N) THEN
       IF(NM>0) THEN
          EL%BN(N)= F*EL%BN(N)+V
       ELSE
          EL%AN(N)= F*EL%AN(N)+V
       ENDIF
       if(el%kind==kind10) then
          call GETANBN(EL%TP10)
       endif
       if(el%kind==kind7) then
          call GETMAT7(EL%T7)     !etienne
       endif
       RETURN
    ENDIF

    allocate(AN(N),BN(N))
    CALL ALLOC(AN,N);CALL ALLOC(BN,N);
    DO I=1,EL%P%NMUL
       AN(I)=EL%AN(I)
       BN(I)=EL%BN(I)
    ENDDO
    IF(NM<0) THEN
       AN(N)=V
    ELSE
       BN(N)=V
    ENDIF

    CALL KILL(EL%AN,EL%P%NMUL);CALL KILL(EL%AN,EL%P%NMUL);
    IF(ASSOCIATED(EL%AN)) DEALLOCATE(EL%AN)
    IF(ASSOCIATED(EL%BN)) DEALLOCATE(EL%BN)
    EL%P%NMUL=N
    ALLOCATE(EL%AN(EL%P%NMUL),EL%BN(EL%P%NMUL))
    CALL ALLOC(EL%AN,EL%P%NMUL);CALL ALLOC(EL%BN,EL%P%NMUL);  ! BUG there

    DO I=1,EL%P%NMUL
       EL%AN(I)=AN(I)
       EL%BN(I)=BN(I)
    ENDDO

    DEALLOCATE(AN);DEALLOCATE(BN);

    SELECT CASE(EL%KIND)
    CASE(KIND2,KIND3,KIND5,KIND6,KIND17)
       select case(EL%KIND)
       case(kind2)
          EL%K2%AN=>EL%AN
          EL%K2%BN=>EL%BN
       case(kind3)
          EL%K3%AN=>EL%AN
          EL%K3%BN=>EL%BN
       case(kind5)
          EL%S5%AN=>EL%AN
          EL%S5%BN=>EL%BN
       case(kind6)
          EL%T6%AN=>EL%AN
          EL%T6%BN=>EL%BN
       case(kind17)
          EL%S17%AN=>EL%AN
          EL%S17%BN=>EL%BN
       end select
    CASE(KIND7)
       EL%T7%AN=>EL%AN
       EL%T7%BN=>EL%BN
       call GETMAT7(EL%T7)
    CASE(KIND8)
       EL%S8%BN=>EL%BN
    CASE(KIND9)
       EL%S9%AN=>EL%AN
    CASE(KIND10)
       EL%TP10%AN=>EL%AN
       EL%TP10%BN=>EL%BN
       call GETANBN(EL%TP10)
    CASE(KIND16,KIND20)
       EL%K16%AN=>EL%AN
       EL%K16%BN=>EL%BN
    CASE(KINDuser1)
       EL%U1%AN=>EL%AN
       EL%U1%BN=>EL%BN
    CASE(KINDuser2)
       EL%U2%AN=>EL%AN
       EL%U2%BN=>EL%BN
    CASE(KINDWIGGLER)
       EL%WI%AN=>EL%AN
       EL%WI%BN=>EL%BN
    case default
       w_p=0
       w_p%nc=1
       w_p%fc='((1X,A72,/,1X,A72))'
       write(w_p%c(1),'(A13,A24,A27)')" THIS MAGNET ", MYTYPE(EL%KIND), " CANNOT ACCEPT ANs AND BNs "
       CALL WRITE_E(987)
    END SELECT

    !if(el%kind==kind10) then
    !call GETANBN(EL%TP10)
    !endif
    !if(el%kind==kind7) then
    !   call GETMAT7(EL%T7)
    !endif

  END SUBROUTINE ADDP_ANBN




  SUBROUTINE MAGSTATE(EL,S)
    IMPLICIT NONE
    TYPE(ELEMENT), INTENT(INOUT)::EL
    TYPE(INTERNAL_STATE), INTENT(IN)::S

    IF(S%TOTALPATH) THEN
       EL%P%TOTALPATH=1
    ELSE
       EL%P%TOTALPATH=0
    ENDIF
    EL%P%RADIATION=S%RADIATION
    EL%P%TIME=S%TIME
    EL%P%NOCAVITY=S%NOCAVITY
    EL%P%FRINGE=S%FRINGE.or.EL%PERMFRINGE
  END SUBROUTINE MAGSTATE


  SUBROUTINE MAGPSTATE(EL,S)
    IMPLICIT NONE
    TYPE(ELEMENTP), INTENT(INOUT)::EL
    TYPE(INTERNAL_STATE), INTENT(IN)::S

    IF(S%TOTALPATH) THEN
       EL%P%TOTALPATH=1
    ELSE
       EL%P%TOTALPATH=0
    ENDIF
    EL%P%RADIATION=S%RADIATION
    EL%P%TIME=S%TIME
    EL%P%NOCAVITY=S%NOCAVITY
    EL%P%FRINGE=S%FRINGE.or.EL%PERMFRINGE
  END SUBROUTINE MAGPSTATE


  SUBROUTINE null_EL(EL)
    IMPLICIT NONE
    TYPE(ELEMENT), INTENT(INOUT)::EL
    nullify(EL%KIND);
    nullify(EL%NAME);nullify(EL%vorname);

    nullify(EL%PERMFRINGE);
    nullify(EL%L);
    nullify(EL%AN);nullify(EL%BN);
    nullify(EL%FINT);nullify(EL%HGAP);
    nullify(EL%H1);nullify(EL%H2);
    nullify(EL%VOLT);nullify(EL%FREQ);nullify(EL%PHAS);nullify(EL%DELTA_E);
    nullify(EL%B_SOL);
    nullify(EL%THIN);
    nullify(EL%MIS);nullify(EL%EXACTMIS);
    nullify(EL%D);nullify(EL%R);
    nullify(EL%D0);
    nullify(EL%K2);
    nullify(EL%K16);
    nullify(EL%K3);
    nullify(EL%C4);
    nullify(EL%CAV21);
    nullify(EL%S5);
    nullify(EL%T6);
    nullify(EL%M22);
    nullify(EL%S17);
    nullify(EL%T7);
    nullify(EL%S8);
    nullify(EL%S9);
    nullify(EL%TP10);
    nullify(EL%MON14);
    nullify(EL%SEP15);
    nullify(EL%RCOL18);
    nullify(EL%ECOL19);
    nullify(EL%U1);
    nullify(EL%U2);
    nullify(EL%WI);
    nullify(EL%PA);
    nullify(EL%P);
    nullify(EL%PARENT_FIBRE);
  end SUBROUTINE null_EL

  SUBROUTINE null_ELp(EL)
    IMPLICIT NONE
    TYPE(ELEMENTP), INTENT(INOUT)::EL

    nullify(EL%KNOB);
    nullify(EL%KIND);
    nullify(EL%NAME);nullify(EL%vorname);

    nullify(EL%PERMFRINGE);
    nullify(EL%L);
    nullify(EL%AN);nullify(EL%BN);
    nullify(EL%FINT);nullify(EL%HGAP);
    nullify(EL%H1);nullify(EL%H2);
    nullify(EL%VOLT);nullify(EL%FREQ);nullify(EL%PHAS);nullify(EL%DELTA_E);
    nullify(EL%B_SOL);
    nullify(EL%THIN);
    nullify(EL%MIS);nullify(EL%EXACTMIS);
    nullify(EL%D);nullify(EL%R);
    nullify(EL%D0);
    nullify(EL%K2);
    nullify(EL%K16);
    nullify(EL%K3);
    nullify(EL%C4);
    nullify(EL%CAV21);
    nullify(EL%S5);
    nullify(EL%T6);
    nullify(EL%M22);
    nullify(EL%S17);
    nullify(EL%T7);
    nullify(EL%S8);
    nullify(EL%S9);
    nullify(EL%TP10);
    nullify(EL%MON14);
    nullify(EL%SEP15);
    nullify(EL%RCOL18);
    nullify(EL%ECOL19);
    nullify(EL%U1);
    nullify(EL%U2);
    nullify(EL%WI);
    nullify(EL%PA);
    nullify(EL%P);
    nullify(EL%PARENT_FIBRE);
  end SUBROUTINE null_ELp



  SUBROUTINE ZERO_EL(EL,I)
    IMPLICIT NONE
    TYPE(ELEMENT), INTENT(INOUT)::EL
    INTEGER, INTENT(IN)::I

    IF(I==-1) THEN

       DEALLOCATE(EL%KIND);
       DEALLOCATE(EL%NAME);DEALLOCATE(EL%VORNAME);
       DEALLOCATE(EL%PERMFRINGE);
       DEALLOCATE(EL%L);
       DEALLOCATE(EL%MIS);DEALLOCATE(EL%EXACTMIS);

       call kill(EL%P)    ! AIMIN MS 4.0
       IF(ASSOCIATED(EL%R)) DEALLOCATE(EL%R)
       IF(ASSOCIATED(EL%D)) DEALLOCATE(EL%D)
       IF(ASSOCIATED(EL%AN)) DEALLOCATE(EL%AN)
       IF(ASSOCIATED(EL%BN)) DEALLOCATE(EL%BN)
       IF(ASSOCIATED(EL%FINT)) DEALLOCATE(EL%FINT)
       IF(ASSOCIATED(EL%HGAP)) DEALLOCATE(EL%HGAP)
       IF(ASSOCIATED(EL%H1)) DEALLOCATE(EL%H1)
       IF(ASSOCIATED(EL%H2)) DEALLOCATE(EL%H2)
       IF(ASSOCIATED(EL%VOLT)) DEALLOCATE(EL%VOLT)
       IF(ASSOCIATED(EL%FREQ)) DEALLOCATE(EL%FREQ)
       IF(ASSOCIATED(EL%PHAS)) DEALLOCATE(EL%PHAS)
       IF(ASSOCIATED(EL%DELTA_E)) DEALLOCATE(EL%DELTA_E)
       IF(ASSOCIATED(EL%B_SOL)) DEALLOCATE(EL%B_SOL)
       IF(ASSOCIATED(EL%THIN)) DEALLOCATE(EL%THIN)
       IF(ASSOCIATED(EL%d0)) DEALLOCATE(EL%d0)       ! drift
       IF(ASSOCIATED(EL%K2)) DEALLOCATE(EL%K2)       ! INTEGRATOR
       !       IF(ASSOCIATED(EL%K16)) DEALLOCATE(EL%K16)       ! INTEGRATOR
       !       IF(ASSOCIATED(EL%K3)) DEALLOCATE(EL%K3)       !  THIN LENS
       IF(ASSOCIATED(EL%K3)) then
          IF(ASSOCIATED(EL%K3%thin_h_foc)) DEALLOCATE(EL%K3%thin_h_foc)
          IF(ASSOCIATED(EL%K3%thin_v_foc)) DEALLOCATE(EL%K3%thin_v_foc)
          IF(ASSOCIATED(EL%K3%thin_h_angle)) DEALLOCATE(EL%K3%thin_h_angle)
          IF(ASSOCIATED(EL%K3%thin_v_angle)) DEALLOCATE(EL%K3%thin_v_angle)
          DEALLOCATE(EL%K3)
       endif

       IF(ASSOCIATED(EL%S5)) DEALLOCATE(EL%S5)       ! SOLENOID
       !       IF(ASSOCIATED(EL%T6)) DEALLOCATE(EL%T6)       ! INTEGRATOR
       !       IF(ASSOCIATED(EL%T7)) DEALLOCATE(EL%T7)       ! INTEGRATOR
       IF(ASSOCIATED(EL%S8)) DEALLOCATE(EL%S8)       ! NORMAL SMI
       IF(ASSOCIATED(EL%S9)) DEALLOCATE(EL%S9)       ! SKEW SMI
       !       IF(ASSOCIATED(EL%TP10)) DEALLOCATE(EL%TP10)   ! SECTOR TEAPOT
       IF(ASSOCIATED(EL%T6)) THEN
          EL%T6=-1
          DEALLOCATE(EL%T6)   ! thick sixtrack
       ENDIF
       IF(ASSOCIATED(EL%M22)) THEN
          EL%M22=-1
          DEALLOCATE(EL%M22)   ! thick sixtrack
       ENDIF
       IF(ASSOCIATED(EL%S17)) THEN
          EL%S17=-1
          DEALLOCATE(EL%S17)   ! thick sixtrack
       ENDIF
       IF(ASSOCIATED(EL%T7)) THEN
          EL%T7=-1
          DEALLOCATE(EL%T7)   ! thick
       ENDIF
       IF(ASSOCIATED(EL%C4)) THEN
          EL%C4=-1
          DEALLOCATE(EL%C4)   ! MONITOR
       ENDIF
       IF(ASSOCIATED(EL%CAV21)) THEN
          EL%CAV21=-1
          DEALLOCATE(EL%CAV21)   ! MONITOR
       ENDIF
       IF(ASSOCIATED(EL%TP10)) then
          EL%TP10=-1
          DEALLOCATE(EL%TP10)   ! SECTOR TEAPOT
       ENDIF
       IF(ASSOCIATED(EL%MON14)) THEN
          EL%MON14=-1
          DEALLOCATE(EL%MON14)   ! MONITOR
       ENDIF
       IF(ASSOCIATED(EL%RCOL18)) THEN
          EL%RCOL18=-1
          DEALLOCATE(EL%RCOL18)   ! RCOLLIMATOR
       ENDIF
       IF(ASSOCIATED(EL%ECOL19)) THEN
          EL%ECOL19=-1
          DEALLOCATE(EL%ECOL19)   ! ECOLLIMATOR
       ENDIF
       IF(ASSOCIATED(EL%SEP15)) DEALLOCATE(EL%SEP15)       ! ELSEPARATOR
       IF(ASSOCIATED(EL%K16)) then
          EL%K16=-1
          DEALLOCATE(EL%K16)       ! INTEGRATOR
       endif
       IF(ASSOCIATED(EL%U1))        then
          el%U1=-1     !USER DEFINED MAGNET
          DEALLOCATE(EL%U1)
       ENDIF

       IF(ASSOCIATED(EL%U2))        then
          el%U2=-1     !USER DEFINED MAGNET
          DEALLOCATE(EL%U2)
       ENDIF

       IF(ASSOCIATED(EL%WI))        then
          el%WI=-1     !USER DEFINED MAGNET
          DEALLOCATE(EL%WI)
       ENDIF

       IF(ASSOCIATED(EL%PARENT_FIBRE))        then
          DEALLOCATE(EL%PARENT_FIBRE)
       ENDIF


    elseif(I>=0)       then

       !FIRST nullifies

       call null_ELEment(el)

       call alloc(el%P);

       ALLOCATE(EL%KIND);EL%KIND=0;
       ALLOCATE(EL%NAME);ALLOCATE(EL%VORNAME);
       EL%NAME=' ';EL%NAME=TRIM(ADJUSTL(EL%NAME));
       EL%VORNAME=' ';EL%VORNAME=TRIM(ADJUSTL(EL%VORNAME));

       ALLOCATE(EL%PERMFRINGE);EL%PERMFRINGE=.FALSE.;  ! PART OF A STATE INITIALIZED BY EL=DEFAULT
       ALLOCATE(EL%L);EL%L=zero;
       ALLOCATE(EL%MIS);ALLOCATE(EL%EXACTMIS);EL%MIS=.FALSE.;EL%EXACTMIS=ALWAYS_EXACTMIS;
       EL=DEFAULT;
       !   ANBN
       CALL ZERO_ANBN(EL,I)
       ALLOCATE(EL%FINT);EL%FINT=half;
       ALLOCATE(EL%HGAP);EL%HGAP=zero;
       ALLOCATE(EL%H1);EL%H1=zero;
       ALLOCATE(EL%H2);EL%H2=zero;

    ENDIF

  END SUBROUTINE ZERO_EL

  SUBROUTINE ZERO_ELP(EL,I)
    IMPLICIT NONE
    TYPE(ELEMENTP), INTENT(INOUT)::EL
    INTEGER, INTENT(IN)::I
    INTEGER J

    IF(I==-1) THEN

       IF(ASSOCIATED(EL%P%NMUL))THEN
          IF(EL%P%NMUL>0) THEN
             DO  J=1,EL%P%NMUL
                CALL KILL(EL%AN(J))
                CALL KILL(EL%BN(J))
             ENDDO
             IF(ASSOCIATED(EL%AN)) DEALLOCATE(EL%AN)
             IF(ASSOCIATED(EL%BN)) DEALLOCATE(EL%BN)
          ENDIF
       ENDIF

       IF(ASSOCIATED(EL%d0)) DEALLOCATE(EL%d0)       ! drift
       IF(ASSOCIATED(EL%K2)) DEALLOCATE(EL%K2)       ! INTEGRATOR
       !       IF(ASSOCIATED(EL%K16)) DEALLOCATE(EL%K16)       ! INTEGRATOR
       !       IF(ASSOCIATED(EL%K3)) DEALLOCATE(EL%K3)       !  THIN LENS
       IF(ASSOCIATED(EL%K3)) then
          IF(ASSOCIATED(EL%K3%thin_h_foc)) DEALLOCATE(EL%K3%thin_h_foc)
          IF(ASSOCIATED(EL%K3%thin_v_foc)) DEALLOCATE(EL%K3%thin_v_foc)
          IF(ASSOCIATED(EL%K3%thin_h_angle)) DEALLOCATE(EL%K3%thin_h_angle)
          IF(ASSOCIATED(EL%K3%thin_v_angle)) DEALLOCATE(EL%K3%thin_v_angle)
          DEALLOCATE(EL%K3)
       endif

       IF(ASSOCIATED(EL%C4)) THEN
          EL%C4=-1
          DEALLOCATE(EL%C4)       ! CAVITY
          CALL KILL(EL%VOLT)
          CALL KILL(EL%FREQ)
          CALL KILL(EL%PHAS)
          IF(ASSOCIATED(EL%VOLT)) DEALLOCATE(EL%VOLT)
          IF(ASSOCIATED(EL%FREQ)) DEALLOCATE(EL%FREQ)
          IF(ASSOCIATED(EL%PHAS)) DEALLOCATE(EL%PHAS)
          IF(ASSOCIATED(EL%DELTA_E)) DEALLOCATE(EL%DELTA_E)
          IF(ASSOCIATED(EL%THIN)) DEALLOCATE(EL%THIN)
       ENDIF

       IF(ASSOCIATED(EL%CAV21)) THEN
          EL%CAV21=-1
          DEALLOCATE(EL%CAV21)       ! CAVITY
          CALL KILL(EL%VOLT)
          CALL KILL(EL%FREQ)
          CALL KILL(EL%PHAS)
          IF(ASSOCIATED(EL%VOLT)) DEALLOCATE(EL%VOLT)
          IF(ASSOCIATED(EL%FREQ)) DEALLOCATE(EL%FREQ)
          IF(ASSOCIATED(EL%PHAS)) DEALLOCATE(EL%PHAS)
          IF(ASSOCIATED(EL%DELTA_E)) DEALLOCATE(EL%DELTA_E)
          IF(ASSOCIATED(EL%THIN)) DEALLOCATE(EL%THIN)
       ENDIF

       IF(ASSOCIATED(EL%S5)) THEN
          DEALLOCATE(EL%S5)       ! solenoid
          !          CALL KILL(EL%B_SOL)    ! sagan
          !         IF(ASSOCIATED(EL%B_SOL)) DEALLOCATE(EL%B_SOL)     ! sagan
       ENDIF
       IF(ASSOCIATED(EL%T6)) then
          EL%T6=-1
          DEALLOCATE(EL%T6)       ! INTEGRATOR
       endif
       IF(ASSOCIATED(EL%M22)) then
          EL%M22=-1
          DEALLOCATE(EL%M22)       ! INTEGRATOR
       endif
       IF(ASSOCIATED(EL%S17)) then
          EL%S17=-1
          DEALLOCATE(EL%S17)       ! INTEGRATOR
       endif
       IF(ASSOCIATED(EL%T7)) then
          EL%T7=-1
          DEALLOCATE(EL%T7)       ! INTEGRATOR
       ENDIF
       IF(ASSOCIATED(EL%S8)) DEALLOCATE(EL%S8)       ! SMI KICK
       IF(ASSOCIATED(EL%S9)) DEALLOCATE(EL%S9)       ! SKEW SMI KICK
       IF(ASSOCIATED(EL%MON14)) THEN
          EL%MON14=-1
          DEALLOCATE(EL%MON14)   ! MONITOR
       ENDIF
       IF(ASSOCIATED(EL%RCOL18)) THEN
          EL%RCOL18=-1
          DEALLOCATE(EL%RCOL18)   ! RCOLLIMATOR
       ENDIF
       IF(ASSOCIATED(EL%ECOL19)) THEN
          EL%ECOL19=-1
          DEALLOCATE(EL%ECOL19)   ! ECOLLIMATOR
       ENDIF
       IF(ASSOCIATED(EL%K16)) then
          EL%K16=-1
          DEALLOCATE(EL%K16)       ! INTEGRATOR
       endif
       IF(ASSOCIATED(EL%SEP15)) THEN
          DEALLOCATE(EL%SEP15)       ! CAVITY
          CALL KILL(EL%VOLT); CALL KILL(EL%PHAS);
          IF(ASSOCIATED(EL%VOLT)) DEALLOCATE(EL%VOLT)
          IF(ASSOCIATED(EL%PHAS)) DEALLOCATE(EL%PHAS)
       ENDIF
       IF(ASSOCIATED(EL%TP10)) then
          EL%TP10=-1
          DEALLOCATE(EL%TP10)       ! INTEGRATOR SECTOR EXACT
       ENDIF
       IF(ASSOCIATED(EL%U1))        then
          el%U1=-1
          DEALLOCATE(EL%U1)
       ENDIF
       IF(ASSOCIATED(EL%U2))        then
          el%U2=-1
          DEALLOCATE(EL%U2)
       ENDIF

       IF(ASSOCIATED(EL%WI))        then
          el%WI=-1
          DEALLOCATE(EL%WI)
       ENDIF

       IF(ASSOCIATED(EL%PARENT_FIBRE))        then
          DEALLOCATE(EL%PARENT_FIBRE)
       ENDIF


       DEALLOCATE(EL%KIND);DEALLOCATE(EL%KNOB);
       DEALLOCATE(EL%NAME);DEALLOCATE(EL%VORNAME);
       DEALLOCATE(EL%PERMFRINGE);
       CALL KILL(EL%L);DEALLOCATE(EL%L);
       CALL KILL(EL%FINT);DEALLOCATE(EL%FINT);
       CALL KILL(EL%HGAP);DEALLOCATE(EL%HGAP);
       CALL KILL(EL%H1);DEALLOCATE(EL%H1);
       CALL KILL(EL%H2);DEALLOCATE(EL%H2);
       DEALLOCATE(EL%MIS);DEALLOCATE(EL%EXACTMIS);

       call kill(EL%P)        ! call kill(EL%P)    ! AIMIN MS 4.0

       IF(ASSOCIATED(EL%R)) DEALLOCATE(EL%R)
       IF(ASSOCIATED(EL%D)) DEALLOCATE(EL%D)
       !       IF(ASSOCIATED(EL%B_SOL)) DEALLOCATE(EL%B_SOL)  ! sagan

       IF(ASSOCIATED(EL%B_SOL)) then ! sagan
          CALL KILL(EL%B_SOL) ! sagan
          DEALLOCATE(EL%B_SOL)     ! sagan
       endif   ! sagan

       IF(ASSOCIATED(EL%THIN)) DEALLOCATE(EL%THIN)


    elseif(I>=0)       then

       !FIRST nullifies


       call null_ELEment(el)

       call alloc(el%P)

       ALLOCATE(EL%KIND);EL%KIND=0;ALLOCATE(EL%KNOB);EL%KNOB=.FALSE.;
       ALLOCATE(EL%NAME);ALLOCATE(EL%VORNAME);
       EL%NAME=' ';EL%NAME=TRIM(ADJUSTL(EL%NAME));
       EL%VORNAME=' ';EL%VORNAME=TRIM(ADJUSTL(EL%VORNAME));
       ALLOCATE(EL%PERMFRINGE);EL%PERMFRINGE=.FALSE.;  ! PART OF A STATE INITIALIZED BY EL=DEFAULT
       ALLOCATE(EL%L);CALL ALLOC(EL%L);EL%L=zero;
       ALLOCATE(EL%MIS);ALLOCATE(EL%EXACTMIS);EL%MIS=.FALSE.;EL%EXACTMIS=ALWAYS_EXACTMIS;
       EL=DEFAULT;
       !   ANBN
       CALL ZERO_ANBN(EL,I)
       ALLOCATE(EL%FINT);CALL ALLOC(EL%FINT);EL%FINT=half;
       ALLOCATE(EL%HGAP);CALL ALLOC(EL%HGAP);EL%HGAP=zero;
       ALLOCATE(EL%H1);CALL ALLOC(EL%H1);EL%H1=zero;
       ALLOCATE(EL%H2);CALL ALLOC(EL%H2);EL%H2=zero;
    ENDIF

  END SUBROUTINE ZERO_ELP

  SUBROUTINE cop_el_elp(EL,ELP)
    IMPLICIT NONE
    TYPE(ELEMENT),INTENT(IN)::  EL
    TYPE(ELEMENTP),INTENT(inOUT)::  ELP
    CALL EQUAL(ELP,EL)
  END SUBROUTINE cop_el_elp

  SUBROUTINE cop_elp_el(EL,ELP)
    IMPLICIT NONE
    TYPE(ELEMENTP),INTENT(IN)::  EL
    TYPE(ELEMENT),INTENT(inOUT)::  ELP
    CALL EQUAL(ELP,EL)
  END SUBROUTINE       cop_elp_el

  SUBROUTINE cop_el_el(EL,ELP)
    IMPLICIT NONE
    TYPE(ELEMENT),INTENT(IN)::  EL
    TYPE(ELEMENT),INTENT(inOUT)::  ELP
    CALL EQUAL(ELP,EL)
  END SUBROUTINE       cop_el_el



  SUBROUTINE copy_el_elp(ELP,EL)
    IMPLICIT NONE
    TYPE(ELEMENT),INTENT(IN)::  EL
    TYPE(ELEMENTP),INTENT(inOUT)::  ELP
    INTEGER I,J,k,l

    ELP%PERMFRINGE=EL%PERMFRINGE
    ELP%NAME=EL%NAME
    ELP%vorname=EL%vorname
    ELP%KIND=EL%KIND
    ELP%L=EL%L
    ELP%FINT=EL%FINT
    ELP%HGAP=EL%HGAP
    ELP%H1=EL%H1
    ELP%H2=EL%H2



    IF(EL%P%NMUL>0) THEN
       IF(EL%P%NMUL/=ELP%P%NMUL.and.ELP%P%NMUL/=0) THEN
          call kill(ELP%AN,ELP%P%NMUL);call kill(ELP%bN,ELP%P%NMUL);
          DEALLOCATE(ELP%AN );DEALLOCATE(ELP%BN )
       endif
       if(.not.ASSOCIATED(ELP%AN)) THEN
          ALLOCATE(ELP%AN(EL%P%NMUL),ELP%BN(EL%P%NMUL))
       ENDIF


       CALL ALLOC(ELP%AN,EL%P%NMUL)
       CALL ALLOC(ELP%BN,EL%P%NMUL)
       DO I=1,EL%P%NMUL
          ELP%AN(I) = EL%AN(I)
          ELP%BN(I) = EL%BN(I)
       ENDDO

    ENDIF
    ELP%P=EL%P

    ! MISALIGNMENTS
    ELP%MIS=EL%MIS
    ELP%EXACTMIS=EL%EXACTMIS

    IF(ASSOCIATED(EL%R)) THEN
       if(.not.ASSOCIATED(ELP%R))  ALLOCATE(ELP%R(3))

       DO I=1,3
          ELP%R(I)=EL%R(I)
       ENDDO
    ENDIF
    IF(ASSOCIATED(EL%D)) THEN
       if(.not.ASSOCIATED(ELP%D))  ALLOCATE(ELP%D(3))

       DO I=1,3
          ELP%D(I)=EL%D(I)
       ENDDO
    ENDIF

    IF(EL%KIND==KIND1) CALL SETFAMILY(ELP)
    IF(EL%KIND==KIND2) CALL SETFAMILY(ELP)
    IF(EL%KIND==KIND16.OR.EL%KIND==KIND20) THEN
       CALL SETFAMILY(ELP)
       ELP%K16%DRIFTKICK=EL%K16%DRIFTKICK
       ELP%K16%LIKEMAD=EL%K16%LIKEMAD
    ENDIF

    IF(EL%KIND==KIND3) THEN
       if(.not.ASSOCIATED(ELP%K3)) ALLOCATE(ELP%K3)
       ELP%K3=0
       CALL SETFAMILY(ELP)
       ELP%K3%thin_h_foc=EL%K3%thin_h_foc
       ELP%K3%thin_v_foc=EL%K3%thin_v_foc
       ELP%K3%thin_h_angle=EL%K3%thin_h_angle
       ELP%K3%thin_v_angle=EL%K3%thin_v_angle
    ENDIF


    IF(EL%KIND==KIND4) THEN         !
       if(.not.ASSOCIATED(ELP%C4)) ALLOCATE(ELP%C4)
       ELP%C4=0
       if(.not.ASSOCIATED(ELP%VOLT)) ALLOCATE(ELP%VOLT,ELP%FREQ,ELP%PHAS,ELP%DELTA_E       )
       if(.not.ASSOCIATED(ELP%THIN)) ALLOCATE(ELP%THIN       )
       CALL ALLOC( ELP%VOLT)
       CALL ALLOC( ELP%FREQ)
       CALL ALLOC( ELP%PHAS)
       ELP%VOLT = EL%VOLT
       ELP%FREQ = EL%FREQ
       ELP%PHAS = EL%PHAS
       ELP%DELTA_E = EL%DELTA_E               ! DELTA_E IS real(dp)
       ELP%THIN = EL%THIN
       CALL SETFAMILY(ELP)
       ELP%C4%N_BESSEL = EL%C4%N_BESSEL
    ENDIF

    IF(EL%KIND==KIND21) THEN         !
       if(.not.ASSOCIATED(ELP%CAV21)) ALLOCATE(ELP%CAV21)
       ELP%CAV21=0
       if(.not.ASSOCIATED(ELP%VOLT)) ALLOCATE(ELP%VOLT,ELP%FREQ,ELP%PHAS,ELP%DELTA_E       )
       if(.not.ASSOCIATED(ELP%THIN)) ALLOCATE(ELP%THIN       )
       CALL ALLOC( ELP%VOLT)
       CALL ALLOC( ELP%FREQ)
       CALL ALLOC( ELP%PHAS)
       ELP%VOLT = EL%VOLT
       ELP%FREQ = EL%FREQ
       ELP%PHAS = EL%PHAS
       ELP%DELTA_E = EL%DELTA_E               ! DELTA_E IS real(dp)
       ELP%THIN = EL%THIN
       CALL SETFAMILY(ELP)
       ELP%CAV21%PSI = EL%CAV21%PSI
       ELP%CAV21%DPHAS = EL%CAV21%DPHAS
    ENDIF


    IF(EL%KIND==KIND5) THEN         !
       if(.not.ASSOCIATED(ELP%B_SOL)) ALLOCATE(ELP%B_SOL       )
       CALL ALLOC( ELP%B_SOL)
       ELP%B_SOL = EL%B_SOL
       CALL SETFAMILY(ELP)
    ENDIF

    IF(EL%KIND==KIND17) THEN         !
       !       if(.not.ASSOCIATED(ELP%S17)) ALLOCATE(ELP%S17)
       if(.not.ASSOCIATED(ELP%B_SOL)) ALLOCATE(ELP%B_SOL       )
       CALL ALLOC( ELP%B_SOL) !! *** This line added *** Sagan
       ELP%B_SOL = EL%B_SOL
       CALL SETFAMILY(ELP)
    ENDIF

    IF(EL%KIND==KIND6) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND22) THEN
       i=0;j=0;k=0;l=0;
       if(associated(EL%M22%T_REV)) i=EL%M22%T_REV%N
       if(associated(EL%M22%T_rad_REV)) j=EL%M22%T_rad_REV%N
       if(associated(EL%M22%T)) k=EL%M22%T%N
       if(associated(EL%M22%T_rad)) l=EL%M22%T_rad%N
       CALL SETFAMILY(ELP,NTOT=k,ntot_rad=l,NTOT_REV=i,ntot_rad_REV=j,ND2=6)
       ELP%M22%DELTAMAP=EL%M22%DELTAMAP
       if(associated(EL%M22%T))  CALL COPY_TREE(EL%M22%T,ELP%M22%T)
       if(associated(EL%M22%T_rad)) CALL COPY_TREE(EL%M22%T_rad,ELP%M22%T_rad)
       if(associated(EL%M22%T_REV)) CALL COPY_TREE(EL%M22%T_REV,ELP%M22%T_REV)
       if(associated(EL%M22%T_rad_REV)) CALL COPY_TREE(EL%M22%T_rad_REV,ELP%M22%T_rad_REV)
    ENDIF

    IF(EL%KIND==KIND7) THEN         !
       GEN=.FALSE.
       CALL SETFAMILY(ELP)
       IF(.NOT.GEN) THEN !.NOT.GEN
          DO J=1,3
             ELP%T7%LX(J)=EL%T7%LX(J)
             ELP%T7%RLX(J)=EL%T7%RLX(J)
             DO I=1,2
                ELP%T7%MATX(I,J)=EL%T7%MATX(I,J)
                ELP%T7%MATY(I,J)=EL%T7%MATY(I,J)
                ELP%T7%RMATX(I,J)=EL%T7%RMATX(I,J)
                ELP%T7%RMATY(I,J)=EL%T7%RMATY(I,J)
             ENDDO
          ENDDO
       ENDIF !.NOT.GEN
       GEN=.TRUE.
    ENDIF

    IF(EL%KIND==KIND8) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND9) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND10) THEN
       CALL SETFAMILY(ELP)
       ELP%TP10%DRIFTKICK=EL%TP10%DRIFTKICK
    ENDIF

    IF(EL%KIND>=KIND11.AND.EL%KIND<=KIND14) THEN
       CALL SETFAMILY(ELP)
       ELP%MON14%X=EL%MON14%X
       ELP%MON14%Y=EL%MON14%Y
    ENDIF

    IF(EL%KIND==KIND18) THEN
       CALL SETFAMILY(ELP)
       ELP%RCOL18%A=EL%RCOL18%A
    ENDIF

    IF(EL%KIND==KIND19) THEN
       CALL SETFAMILY(ELP)
       ELP%ECOL19%A=EL%ECOL19%A
    ENDIF

    IF(EL%KIND==KIND15) THEN         !
       if(.not.ASSOCIATED(ELP%VOLT)) ALLOCATE(ELP%VOLT)
       if(.not.ASSOCIATED(ELP%PHAS)) ALLOCATE(ELP%PHAS)
       CALL ALLOC( ELP%VOLT)
       CALL ALLOC( ELP%PHAS)
       ELP%VOLT = EL%VOLT
       ELP%PHAS = EL%PHAS
       CALL SETFAMILY(ELP)
    ENDIF

    IF(EL%KIND==KINDUSER1) THEN         !
       CALL SETFAMILY(ELP)
       CALL COPY(EL%U1,ELP%U1)
    ENDIF

    IF(EL%KIND==KINDUSER2) THEN         !
       CALL SETFAMILY(ELP)
       CALL COPY(EL%U2,ELP%U2)
    ENDIF
    IF(EL%KIND==KINDWIGGLER) THEN         !
       CALL SETFAMILY(ELP)
       CALL COPY(EL%WI,ELP%WI)
    ENDIF
    IF(EL%KIND==KINDPA) THEN         !
       CALL SETFAMILY(ELP,EL%PA%B,EL%PA%ax,EL%PA%ay)
       CALL COPY(EL%PA,ELP%PA)
    ENDIF
    IF(ASSOCIATED(EL%PARENT_FIBRE))        then
       ELP%PARENT_FIBRE=>EL%PARENT_FIBRE
    ENDIF


  END SUBROUTINE copy_el_elp





  SUBROUTINE copy_elp_el(ELP,EL)
    IMPLICIT NONE
    TYPE(ELEMENTP),INTENT(IN)::  EL
    TYPE(ELEMENT),INTENT(inOUT)::  ELP
    INTEGER I,J,k,l

    ELP%PERMFRINGE=EL%PERMFRINGE
    ELP%NAME=EL%NAME
    ELP%vorname=EL%vorname
    ELP%KIND=EL%KIND
    ELP%L=EL%L
    ELP%FINT=EL%FINT
    ELP%HGAP=EL%HGAP
    ELP%H1=EL%H1
    ELP%H2=EL%H2

    IF(EL%P%NMUL>0) THEN
       IF(EL%P%NMUL/=ELP%P%NMUL.and.ELP%P%NMUL/=0) THEN
          DEALLOCATE(ELP%AN );DEALLOCATE(ELP%BN )
       endif
       if(.not.ASSOCIATED(ELP%AN)) THEN
          ALLOCATE(ELP%AN(EL%P%NMUL),ELP%BN(EL%P%NMUL))
       ENDIF

       DO I=1,EL%P%NMUL
          ELP%AN(I) = EL%AN(I)
          ELP%BN(I) = EL%BN(I)
       ENDDO

    ENDIF
    ELP%P=EL%P



    ! MISALIGNMENTS
    ELP%MIS=EL%MIS
    ELP%EXACTMIS=EL%EXACTMIS

    IF(ASSOCIATED(EL%R)) THEN
       if(.not.ASSOCIATED(ELP%R))  ALLOCATE(ELP%R(3))

       DO I=1,3
          ELP%R(I)=EL%R(I)
       ENDDO
    ENDIF
    IF(ASSOCIATED(EL%D)) THEN
       if(.not.ASSOCIATED(ELP%D))  ALLOCATE(ELP%D(3))

       DO I=1,3
          ELP%D(I)=EL%D(I)
       ENDDO
    ENDIF

    IF(EL%KIND==KIND1) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND2) CALL SETFAMILY(ELP)
    IF(EL%KIND==KIND16.OR.EL%KIND==KIND20) THEN
       CALL SETFAMILY(ELP)
       ELP%K16%DRIFTKICK=EL%K16%DRIFTKICK
       ELP%K16%LIKEMAD=EL%K16%LIKEMAD
    ENDIF

    IF(EL%KIND==KIND3) THEN
       if(.not.ASSOCIATED(ELP%K3)) ALLOCATE(ELP%K3)
       ELP%K3=0
       CALL SETFAMILY(ELP)
       ELP%K3%thin_h_foc=EL%K3%thin_h_foc
       ELP%K3%thin_v_foc=EL%K3%thin_v_foc
       ELP%K3%thin_h_angle=EL%K3%thin_h_angle
       ELP%K3%thin_v_angle=EL%K3%thin_v_angle
    ENDIF


    IF(EL%KIND==KIND4) THEN         !
       if(.not.ASSOCIATED(ELP%C4)) ALLOCATE(ELP%C4)
       ELP%C4=0
       if(.not.ASSOCIATED(ELP%VOLT)) ALLOCATE(ELP%VOLT,ELP%FREQ,ELP%PHAS,ELP%DELTA_E       )
       if(.not.ASSOCIATED(ELP%THIN)) ALLOCATE(ELP%THIN       )
       ELP%VOLT = EL%VOLT
       ELP%FREQ = EL%FREQ
       ELP%PHAS = EL%PHAS
       ELP%DELTA_E = EL%DELTA_E
       ELP%THIN = EL%THIN
       CALL SETFAMILY(ELP)
       ELP%C4%N_BESSEL = EL%C4%N_BESSEL
    ENDIF

    IF(EL%KIND==KIND21) THEN         !
       if(.not.ASSOCIATED(ELP%CAV21)) ALLOCATE(ELP%CAV21)
       ELP%CAV21=0
       if(.not.ASSOCIATED(ELP%VOLT)) ALLOCATE(ELP%VOLT,ELP%FREQ,ELP%PHAS,ELP%DELTA_E       )
       if(.not.ASSOCIATED(ELP%THIN)) ALLOCATE(ELP%THIN       )
       ELP%VOLT = EL%VOLT
       ELP%FREQ = EL%FREQ
       ELP%PHAS = EL%PHAS
       ELP%DELTA_E = EL%DELTA_E
       ELP%THIN = EL%THIN
       CALL SETFAMILY(ELP)
       ELP%CAV21%PSI = EL%CAV21%PSI
       ELP%CAV21%DPHAS = EL%CAV21%DPHAS
    ENDIF

    IF(EL%KIND==KIND5) THEN         !
       if(.not.ASSOCIATED(ELP%S5)) ALLOCATE(ELP%S5)
       if(.not.ASSOCIATED(ELP%B_SOL)) ALLOCATE(ELP%B_SOL       )
       ELP%B_SOL = EL%B_SOL
       CALL SETFAMILY(ELP)
    ENDIF

    IF(EL%KIND==KIND17) THEN         !
       !       if(.not.ASSOCIATED(ELP%S17)) ALLOCATE(ELP%S17)
       if(.not.ASSOCIATED(ELP%B_SOL)) ALLOCATE(ELP%B_SOL       )
       ELP%B_SOL = EL%B_SOL
       CALL SETFAMILY(ELP)
    ENDIF

    IF(EL%KIND==KIND6) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND22) THEN
       i=0;j=0;k=0;l=0;
       if(associated(EL%M22%T_REV)) i=EL%M22%T_REV%N
       if(associated(EL%M22%T_rad_REV)) j=EL%M22%T_rad_REV%N
       if(associated(EL%M22%T)) k=EL%M22%T%N
       if(associated(EL%M22%T_rad)) l=EL%M22%T_rad%N
       CALL SETFAMILY(ELP,NTOT=k,ntot_rad=l,NTOT_REV=i,ntot_rad_REV=j,ND2=6)
       ELP%M22%DELTAMAP=EL%M22%DELTAMAP

       if(associated(EL%M22%T))  CALL COPY_TREE(EL%M22%T,ELP%M22%T)
       if(associated(EL%M22%T_rad)) CALL COPY_TREE(EL%M22%T_rad,ELP%M22%T_rad)
       if(associated(EL%M22%T_REV)) CALL COPY_TREE(EL%M22%T_REV,ELP%M22%T_REV)
       if(associated(EL%M22%T_rad_REV)) CALL COPY_TREE(EL%M22%T_rad_REV,ELP%M22%T_rad_REV)
    ENDIF

    IF(EL%KIND==KIND7) THEN         !
       GEN=.FALSE.
       CALL SETFAMILY(ELP)
       IF(.NOT.GEN) THEN !.NOT.GEN
          DO J=1,3
             ELP%T7%LX(J)=EL%T7%LX(J)
             ELP%T7%RLX(J)=EL%T7%RLX(J)
             DO I=1,2
                ELP%T7%MATX(I,J)=EL%T7%MATX(I,J)
                ELP%T7%MATY(I,J)=EL%T7%MATY(I,J)
                ELP%T7%RMATX(I,J)=EL%T7%RMATX(I,J)
                ELP%T7%RMATY(I,J)=EL%T7%RMATY(I,J)
             ENDDO
          ENDDO
       ENDIF !.NOT.GEN
       GEN=.TRUE.
    ENDIF


    IF(EL%KIND==KIND8) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND9) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND10) THEN
       CALL SETFAMILY(ELP)
       ELP%TP10%DRIFTKICK=EL%TP10%DRIFTKICK
    ENDIF

    IF(EL%KIND>=KIND11.AND.EL%KIND<=KIND14) THEN
       CALL SETFAMILY(ELP)
       ELP%MON14%X=EL%MON14%X
       ELP%MON14%Y=EL%MON14%Y
    ENDIF

    IF(EL%KIND==KIND18) THEN
       CALL SETFAMILY(ELP)
       ELP%RCOL18%A=EL%RCOL18%A
    ENDIF

    IF(EL%KIND==KIND19) THEN
       CALL SETFAMILY(ELP)
       ELP%ECOL19%A=EL%ECOL19%A
    ENDIF

    IF(EL%KIND==KIND15) THEN         !
       if(.not.ASSOCIATED(ELP%SEP15)) ALLOCATE(ELP%SEP15)
       if(.not.ASSOCIATED(ELP%VOLT)) ALLOCATE(ELP%VOLT)
       if(.not.ASSOCIATED(ELP%PHAS)) ALLOCATE(ELP%PHAS)
       ELP%VOLT = EL%VOLT
       ELP%PHAS = EL%PHAS
       CALL SETFAMILY(ELP)
    ENDIF

    IF(EL%KIND==KINDUSER1) THEN         !
       CALL SETFAMILY(ELP)
       CALL COPY(EL%U1,ELP%U1)
    ENDIF

    IF(EL%KIND==KINDUSER2) THEN         !
       CALL SETFAMILY(ELP)
       CALL COPY(EL%U2,ELP%U2)
    ENDIF

    IF(EL%KIND==KINDWIGGLER) THEN         !
       CALL SETFAMILY(ELP)
       CALL COPY(EL%WI,ELP%WI)
    ENDIF
    IF(EL%KIND==KINDPA) THEN         !
       CALL SETFAMILY(ELP,EL%PA%B,EL%PA%ax,EL%PA%ay)
       CALL COPY(EL%PA,ELP%PA)
    ENDIF

    IF(ASSOCIATED(EL%PARENT_FIBRE))        then
       ELP%PARENT_FIBRE=>EL%PARENT_FIBRE
    ENDIF


  END SUBROUTINE copy_elp_el



  SUBROUTINE copy_el_el(ELP,EL)
    IMPLICIT NONE
    TYPE(ELEMENT),INTENT(IN)::  EL
    TYPE(ELEMENT),INTENT(inOUT)::  ELP
    INTEGER I,J,k,l


    ELP%PERMFRINGE=EL%PERMFRINGE
    ELP%NAME=EL%NAME
    ELP%vorname=EL%vorname
    ELP%KIND=EL%KIND
    ELP%L=EL%L
    ELP%FINT=EL%FINT
    ELP%HGAP=EL%HGAP
    ELP%H1=EL%H1
    ELP%H2=EL%H2


    IF(EL%P%NMUL>0) THEN
       IF(EL%P%NMUL/=ELP%P%NMUL.and.ELP%P%NMUL/=0) THEN
          DEALLOCATE(ELP%AN );DEALLOCATE(ELP%BN )
       endif
       if(.not.ASSOCIATED(ELP%AN)) THEN
          ALLOCATE(ELP%AN(EL%P%NMUL),ELP%BN(EL%P%NMUL))
       ENDIF

       DO I=1,EL%P%NMUL
          ELP%AN(I) = EL%AN(I)
          ELP%BN(I) = EL%BN(I)
       ENDDO

    ENDIF
    ELP%P=EL%P



    ! MISALIGNMENTS
    ELP%MIS=EL%MIS
    ELP%EXACTMIS=EL%EXACTMIS

    IF(ASSOCIATED(EL%R)) THEN
       if(.not.ASSOCIATED(ELP%R))  ALLOCATE(ELP%R(3))
       DO I=1,3
          ELP%R(I)=EL%R(I)
       ENDDO
    ENDIF
    IF(ASSOCIATED(EL%D)) THEN
       if(.not.ASSOCIATED(ELP%D))  ALLOCATE(ELP%D(3))
       DO I=1,3
          ELP%D(I)=EL%D(I)
       ENDDO
    ENDIF

    IF(EL%KIND==KIND1) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND2) CALL SETFAMILY(ELP)
    IF(EL%KIND==KIND16.OR.EL%KIND==KIND20) THEN
       CALL SETFAMILY(ELP)
       ELP%K16%DRIFTKICK=EL%K16%DRIFTKICK
       ELP%K16%LIKEMAD=EL%K16%LIKEMAD
    ENDIF

    IF(EL%KIND==KIND3) THEN
       if(.not.ASSOCIATED(ELP%K3)) ALLOCATE(ELP%K3)
       ELP%K3=0
       CALL SETFAMILY(ELP)
       ELP%K3%thin_h_foc=EL%K3%thin_h_foc
       ELP%K3%thin_v_foc=EL%K3%thin_v_foc
       ELP%K3%thin_h_angle=EL%K3%thin_h_angle
       ELP%K3%thin_v_angle=EL%K3%thin_v_angle
    ENDIF


    IF(EL%KIND==KIND4) THEN         !
       if(.not.ASSOCIATED(ELP%C4)) ALLOCATE(ELP%C4)
       ELP%C4=0
       if(.not.ASSOCIATED(ELP%VOLT)) ALLOCATE(ELP%VOLT,ELP%FREQ,ELP%PHAS,ELP%DELTA_E       )
       if(.not.ASSOCIATED(ELP%THIN)) ALLOCATE(ELP%THIN       )
       ELP%VOLT = EL%VOLT
       ELP%FREQ = EL%FREQ
       ELP%PHAS = EL%PHAS
       ELP%DELTA_E = EL%DELTA_E
       ELP%THIN = EL%THIN
       CALL SETFAMILY(ELP)
       ELP%C4%N_BESSEL = EL%C4%N_BESSEL
    ENDIF

    IF(EL%KIND==KIND21) THEN         !
       if(.not.ASSOCIATED(ELP%CAV21)) ALLOCATE(ELP%CAV21)
       ELP%CAV21=0
       if(.not.ASSOCIATED(ELP%VOLT)) ALLOCATE(ELP%VOLT,ELP%FREQ,ELP%PHAS,ELP%DELTA_E       )
       if(.not.ASSOCIATED(ELP%THIN)) ALLOCATE(ELP%THIN       )
       ELP%VOLT = EL%VOLT
       ELP%FREQ = EL%FREQ
       ELP%PHAS = EL%PHAS
       ELP%DELTA_E = EL%DELTA_E
       ELP%THIN = EL%THIN
       CALL SETFAMILY(ELP)
       ELP%CAV21%PSI = EL%CAV21%PSI
       ELP%CAV21%DPHAS = EL%CAV21%DPHAS
    ENDIF

    IF(EL%KIND==KIND5) THEN         !
       if(.not.ASSOCIATED(ELP%S5)) ALLOCATE(ELP%S5)
       if(.not.ASSOCIATED(ELP%B_SOL)) ALLOCATE(ELP%B_SOL       )
       ELP%B_SOL = EL%B_SOL
       CALL SETFAMILY(ELP)
    ENDIF

    IF(EL%KIND==KIND17) THEN         !
       !      if(.not.ASSOCIATED(ELP%S17)) ALLOCATE(ELP%S17)
       if(.not.ASSOCIATED(ELP%B_SOL)) ALLOCATE(ELP%B_SOL       )
       ELP%B_SOL = EL%B_SOL
       CALL SETFAMILY(ELP)
    ENDIF

    IF(EL%KIND==KIND6) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND22) THEN
       i=0;j=0;k=0;l=0;
       if(associated(EL%M22%T_REV)) i=EL%M22%T_REV%N
       if(associated(EL%M22%T_rad_REV)) j=EL%M22%T_rad_REV%N
       if(associated(EL%M22%T)) k=EL%M22%T%N
       if(associated(EL%M22%T_rad)) l=EL%M22%T_rad%N
       CALL SETFAMILY(ELP,NTOT=k,ntot_rad=l,NTOT_REV=i,ntot_rad_REV=j,ND2=6)
       ELP%M22%DELTAMAP=EL%M22%DELTAMAP

       if(associated(EL%M22%T))  CALL COPY_TREE(EL%M22%T,ELP%M22%T)
       if(associated(EL%M22%T_rad)) CALL COPY_TREE(EL%M22%T_rad,ELP%M22%T_rad)
       if(associated(EL%M22%T_REV)) CALL COPY_TREE(EL%M22%T_REV,ELP%M22%T_REV)
       if(associated(EL%M22%T_rad_REV)) CALL COPY_TREE(EL%M22%T_rad_REV,ELP%M22%T_rad_REV)
    ENDIF

    IF(EL%KIND==KIND7) THEN         !
       GEN=.FALSE.
       CALL SETFAMILY(ELP)
       IF(.NOT.GEN) THEN !.NOT.GEN
          DO J=1,3
             ELP%T7%LX(J)=EL%T7%LX(J)
             ELP%T7%RLX(J)=EL%T7%RLX(J)
             DO I=1,2
                ELP%T7%MATX(I,J)=EL%T7%MATX(I,J)
                ELP%T7%MATY(I,J)=EL%T7%MATY(I,J)
                ELP%T7%RMATX(I,J)=EL%T7%RMATX(I,J)
                ELP%T7%RMATY(I,J)=EL%T7%RMATY(I,J)
             ENDDO
          ENDDO
       ENDIF !.NOT.GEN
       GEN=.TRUE.
    ENDIF


    IF(EL%KIND==KIND8) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND9) CALL SETFAMILY(ELP)

    IF(EL%KIND==KIND10) THEN
       CALL SETFAMILY(ELP)
       ELP%TP10%DRIFTKICK=EL%TP10%DRIFTKICK
    ENDIF

    IF(EL%KIND>=KIND11.AND.EL%KIND<=KIND14) THEN
       CALL SETFAMILY(ELP)
       ELP%MON14%X=EL%MON14%X
       ELP%MON14%Y=EL%MON14%Y
    ENDIF

    IF(EL%KIND==KIND18) THEN
       CALL SETFAMILY(ELP)
       ELP%RCOL18%A=EL%RCOL18%A
    ENDIF

    IF(EL%KIND==KIND19) THEN
       CALL SETFAMILY(ELP)
       ELP%ECOL19%A=EL%ECOL19%A
    ENDIF

    IF(EL%KIND==KIND15) THEN         !
       if(.not.ASSOCIATED(ELP%SEP15)) ALLOCATE(ELP%SEP15)
       if(.not.ASSOCIATED(ELP%VOLT)) ALLOCATE(ELP%VOLT)
       if(.not.ASSOCIATED(ELP%PHAS)) ALLOCATE(ELP%PHAS)
       ELP%VOLT = EL%VOLT
       ELP%PHAS = EL%PHAS
       CALL SETFAMILY(ELP)
    ENDIF

    IF(EL%KIND==KINDUSER1) THEN         !
       CALL SETFAMILY(ELP)
       CALL COPY(EL%U1,ELP%U1)
    ENDIF

    IF(EL%KIND==KINDUSER2) THEN         !
       CALL SETFAMILY(ELP)
       CALL COPY(EL%U2,ELP%U2)
    ENDIF

    IF(EL%KIND==KINDWIGGLER) THEN         !
       CALL SETFAMILY(ELP)
       CALL COPY(EL%WI,ELP%WI)
    ENDIF
    IF(EL%KIND==KINDPA) THEN         !
       CALL SETFAMILY(ELP,EL%PA%B,EL%PA%ax,EL%PA%ay)
       CALL COPY(EL%PA,ELP%PA)
    ENDIF

    IF(ASSOCIATED(EL%PARENT_FIBRE))        then
       ELP%PARENT_FIBRE=>EL%PARENT_FIBRE
    ENDIF


  END SUBROUTINE copy_el_el


  SUBROUTINE reset31(ELP)
    IMPLICIT NONE
    TYPE(ELEMENTP),INTENT(inOUT)::  ELP
    INTEGER I

    ELP%knob=.FALSE.

    CALL resetpoly_R31(ELP%L)         ! SHARED BY EVERYONE
    CALL resetpoly_R31(ELP%FINT)         ! SHARED BY EVERYONE
    CALL resetpoly_R31(ELP%HGAP)         ! SHARED BY EVERYONE
    CALL resetpoly_R31(ELP%H1)         ! SHARED BY EVERYONE
    CALL resetpoly_R31(ELP%H2)         ! SHARED BY EVERYONE

    IF(ELP%P%NMUL>0) THEN             ! SHARED BY A LOT
       DO I=1,ELP%P%NMUL
          CALL resetpoly_R31(ELP%AN(I))
          CALL resetpoly_R31(ELP%BN(I))
       ENDDO
    ENDIF


    IF(ELP%KIND==KIND4) THEN
       CALL resetpoly_R31(ELP%VOLT)
       CALL resetpoly_R31(ELP%FREQ )
       CALL resetpoly_R31(ELP%PHAS )
       !      CALL resetpoly_R31(ELP%P0C )
    ENDIF

    IF(ELP%KIND==KIND3) THEN
       CALL resetpoly_R31(ELP%K3%thin_h_foc)
       CALL resetpoly_R31(ELP%K3%thin_v_foc)
       CALL resetpoly_R31(ELP%K3%thin_h_angle )
       CALL resetpoly_R31(ELP%K3%thin_v_angle)
    ENDIF

    IF(ELP%KIND==KIND21) THEN
       CALL resetpoly_R31(ELP%VOLT)
       CALL resetpoly_R31(ELP%FREQ )
       CALL resetpoly_R31(ELP%PHAS )
       CALL resetpoly_R31(ELP%CAV21%PSI )
       CALL resetpoly_R31(ELP%CAV21%DPHAS )
    ENDIF

    IF(ELP%KIND==KIND15) THEN          ! NEW 2002.11.16
       CALL resetpoly_R31(ELP%VOLT)
       CALL resetpoly_R31(ELP%PHAS )
    ENDIF

    IF(ELP%KIND==KIND5.OR.ELP%KIND==KIND17) THEN
       CALL resetpoly_R31(ELP%B_SOL)
    ENDIF



    IF(ELP%KIND==KINDUSER1) THEN
       CALL reset_U1(ELP%U1)
    ENDIF

    IF(ELP%KIND==KINDUSER2) THEN
       CALL reset_U2(ELP%U2)
    ENDIF

    IF(ELP%KIND==KINDWIGGLER) THEN
       CALL reset_WI(ELP%WI)
    ENDIF

    IF(ELP%KIND==KINDPA) THEN
       CALL reset_PA(ELP%PA)
    ENDIF


  END SUBROUTINE reset31

  SUBROUTINE  find_energy(t,KINETIC,ENERGY,P0C,BRHO,beta0)
    implicit none
    type(work) ,INTENT(INout):: t
    real(dp) XMC2,cl,CU,ERG,beta0i,GAMMA,GAMMA2,CON
    logical(lp) PROTON
    real(dp) KINETIC1,ENERGY1,P0C1,BRHO1,beta01   !  private here
    real(dp), optional ::   KINETIC,ENERGY,P0C,BRHO,beta0   !  private here
    real(dp)  gamma0I,gamBET  ! private here

    kinetic1=zero
    ENERGY1=zero
    beta01=zero
    brho1=zero
    p0c1=zero
    if(present(KINETIC)) kinetic1=-kinetic
    if(present(energy))  energy1=-energy
    if(present(BETa0))   BETa01=-BETa0
    if(present(brho) )    brho1=-brho
    if(present(p0c) )    p0c1=-p0c

    PROTON=.NOT.ELECTRON
    cl=(clight/c_1d8)
    CU=c_55/c_24/SQRT(three)
    w_p=0
    w_p%nc=8
    w_p%fc='(7((1X,A72,/)),1X,A72)'
    if(electron) then
       XMC2=muon*pmae
       w_p%c(1)=" This is an electron "
    elseif(proton) then
       XMC2=pmap
       w_p%c(1)=" This is a proton! "
    endif
    if(ENERGY1<0) then
       ENERGY1=-ENERGY1
       erg=ENERGY1
       p0c1=SQRT(erg**2-xmc2**2)
    endif
    if(kinetic1<0) then
       kinetic1=-kinetic1
       erg=kinetic1+xmc2
       p0c1=SQRT(erg**2-xmc2**2)
    endif
    if(brho1<0) then
       brho1=-brho1
       p0c1=SQRT(brho1**2*(cl/ten)**2)
    endif
    if(BETa01<0) then
       BETa01=-BETa01
       p0c1=xmc2*BETa01/SQRT(one-BETa01**2)
    endif

    if(p0c1<0) then
       p0c1=-p0c1
    endif

    erg=SQRT(p0c1**2+XMC2**2)
    kinetic1=ERG-xmc2
    BETa01=SQRT(kinetic1**2+two*kinetic1*XMC2)/erg
    beta0i=one/BETa01
    GAMMA=erg/XMC2
    write(W_P%C(2),'(A16,G20.14)') ' Kinetic Energy ',kinetic1
    write(W_P%C(3),'(A7,G20.14)') ' gamma ',gamma
    write(W_P%C(4),'(A7,G20.14)')' beta0 ',BETa01
    CON=three*CU*CGAM*HBC/two*TWOPII/XMC2**3
    CRAD=CGAM*ERG**3*TWOPII
    CFLUC=CON*ERG**5
    GAMMA2=erg**2/XMC2**2
    brho1=SQRT(ERG**2-XMC2**2)*ten/cl
    write(W_P%C(5),'(A7,G20.14)') ' p0c = ',p0c1
    write(W_P%C(6),'(A9,G20.14)')' GAMMA = ',SQRT(GAMMA2)
    write(W_P%C(7),'(A8,G20.14)')' BRHO = ',brho1
    write(W_P%C(8),'(A15,G20.14,1X,G20.14)')"CRAD AND CFLUC ", crad ,CFLUC
    IF(VERBOSE) CALL WRITE_I
    !END OF SET RADIATION STUFF  AND TIME OF FLIGHT SUFF
    !    gamma0I=SQRT(one-beta0**2)
    !    gambet =(gamma0I/beta0)**2
    gamma0I=XMC2*BETa01/p0c1
    GAMBET=(XMC2/p0c1)**2

    t%kinetic=kinetic1
    t%energy =ERG
    t%BETa0=BETa01
    t%BRHO=brho1
    t%p0c=p0c1
    t%gamma0I=gamma0I
    t%gambet=gambet
    t%mass=xmc2


  END SUBROUTINE find_energy

  subroutine put_aperture_el(el,kind,r,x,y)
    implicit none
    real(dp),intent(in):: r(:),x,y
    integer,intent(in):: kind
    type(element),intent(inout):: el

    if(.not.associated(el%p%aperture)) call alloc(el%p%aperture)
    el%p%aperture%x=x
    el%p%aperture%y=y
    el%p%aperture%r=r
    el%p%aperture%kind=kind
  end  subroutine put_aperture_el

  subroutine put_aperture_elp(el,kind,r,x,y)
    implicit none
    real(dp),intent(in):: r(:),x,y
    integer,intent(in):: kind
    type(elementp),intent(inout):: el

    if(.not.associated(el%p%aperture)) call alloc(el%p%aperture)
    el%p%aperture%x=x
    el%p%aperture%y=y
    el%p%aperture%r=r
    el%p%aperture%kind=kind
  end  subroutine put_aperture_elp

  subroutine remove_aperture_el(el)
    implicit none
    type(element),intent(inout):: el

    if(associated(el%p%aperture)) then
       CALL kill(el%p%APERTURE)
       DEALLOCATE(el%p%APERTURE);
    endif
  end  subroutine remove_aperture_el

  subroutine remove_aperture_elp(el)
    implicit none
    type(elementp),intent(inout):: el

    if(associated(el%p%aperture)) then
       CALL kill(el%p%APERTURE)
       DEALLOCATE(el%p%APERTURE);
    endif
  end  subroutine remove_aperture_elp


END MODULE S_DEF_ELEMENT
