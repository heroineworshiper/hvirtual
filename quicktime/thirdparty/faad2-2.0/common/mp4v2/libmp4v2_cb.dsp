# Microsoft Developer Studio Project File - Name="libmp4v2_cb" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libmp4v2_cb - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libmp4v2_cb.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libmp4v2_cb.mak" CFG="libmp4v2_cb - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libmp4v2_cb - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libmp4v2_cb - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "libmp4v2_cb - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "CB_Release"
# PROP Intermediate_Dir "CB_Release"
# PROP Target_Dir ""
MTL=midl.exe
F90=df.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_FILE_CALLBACKS" /YX /FD /c
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libmp4v2_cb - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "CB_Debug"
# PROP Intermediate_Dir "CB_Debug"
# PROP Target_Dir ""
MTL=midl.exe
F90=df.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_FILE_CALLBACKS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libmp4v2_cb - Win32 Release"
# Name "libmp4v2_cb - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\atom_co64.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_cprt.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_ctts.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_dimm.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_dinf.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_dmax.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_dmed.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_dref.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_drep.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_edts.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_elst.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_enca.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_encv.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_esds.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_free.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_frma.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_ftyp.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_hdlr.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_hinf.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_hmhd.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_hnti.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_iKMS.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_iods.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_iSFM.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_maxr.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_mdat.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_mdhd.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_mdia.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_meta.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_mfhd.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_minf.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_moof.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_moov.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_mp4a.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_mp4s.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_mp4v.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_mvex.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_mvhd.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_nmhd.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_nump.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_payt.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_pmax.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_root.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_rtp.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_schi.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_schm.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_sdp.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_sinf.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_smhd.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_snro.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_stbl.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_stco.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_stdp.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_stsc.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_stsd.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_stsh.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_stss.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_stsz.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_stts.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_tfhd.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_tims.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_tkhd.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_tmax.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_tmin.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_tpyl.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_traf.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_trak.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_tref.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_treftype.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_trex.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_trpy.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_trun.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_tsro.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_udta.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_url.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_urn.cpp
# End Source File
# Begin Source File

SOURCE=.\atom_vmhd.cpp
# End Source File
# Begin Source File

SOURCE=.\descriptors.cpp
# End Source File
# Begin Source File

SOURCE=.\isma.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4atom.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4container.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4descriptor.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4file.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4file_io.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4info.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4meta.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4property.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4track.cpp
# End Source File
# Begin Source File

SOURCE=.\mp4util.cpp
# End Source File
# Begin Source File

SOURCE=.\need_for_win32.c
# End Source File
# Begin Source File

SOURCE=.\ocidescriptors.cpp
# End Source File
# Begin Source File

SOURCE=.\odcommands.cpp
# End Source File
# Begin Source File

SOURCE=.\qosqualifiers.cpp
# End Source File
# Begin Source File

SOURCE=.\rtphint.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\acconfig.h
# End Source File
# Begin Source File

SOURCE=.\atoms.h
# End Source File
# Begin Source File

SOURCE=.\descriptors.h
# End Source File
# Begin Source File

SOURCE=.\mp4.h
# End Source File
# Begin Source File

SOURCE=.\mp4array.h
# End Source File
# Begin Source File

SOURCE=.\mp4atom.h
# End Source File
# Begin Source File

SOURCE=.\mp4common.h
# End Source File
# Begin Source File

SOURCE=.\mp4container.h
# End Source File
# Begin Source File

SOURCE=.\mp4descriptor.h
# End Source File
# Begin Source File

SOURCE=.\mp4file.h
# End Source File
# Begin Source File

SOURCE=.\mp4property.h
# End Source File
# Begin Source File

SOURCE=.\mp4track.h
# End Source File
# Begin Source File

SOURCE=.\mp4util.h
# End Source File
# Begin Source File

SOURCE=.\mpeg4ip.h
# End Source File
# Begin Source File

SOURCE=.\ocidescriptors.h
# End Source File
# Begin Source File

SOURCE=.\odcommands.h
# End Source File
# Begin Source File

SOURCE=.\qosqualifiers.h
# End Source File
# Begin Source File

SOURCE=.\rtphint.h
# End Source File
# Begin Source File

SOURCE=.\systems.h
# End Source File
# Begin Source File

SOURCE=.\win32_ver.h
# End Source File
# End Group
# End Target
# End Project
