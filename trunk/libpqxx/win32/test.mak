# AUTOMATICALLY GENERATED--DO NOT EDIT
# This file is generated automatically for automake whenever test programs are
# added to libpqxx, using the Perl script "maketestvcmak.pl" found in the tools
# directory.
#
# Contents of this file based on the original test.mak by Clinton James:
# Clinton James clinton.james@jidn.com (March 2002)

!IF "$(CFG)" != "Release" && "$(CFG)" != "Debug"
!MESSAGE You can specify a specific testcase when running NMAKE. For example:
!MESSAGE     NMAKE /f "test.mak" testcase
!MESSAGE Possible choices for testcase are TEST001 through TEST059 or ALL
!MESSAGE
CFG=Release
!ENDIF

!include common
OUTDIR=./lib
INTDIR=./obj

!IF  "$(CFG)" == "Release"
CPP_EXTRA=/MT /D "NDEBUG"
LINK32_FLAG_LIB=libpqxx.lib
LINK32_FLAG_EXTRA=/incremental:no

!ELSEIF  "$(CFG)" == "Debug"
CPP_EXTRA=/MTd /Gm /GZ /Zi /Od /D "_DEBUG"
LINK32_FLAG_LIB=libpqxxD.lib
LINK32_FLAG_EXTRA=/incremental:yes /debug /pdbtype:sept

!ENDIF

CPP=cl.exe
CPP_PROJ=/nologo $(CPP_EXTRA) /W3 /GX /FD /c \
	/D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_VSNPRINTF_DECL" $(STD) \
	/I "../include" /I "$(PGSQLSRC)/include" /I "$(PGSQLSRC)/interfaces/libpq" \
	/YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"

LINK32=link.exe
LINK32_FLAGS=$(LINK32_FLAG_LIB) kernel32.lib user32.lib gdi32.lib \
        winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib \
        oleaut32.lib uuid.lib odbc32.lib odbccp32.lib \
        /nologo /subsystem:console /machine:I386 \
        $(LINK32_FLAG_EXTRA) $(LIBPATH) /libpath:"lib"

TEST001: "$(OUTDIR)\test001.exe"
TEST002: "$(OUTDIR)\test002.exe"
TEST003: "$(OUTDIR)\test003.exe"
TEST004: "$(OUTDIR)\test004.exe"
TEST005: "$(OUTDIR)\test005.exe"
TEST006: "$(OUTDIR)\test006.exe"
TEST007: "$(OUTDIR)\test007.exe"
TEST008: "$(OUTDIR)\test008.exe"
TEST009: "$(OUTDIR)\test009.exe"
TEST010: "$(OUTDIR)\test010.exe"
TEST011: "$(OUTDIR)\test011.exe"
TEST012: "$(OUTDIR)\test012.exe"
TEST013: "$(OUTDIR)\test013.exe"
TEST014: "$(OUTDIR)\test014.exe"
TEST015: "$(OUTDIR)\test015.exe"
TEST016: "$(OUTDIR)\test016.exe"
TEST017: "$(OUTDIR)\test017.exe"
TEST018: "$(OUTDIR)\test018.exe"
TEST019: "$(OUTDIR)\test019.exe"
TEST020: "$(OUTDIR)\test020.exe"
TEST021: "$(OUTDIR)\test021.exe"
TEST022: "$(OUTDIR)\test022.exe"
TEST023: "$(OUTDIR)\test023.exe"
TEST024: "$(OUTDIR)\test024.exe"
TEST025: "$(OUTDIR)\test025.exe"
TEST026: "$(OUTDIR)\test026.exe"
TEST027: "$(OUTDIR)\test027.exe"
TEST028: "$(OUTDIR)\test028.exe"
TEST029: "$(OUTDIR)\test029.exe"
TEST030: "$(OUTDIR)\test030.exe"
TEST031: "$(OUTDIR)\test031.exe"
TEST032: "$(OUTDIR)\test032.exe"
TEST033: "$(OUTDIR)\test033.exe"
TEST034: "$(OUTDIR)\test034.exe"
TEST035: "$(OUTDIR)\test035.exe"
TEST036: "$(OUTDIR)\test036.exe"
TEST037: "$(OUTDIR)\test037.exe"
TEST038: "$(OUTDIR)\test038.exe"
TEST039: "$(OUTDIR)\test039.exe"
TEST040: "$(OUTDIR)\test040.exe"
TEST041: "$(OUTDIR)\test041.exe"
TEST042: "$(OUTDIR)\test042.exe"
TEST043: "$(OUTDIR)\test043.exe"
TEST044: "$(OUTDIR)\test044.exe"
TEST045: "$(OUTDIR)\test045.exe"
TEST046: "$(OUTDIR)\test046.exe"
TEST047: "$(OUTDIR)\test047.exe"
TEST048: "$(OUTDIR)\test048.exe"
TEST049: "$(OUTDIR)\test049.exe"
TEST050: "$(OUTDIR)\test050.exe"
TEST051: "$(OUTDIR)\test051.exe"
TEST052: "$(OUTDIR)\test052.exe"
TEST053: "$(OUTDIR)\test053.exe"
TEST054: "$(OUTDIR)\test054.exe"
TEST055: "$(OUTDIR)\test055.exe"
TEST056: "$(OUTDIR)\test056.exe"
TEST057: "$(OUTDIR)\test057.exe"
TEST058: "$(OUTDIR)\test058.exe"
TEST059: "$(OUTDIR)\test059.exe"
TEST060: "$(OUTDIR)\test060.exe"
TEST061: "$(OUTDIR)\test061.exe"
TEST062: "$(OUTDIR)\test062.exe"
TEST063: "$(OUTDIR)\test063.exe"
TEST064: "$(OUTDIR)\test064.exe"
TEST065: "$(OUTDIR)\test065.exe"
TEST066: "$(OUTDIR)\test066.exe"
TEST067: "$(OUTDIR)\test067.exe"
TEST068: "$(OUTDIR)\test068.exe"
TEST069: "$(OUTDIR)\test069.exe"
TEST070: "$(OUTDIR)\test070.exe"
TEST071: "$(OUTDIR)\test071.exe"
TEST072: "$(OUTDIR)\test072.exe"
TEST073: "$(OUTDIR)\test073.exe"
TEST074: "$(OUTDIR)\test074.exe"
TEST075: "$(OUTDIR)\test075.exe"
TEST076: "$(OUTDIR)\test076.exe"
TEST077: "$(OUTDIR)\test077.exe"
TEST078: "$(OUTDIR)\test078.exe"
TEST079: "$(OUTDIR)\test079.exe"

ALL : TEST001 TEST002 TEST003 TEST004 TEST005 TEST006 TEST007 TEST008 TEST009 TEST010 TEST011 TEST012 TEST013 TEST014 TEST015 TEST016 TEST017 TEST018 TEST019 TEST020 TEST021 TEST022 TEST023 TEST024 TEST025 TEST026 TEST027 TEST028 TEST029 TEST030 TEST031 TEST032 TEST033 TEST034 TEST035 TEST036 TEST037 TEST038 TEST039 TEST040 TEST041 TEST042 TEST043 TEST044 TEST045 TEST046 TEST047 TEST048 TEST049 TEST050 TEST051 TEST052 TEST053 TEST054 TEST055 TEST056 TEST057 TEST058 TEST059 TEST060 TEST061 TEST062 TEST063 TEST064 TEST065 TEST066 TEST067 TEST068 TEST069 TEST070 TEST071 TEST072 TEST073 TEST074 TEST075 TEST076 TEST077 TEST078 TEST079
CLEAN :
         "$(INTDIR)" /Q
        -@erase "$(OUTDIR)\test*.exe" /Q

"$(INTDIR)" :
        if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(OUTDIR)\test001.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test001.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test001.exe" "$(INTDIR)\test001.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test002.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test002.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test002.exe" "$(INTDIR)\test002.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test003.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test003.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test003.exe" "$(INTDIR)\test003.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test004.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test004.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test004.exe" "$(INTDIR)\test004.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test005.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test005.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test005.exe" "$(INTDIR)\test005.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test006.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test006.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test006.exe" "$(INTDIR)\test006.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test007.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test007.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test007.exe" "$(INTDIR)\test007.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test008.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test008.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test008.exe" "$(INTDIR)\test008.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test009.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test009.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test009.exe" "$(INTDIR)\test009.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test010.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test010.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test010.exe" "$(INTDIR)\test010.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test011.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test011.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test011.exe" "$(INTDIR)\test011.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test012.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test012.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test012.exe" "$(INTDIR)\test012.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test013.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test013.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test013.exe" "$(INTDIR)\test013.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test014.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test014.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test014.exe" "$(INTDIR)\test014.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test015.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test015.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test015.exe" "$(INTDIR)\test015.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test016.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test016.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test016.exe" "$(INTDIR)\test016.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test017.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test017.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test017.exe" "$(INTDIR)\test017.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test018.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test018.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test018.exe" "$(INTDIR)\test018.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test019.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test019.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test019.exe" "$(INTDIR)\test019.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test020.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test020.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test020.exe" "$(INTDIR)\test020.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test021.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test021.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test021.exe" "$(INTDIR)\test021.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test022.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test022.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test022.exe" "$(INTDIR)\test022.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test023.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test023.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test023.exe" "$(INTDIR)\test023.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test024.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test024.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test024.exe" "$(INTDIR)\test024.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test025.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test025.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test025.exe" "$(INTDIR)\test025.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test026.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test026.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test026.exe" "$(INTDIR)\test026.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test027.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test027.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test027.exe" "$(INTDIR)\test027.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test028.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test028.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test028.exe" "$(INTDIR)\test028.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test029.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test029.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test029.exe" "$(INTDIR)\test029.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test030.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test030.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test030.exe" "$(INTDIR)\test030.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test031.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test031.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test031.exe" "$(INTDIR)\test031.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test032.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test032.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test032.exe" "$(INTDIR)\test032.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test033.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test033.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test033.exe" "$(INTDIR)\test033.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test034.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test034.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test034.exe" "$(INTDIR)\test034.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test035.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test035.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test035.exe" "$(INTDIR)\test035.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test036.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test036.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test036.exe" "$(INTDIR)\test036.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test037.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test037.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test037.exe" "$(INTDIR)\test037.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test038.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test038.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test038.exe" "$(INTDIR)\test038.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test039.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test039.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test039.exe" "$(INTDIR)\test039.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test040.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test040.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test040.exe" "$(INTDIR)\test040.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test041.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test041.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test041.exe" "$(INTDIR)\test041.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test042.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test042.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test042.exe" "$(INTDIR)\test042.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test043.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test043.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test043.exe" "$(INTDIR)\test043.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test044.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test044.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test044.exe" "$(INTDIR)\test044.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test045.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test045.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test045.exe" "$(INTDIR)\test045.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test046.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test046.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test046.exe" "$(INTDIR)\test046.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test047.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test047.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test047.exe" "$(INTDIR)\test047.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test048.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test048.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test048.exe" "$(INTDIR)\test048.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test049.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test049.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test049.exe" "$(INTDIR)\test049.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test050.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test050.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test050.exe" "$(INTDIR)\test050.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test051.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test051.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test051.exe" "$(INTDIR)\test051.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test052.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test052.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test052.exe" "$(INTDIR)\test052.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test053.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test053.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test053.exe" "$(INTDIR)\test053.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test054.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test054.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test054.exe" "$(INTDIR)\test054.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test055.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test055.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test055.exe" "$(INTDIR)\test055.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test056.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test056.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test056.exe" "$(INTDIR)\test056.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test057.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test057.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test057.exe" "$(INTDIR)\test057.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test058.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test058.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test058.exe" "$(INTDIR)\test058.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test059.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test059.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test059.exe" "$(INTDIR)\test059.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test060.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test060.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test060.exe" "$(INTDIR)\test060.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test061.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test061.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test061.exe" "$(INTDIR)\test061.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test062.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test062.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test062.exe" "$(INTDIR)\test062.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test063.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test063.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test063.exe" "$(INTDIR)\test063.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test064.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test064.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test064.exe" "$(INTDIR)\test064.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test065.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test065.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test065.exe" "$(INTDIR)\test065.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test066.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test066.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test066.exe" "$(INTDIR)\test066.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test067.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test067.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test067.exe" "$(INTDIR)\test067.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test068.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test068.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test068.exe" "$(INTDIR)\test068.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test069.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test069.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test069.exe" "$(INTDIR)\test069.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test070.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test070.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test070.exe" "$(INTDIR)\test070.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test071.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test071.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test071.exe" "$(INTDIR)\test071.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test072.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test072.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test072.exe" "$(INTDIR)\test072.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test073.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test073.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test073.exe" "$(INTDIR)\test073.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test074.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test074.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test074.exe" "$(INTDIR)\test074.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test075.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test075.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test075.exe" "$(INTDIR)\test075.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test076.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test076.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test076.exe" "$(INTDIR)\test076.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test077.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test077.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test077.exe" "$(INTDIR)\test077.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test078.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test078.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test078.exe" "$(INTDIR)\test078.obj"
<<
        -@erase "$(INTDIR)" /Q

"$(OUTDIR)\test079.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test079.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test079.exe" "$(INTDIR)\test079.obj"
<<
        -@erase "$(INTDIR)" /Q


.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

!IF "$(CFG)" == "Release" || "$(CFG)" == "Debug"

SOURCE=..\test\test001.cxx
"$(INTDIR)\test001.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test002.cxx
"$(INTDIR)\test002.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test003.cxx
"$(INTDIR)\test003.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test004.cxx
"$(INTDIR)\test004.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test005.cxx
"$(INTDIR)\test005.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test006.cxx
"$(INTDIR)\test006.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test007.cxx
"$(INTDIR)\test007.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test008.cxx
"$(INTDIR)\test008.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test009.cxx
"$(INTDIR)\test009.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test010.cxx
"$(INTDIR)\test010.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test011.cxx
"$(INTDIR)\test011.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test012.cxx
"$(INTDIR)\test012.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test013.cxx
"$(INTDIR)\test013.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test014.cxx
"$(INTDIR)\test014.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test015.cxx
"$(INTDIR)\test015.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test016.cxx
"$(INTDIR)\test016.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test017.cxx
"$(INTDIR)\test017.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test018.cxx
"$(INTDIR)\test018.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test019.cxx
"$(INTDIR)\test019.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test020.cxx
"$(INTDIR)\test020.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test021.cxx
"$(INTDIR)\test021.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test022.cxx
"$(INTDIR)\test022.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test023.cxx
"$(INTDIR)\test023.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test024.cxx
"$(INTDIR)\test024.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test025.cxx
"$(INTDIR)\test025.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test026.cxx
"$(INTDIR)\test026.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test027.cxx
"$(INTDIR)\test027.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test028.cxx
"$(INTDIR)\test028.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test029.cxx
"$(INTDIR)\test029.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test030.cxx
"$(INTDIR)\test030.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test031.cxx
"$(INTDIR)\test031.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test032.cxx
"$(INTDIR)\test032.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test033.cxx
"$(INTDIR)\test033.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test034.cxx
"$(INTDIR)\test034.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test035.cxx
"$(INTDIR)\test035.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test036.cxx
"$(INTDIR)\test036.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test037.cxx
"$(INTDIR)\test037.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test038.cxx
"$(INTDIR)\test038.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test039.cxx
"$(INTDIR)\test039.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test040.cxx
"$(INTDIR)\test040.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test041.cxx
"$(INTDIR)\test041.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test042.cxx
"$(INTDIR)\test042.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test043.cxx
"$(INTDIR)\test043.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test044.cxx
"$(INTDIR)\test044.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test045.cxx
"$(INTDIR)\test045.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test046.cxx
"$(INTDIR)\test046.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test047.cxx
"$(INTDIR)\test047.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test048.cxx
"$(INTDIR)\test048.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test049.cxx
"$(INTDIR)\test049.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test050.cxx
"$(INTDIR)\test050.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test051.cxx
"$(INTDIR)\test051.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test052.cxx
"$(INTDIR)\test052.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test053.cxx
"$(INTDIR)\test053.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test054.cxx
"$(INTDIR)\test054.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test055.cxx
"$(INTDIR)\test055.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test056.cxx
"$(INTDIR)\test056.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test057.cxx
"$(INTDIR)\test057.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test058.cxx
"$(INTDIR)\test058.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test059.cxx
"$(INTDIR)\test059.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test060.cxx
"$(INTDIR)\test060.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test061.cxx
"$(INTDIR)\test061.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test062.cxx
"$(INTDIR)\test062.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test063.cxx
"$(INTDIR)\test063.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test064.cxx
"$(INTDIR)\test064.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test065.cxx
"$(INTDIR)\test065.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test066.cxx
"$(INTDIR)\test066.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test067.cxx
"$(INTDIR)\test067.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test068.cxx
"$(INTDIR)\test068.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test069.cxx
"$(INTDIR)\test069.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test070.cxx
"$(INTDIR)\test070.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test071.cxx
"$(INTDIR)\test071.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test072.cxx
"$(INTDIR)\test072.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test073.cxx
"$(INTDIR)\test073.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test074.cxx
"$(INTDIR)\test074.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test075.cxx
"$(INTDIR)\test075.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test076.cxx
"$(INTDIR)\test076.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test077.cxx
"$(INTDIR)\test077.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test078.cxx
"$(INTDIR)\test078.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\test079.cxx
"$(INTDIR)\test079.obj" : $(SOURCE) "$(INTDIR)"
        @$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF

