# Clinton James clinton.james@jidn.com March 2002
!IF "$(CFG)" != "dll" && "$(CFG)" != "dll debug" && "$(CFG)" != "static" && "$(CFG)" != "static debug"
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f libpqxx.mak configuration
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RELEASE"      (release Win32 (x86) Dynamic-Link Library and Static Library)
!MESSAGE "DEBUG"        (debug   Win32 (x86) Dynamic-Link Library and Static Library)
!MESSAGE "DLL"          (release & debug Win32 (x86) Dynamic-Link Library)
!MESSAGE "STATIC"       (release & debug Win32 (x86) Static Library")
!MESSAGE "ALL"          Both the DLL and STATIC builds.
!MESSAGE
!ENDIF 

!include common

OUTDIR=lib
INTDIR=obj
LINK32_OBJ_EXTRA=

# Defining extra CCP and LINK options for the four different builds
!IF  "$(CFG)" == "dll"
BUILDTYPE=DLL
OUTFILE=$(OUTDIR)\libpqxx
CPP_EXTRAS=/MT /D "_WINDOWS" /D "_USRDLL" /D "NDEBUG"  
LINK32_FLAG_EXTRA=/incremental:no 
LINK32_OBJ_EXTRA="$(INTDIR)\libpqxx.obj"

!ELSEIF  "$(CFG)" == "dll debug"
BUILDTYPE=DLL
OUTFILE=$(OUTDIR)\libpqxxD
CPP_EXTRAS=/MTd /Gm /ZI /Od /D "_WINDOWS" /D "_USRDLL" /D "_DEBUG" /D "NO_PARTIAL_CLASS_TEMPLATE_SPECIALISATION" /GZ
LINK32_FLAG_EXTRA=/incremental:yes /debug 
LINK32_OBJ_EXTRA="$(INTDIR)\libpqxx.obj"

!ELSEIF "$(CFG)" == "static"
BUILDTYPE=STATIC
OUTFILE=$(OUTDIR)\libpqxx_static
CPP_EXTRAS=/MT /D "_LIB" /D "NDEBUG" /D "NO_PARTIAL_CLASS_TEMPLATE_SPECIALISATION"

!ELSEIF "$(CFG)" == "static debug"
BUILDTYPE=STATIC
OUTFILE=$(OUTDIR)\libpqxx_staticD
CPP_EXTRAS=/MTd /Gm /ZI /Od /D "_LIB" /D "_DEBUG" /D "NO_PARTIAL_CLASS_TEMPLATE_SPECIALISATION" /GZ

!ENDIF

IAM=$(MAKEDIR)\libpqxx.mak /NOLOGO
# Putting the extra options together with the options common to all the
# different builds.
CPP=cl.exe
CPP_PROJ=/nologo /W3 /GX /FD /c $(CPP_EXTRAS) \
	/I "../include" /I "$(PGSQLSRC)/include" /I "$(PGSQLSRC)/interfaces/libpq" \
	$(STD) /D "HAVE_VSNPRINTF_DECL" /D "HAVE_STRDUP" \
	/D "WIN32" /D "_MBCS" /D "LIBPQXXDLL_EXPORTS" \
	/Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" \

LINK32=link.exe 
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib \
	shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib \
	/nologo /dll /machine:I386 /out:"$(OUTFILE).dll" /implib:"$(OUTFILE).lib" \
	$(LINK32_FLAG_EXTRA) $(LIBPATH)

LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTFILE).lib" 

LINK32_OBJS= $(LINK32_OBJ_EXTRA) \
	"$(INTDIR)\connection.obj" \
	"$(INTDIR)\cursor.obj" \
	"$(INTDIR)\nontransaction.obj" \
	"$(INTDIR)\result.obj" \
	"$(INTDIR)\robusttransaction.obj" \
	"$(INTDIR)\tablereader.obj" \
	"$(INTDIR)\tablestream.obj" \
	"$(INTDIR)\tablewriter.obj" \
	"$(INTDIR)\transaction.obj" \
	"$(INTDIR)\transactionitf.obj" 
	
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\libpqxx.res"

RELEASE :
	@$(MAKE) /f $(IAM) CFG="dll" LIBPQXXDLL 
	@$(MAKE) /f $(IAM) CFG="static" LIBPQXXSTATIC 

DEBUG :
	@$(MAKE) /f $(IAM) CFG="dll debug" LIBPQXXDLL 
	@$(MAKE) /f $(IAM) CFG="static debug" LIBPQXXSTATIC 

DLL :
	@$(MAKE) /f $(IAM) CFG="dll" LIBPQXXDLL 
	@$(MAKE) /f $(IAM) CFG="dll debug" LIBPQXXDLL 

STATIC:
	@$(MAKE) /f $(IAM) CFG="static" LIBPQXXSTATIC 
	@$(MAKE) /f $(IAM) CFG="static debug" LIBPQXXSTATIC 

ALL :
	@$(MAKE) /f $(IAM) CFG="dll" LIBPQXXDLL 
	@$(MAKE) /f $(IAM) CFG="dll debug" LIBPQXXDLL 
	@$(MAKE) /f $(IAM) CFG="static" LIBPQXXSTATIC 
	@$(MAKE) /f $(IAM) CFG="static debug" LIBPQXXSTATIC 

CLEAN :
	@echo Deleting all files from $(INTDIR) and $(OUTDIR).
	-@erase "$(INTDIR)" /Q
	-@erase "$(OUTDIR)" /Q

!IF "$(CFG)" == "dll" || "$(CFG)" == "dll debug" || "$(CFG)" == "static" || "$(CFG)" == "static debug"
!MESSAGE
!MESSAGE Building $(CFG)

LIBPQXXDLL : "$(OUTFILE).dll"

"$(OUTFILE).dll" : "$(OUTDIR)" $(LINK32_OBJS)
    @$(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
!IF "$(CFG)" == "dll debug"
   -@erase "$(OUTFILE).ilk"
   -@erase "$(OUTFILE).pdb"
!ENDIF
   -@erase "$(OUTFILE).exp"
   -@erase "$(INTDIR)\*.?db"
   -@erase "$(INTDIR)\*.obj"

LIBPQXXSTATIC : "$(OUTFILE).lib"

"$(OUTFILE).lib" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    @$(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LINK32_OBJS)
<<
	-@erase "$(INTDIR)\*.obj"
	-@erase "$(INTDIR)\*.?db"
	
"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
	if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"
	
# Do we want a resource to go with the dll's?  If so what?    
#"$(INTDIR)\libpqxx.res" : "$(INTDIR)" libpqxx.rc
#    $(RSC) $(RSC_PROJ) libpqxx.rc

SOURCE=..\src\connection.cxx
"$(INTDIR)\connection.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\src\cursor.cxx
"$(INTDIR)\cursor.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=.\libpqxx.cxx
"$(INTDIR)\libpqxx.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\src\nontransaction.cxx
"$(INTDIR)\nontransaction.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\src\result.cxx
"$(INTDIR)\result.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\src\robusttransaction.cxx
"$(INTDIR)\robusttransaction.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\src\tablereader.cxx
"$(INTDIR)\tablereader.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\src\tablestream.cxx
"$(INTDIR)\tablestream.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\src\tablewriter.cxx
"$(INTDIR)\tablewriter.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\src\transaction.cxx
"$(INTDIR)\transaction.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\src\transactionitf.cxx
"$(INTDIR)\transactionitf.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

!ENDIF 

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


