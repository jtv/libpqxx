# Clinton James clinton.james@jidn.com March 2002
!IF "$(CFG)" != "Release" && "$(CFG)" != "Debug"
!MESSAGE You can specify a specific testcase when running NMAKE. For example:
!MESSAGE     NMAKE /f "test.mak" testcase
!MESSAGE Possible choices for testcase are TEST1 through TEST20 or ALL
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

   
TEST1 : "$(OUTDIR)\test1.exe"
TEST2 : "$(OUTDIR)\test2.exe"
TEST3 : "$(OUTDIR)\test3.exe"
TEST4 : "$(OUTDIR)\test4.exe"
TEST5 : "$(OUTDIR)\test5.exe"
TEST6 : "$(OUTDIR)\test6.exe"
TEST7 : "$(OUTDIR)\test7.exe"
TEST8 : "$(OUTDIR)\test8.exe"
TEST9 : "$(OUTDIR)\test9.exe"
TEST10: "$(OUTDIR)\test10.exe"
TEST11: "$(OUTDIR)\test11.exe"
TEST12: "$(OUTDIR)\test12.exe"
TEST13: "$(OUTDIR)\test13.exe"
TEST14: "$(OUTDIR)\test14.exe"
TEST15: "$(OUTDIR)\test15.exe"
TEST16: "$(OUTDIR)\test16.exe"
TEST17: "$(OUTDIR)\test17.exe"
TEST18: "$(OUTDIR)\test18.exe"
TEST19: "$(OUTDIR)\test19.exe"
TEST20: "$(OUTDIR)\test20.exe"

ALL : TEST1 TEST2 TEST3 TEST4 TEST5 TEST6 TEST7 TEST8 TEST9 TEST10 TEST11 TEST12 TEST13 TEST14 TEST15 TEST16 TEST17 TEST18 TEST19 TEST20
	
CLEAN :
	 "$(INTDIR)" /Q
	-@erase "$(OUTDIR)\test*.exe" /Q

"$(INTDIR)" :
	if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"
	
"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(OUTDIR)\test1.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test1.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test1.exe" "$(INTDIR)\test1.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test2.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test2.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test2.exe" "$(INTDIR)\test2.obj"
<<
	-@erase "$(INTDIR)" /Q
	
"$(OUTDIR)\test3.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test3.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test3.exe" "$(INTDIR)\test3.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test4.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test4.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test4.exe" "$(INTDIR)\test4.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test5.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test5.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test5.exe" "$(INTDIR)\test5.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test6.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test6.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test6.exe" "$(INTDIR)\test6.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test7.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test7.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test7.exe" "$(INTDIR)\test7.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test8.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test8.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test8.exe" "$(INTDIR)\test8.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test9.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test9.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test9.exe" "$(INTDIR)\test9.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test10.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test10.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test10.exe" "$(INTDIR)\test10.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test11.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test11.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test11.exe" "$(INTDIR)\test11.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test12.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test12.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test12.exe" "$(INTDIR)\test12.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test13.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test13.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test13.exe" "$(INTDIR)\test13.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test14.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test14.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test14.exe" "$(INTDIR)\test14.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test15.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test15.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test15.exe" "$(INTDIR)\test15.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test16.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test16.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test16.exe" "$(INTDIR)\test16.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test17.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test17.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test17.exe" "$(INTDIR)\test17.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test18.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test18.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test18.exe" "$(INTDIR)\test18.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test19.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test19.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test19.exe" "$(INTDIR)\test19.obj"
<<
	-@erase "$(INTDIR)" /Q

"$(OUTDIR)\test20.exe" : "$(OUTDIR)" $(DEF_FILE) "$(INTDIR)\test20.obj"
    @$(LINK32) @<<
  $(LINK32_FLAGS) /out:"$(OUTDIR)\test20.exe" "$(INTDIR)\test20.obj"
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

SOURCE=..\test\test1.cxx
"$(INTDIR)\test1.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)
	
SOURCE=..\test\test2.cxx
"$(INTDIR)\test2.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test3.cxx
"$(INTDIR)\test3.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test4.cxx
"$(INTDIR)\test4.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)
	
SOURCE=..\test\test5.cxx
"$(INTDIR)\test5.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test6.cxx
"$(INTDIR)\test6.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test7.cxx
"$(INTDIR)\test7.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test8.cxx
"$(INTDIR)\test8.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test9.cxx
"$(INTDIR)\test9.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test10.cxx
"$(INTDIR)\test10.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test11.cxx
"$(INTDIR)\test11.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test12.cxx
"$(INTDIR)\test12.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test13.cxx
"$(INTDIR)\test13.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test14.cxx
"$(INTDIR)\test14.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test15.cxx
"$(INTDIR)\test15.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test16.cxx
"$(INTDIR)\test16.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test17.cxx
"$(INTDIR)\test17.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test18.cxx
"$(INTDIR)\test18.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test19.cxx
"$(INTDIR)\test19.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\test\test20.cxx
"$(INTDIR)\test20.obj" : $(SOURCE) "$(INTDIR)"
	@$(CPP) $(CPP_PROJ) $(SOURCE)

!ENDIF 


