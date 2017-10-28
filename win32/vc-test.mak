################################################################################
# AUTOMATICALLY GENERATED FILE -- DO NOT EDIT.
#
# This file is generated automatically by libpqxx's template2mak.py script, and
# will be rewritten from time to time.
#
# If you modify this file, chances are your modifications will be lost.
#
# The template2mak.py script should be available in the tools directory of the
# libpqxx source archive.
#
# Generated from template 'win32/vc-test.mak.template'.
################################################################################
# Visual C++ Makefile for libpqxx test suite
# This file was written by Bart Samwel.

default:
	@echo LIBPQXX testsuite Makefile for Visual C++ Available Targets
	@echo -----------------------------------------------------------
	@echo.
	@echo ALL: perform all tests
	@echo TESTxxx: perform specific test xxx
	@echo CLEAN: clean up all output so that tests will run again.
	@echo.
	@echo Pass the option DLL=1 to link the test suite against the libpqxx DLL instead
	@echo of against the static libpqxx library, and pass DEBUG=1 to link against
	@echo the debug build of libpqxx.

!include win32\common

LIBDIR=lib


# C++ compiler, linker
CXX=cl.exe
LINK=link.exe

!IF "$(DLL)" == "1"
PQXX_SHARED=/D "PQXX_SHARED"
PQXXLIBTYPE=Dll
PQXXLIBTYPESUFFIX=
PQXXLIBEXT=dll
!ELSE
PQXXLIBTYPE=Static
PQXXLIBEXT=lib
PQXXLIBTYPESUFFIX=_static
!ENDIF


CXX_FLAGS_BASE=/nologo /W3 /EHsc /FD /GR /c \
    /I "include" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" /D "NOMINMAX" $(PQXX_SHARED)

LINK_FLAGS_BASE=kernel32.lib ws2_32.lib advapi32.lib /nologo /machine:I386 /libpath:"$(LIBDIR)"


!IF "$(DEBUG)" == "1"
BUILDMODE=Debug
DEBUGSUFFIX=D
CXX_FLAGS=$(CXX_FLAGS_BASE) /MDd /Gm /ZI /Od /D "_DEBUG" /RTC1
LINK_FLAGS=$(LINK_FLAGS_BASE) $(LIBPQDLIB) /debug
LIBPQ=$(LIBPQDDLL)
!ELSE
BUILDMODE=Release
DEBUGSUFFIX=
CXX_FLAGS=$(CXX_FLAGS_BASE) /MD /D "NDEBUG"
LINK_FLAGS=$(LINK_FLAGS_BASE) $(LIBPQLIB)
LIBPQ=$(LIBPQDLL)
!ENDIF


INTDIR=Test$(PQXXLIBTYPE)$(BUILDMODE)
PQXXLIB=libpqxx$(PQXXLIBTYPESUFFIX)$(DEBUGSUFFIX).lib
PQXX=libpqxx$(DEBUGSUFFIX).dll

!IF "$(DLL)" == "1"
DLLS=$(INTDIR)\$(LIBPQ) $(INTDIR)\$(PQXX)
!ELSE
DLLS=$(INTDIR)\$(LIBPQ)
!ENDIF

OBJS= \
  $(INTDIR)\test00.obj \
  $(INTDIR)\test01.obj \
  $(INTDIR)\test02.obj \
  $(INTDIR)\test04.obj \
  $(INTDIR)\test07.obj \
  $(INTDIR)\test10.obj \
  $(INTDIR)\test11.obj \
  $(INTDIR)\test12.obj \
  $(INTDIR)\test13.obj \
  $(INTDIR)\test14.obj \
  $(INTDIR)\test15.obj \
  $(INTDIR)\test16.obj \
  $(INTDIR)\test17.obj \
  $(INTDIR)\test18.obj \
  $(INTDIR)\test20.obj \
  $(INTDIR)\test21.obj \
  $(INTDIR)\test23.obj \
  $(INTDIR)\test26.obj \
  $(INTDIR)\test29.obj \
  $(INTDIR)\test30.obj \
  $(INTDIR)\test31.obj \
  $(INTDIR)\test32.obj \
  $(INTDIR)\test33.obj \
  $(INTDIR)\test34.obj \
  $(INTDIR)\test35.obj \
  $(INTDIR)\test36.obj \
  $(INTDIR)\test37.obj \
  $(INTDIR)\test39.obj \
  $(INTDIR)\test46.obj \
  $(INTDIR)\test48.obj \
  $(INTDIR)\test49.obj \
  $(INTDIR)\test50.obj \
  $(INTDIR)\test51.obj \
  $(INTDIR)\test52.obj \
  $(INTDIR)\test53.obj \
  $(INTDIR)\test54.obj \
  $(INTDIR)\test55.obj \
  $(INTDIR)\test56.obj \
  $(INTDIR)\test57.obj \
  $(INTDIR)\test58.obj \
  $(INTDIR)\test59.obj \
  $(INTDIR)\test60.obj \
  $(INTDIR)\test61.obj \
  $(INTDIR)\test62.obj \
  $(INTDIR)\test63.obj \
  $(INTDIR)\test64.obj \
  $(INTDIR)\test65.obj \
  $(INTDIR)\test66.obj \
  $(INTDIR)\test67.obj \
  $(INTDIR)\test69.obj \
  $(INTDIR)\test70.obj \
  $(INTDIR)\test71.obj \
  $(INTDIR)\test72.obj \
  $(INTDIR)\test73.obj \
  $(INTDIR)\test74.obj \
  $(INTDIR)\test75.obj \
  $(INTDIR)\test76.obj \
  $(INTDIR)\test77.obj \
  $(INTDIR)\test78.obj \
  $(INTDIR)\test79.obj \
  $(INTDIR)\test82.obj \
  $(INTDIR)\test83.obj \
  $(INTDIR)\test84.obj \
  $(INTDIR)\test86.obj \
  $(INTDIR)\test87.obj \
  $(INTDIR)\test88.obj \
  $(INTDIR)\test89.obj \
  $(INTDIR)\test90.obj \
  $(INTDIR)\test92.obj \
  $(INTDIR)\test93.obj \
  $(INTDIR)\test94.obj \
  $(INTDIR)\runner.obj


########################################################
# Logical targets
########################################################

all: runner

runner: $(INTDIR) $(INTDIR)\runner.exe

clean:
	-@del /Q $(INTDIR)\*.*

$(INTDIR):
	@mkdir $(INTDIR)


########################################################
# Test implementations
########################################################


$(INTDIR)\runner.success: $(INTDIR)\runner.exe
	@$(INTDIR)\runner.exe
	@echo >$(INTDIR)\runner.success


$(INTDIR)\runner.exe: $(OBJS) $(DLLS)
	@$(LINK) $(LINK_FLAGS) $(OBJS) $(PQXXLIB) \
		/out:"$(INTDIR)\\runner.exe"


$(INTDIR)\runner.obj::
	@$(CXX) $(CXX_FLAGS) test/runner.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test00.obj::
	@$(CXX) $(CXX_FLAGS) test/test00.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test01.obj::
	@$(CXX) $(CXX_FLAGS) test/test01.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test02.obj::
	@$(CXX) $(CXX_FLAGS) test/test02.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test04.obj::
	@$(CXX) $(CXX_FLAGS) test/test04.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test07.obj::
	@$(CXX) $(CXX_FLAGS) test/test07.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test10.obj::
	@$(CXX) $(CXX_FLAGS) test/test10.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test11.obj::
	@$(CXX) $(CXX_FLAGS) test/test11.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test12.obj::
	@$(CXX) $(CXX_FLAGS) test/test12.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test13.obj::
	@$(CXX) $(CXX_FLAGS) test/test13.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test14.obj::
	@$(CXX) $(CXX_FLAGS) test/test14.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test15.obj::
	@$(CXX) $(CXX_FLAGS) test/test15.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test16.obj::
	@$(CXX) $(CXX_FLAGS) test/test16.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test17.obj::
	@$(CXX) $(CXX_FLAGS) test/test17.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test18.obj::
	@$(CXX) $(CXX_FLAGS) test/test18.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test20.obj::
	@$(CXX) $(CXX_FLAGS) test/test20.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test21.obj::
	@$(CXX) $(CXX_FLAGS) test/test21.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test23.obj::
	@$(CXX) $(CXX_FLAGS) test/test23.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test26.obj::
	@$(CXX) $(CXX_FLAGS) test/test26.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test29.obj::
	@$(CXX) $(CXX_FLAGS) test/test29.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test30.obj::
	@$(CXX) $(CXX_FLAGS) test/test30.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test31.obj::
	@$(CXX) $(CXX_FLAGS) test/test31.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test32.obj::
	@$(CXX) $(CXX_FLAGS) test/test32.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test33.obj::
	@$(CXX) $(CXX_FLAGS) test/test33.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test34.obj::
	@$(CXX) $(CXX_FLAGS) test/test34.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test35.obj::
	@$(CXX) $(CXX_FLAGS) test/test35.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test36.obj::
	@$(CXX) $(CXX_FLAGS) test/test36.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test37.obj::
	@$(CXX) $(CXX_FLAGS) test/test37.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test39.obj::
	@$(CXX) $(CXX_FLAGS) test/test39.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test46.obj::
	@$(CXX) $(CXX_FLAGS) test/test46.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test48.obj::
	@$(CXX) $(CXX_FLAGS) test/test48.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test49.obj::
	@$(CXX) $(CXX_FLAGS) test/test49.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test50.obj::
	@$(CXX) $(CXX_FLAGS) test/test50.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test51.obj::
	@$(CXX) $(CXX_FLAGS) test/test51.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test52.obj::
	@$(CXX) $(CXX_FLAGS) test/test52.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test53.obj::
	@$(CXX) $(CXX_FLAGS) test/test53.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test54.obj::
	@$(CXX) $(CXX_FLAGS) test/test54.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test55.obj::
	@$(CXX) $(CXX_FLAGS) test/test55.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test56.obj::
	@$(CXX) $(CXX_FLAGS) test/test56.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test57.obj::
	@$(CXX) $(CXX_FLAGS) test/test57.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test58.obj::
	@$(CXX) $(CXX_FLAGS) test/test58.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test59.obj::
	@$(CXX) $(CXX_FLAGS) test/test59.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test60.obj::
	@$(CXX) $(CXX_FLAGS) test/test60.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test61.obj::
	@$(CXX) $(CXX_FLAGS) test/test61.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test62.obj::
	@$(CXX) $(CXX_FLAGS) test/test62.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test63.obj::
	@$(CXX) $(CXX_FLAGS) test/test63.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test64.obj::
	@$(CXX) $(CXX_FLAGS) test/test64.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test65.obj::
	@$(CXX) $(CXX_FLAGS) test/test65.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test66.obj::
	@$(CXX) $(CXX_FLAGS) test/test66.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test67.obj::
	@$(CXX) $(CXX_FLAGS) test/test67.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test69.obj::
	@$(CXX) $(CXX_FLAGS) test/test69.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test70.obj::
	@$(CXX) $(CXX_FLAGS) test/test70.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test71.obj::
	@$(CXX) $(CXX_FLAGS) test/test71.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test72.obj::
	@$(CXX) $(CXX_FLAGS) test/test72.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test73.obj::
	@$(CXX) $(CXX_FLAGS) test/test73.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test74.obj::
	@$(CXX) $(CXX_FLAGS) test/test74.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test75.obj::
	@$(CXX) $(CXX_FLAGS) test/test75.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test76.obj::
	@$(CXX) $(CXX_FLAGS) test/test76.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test77.obj::
	@$(CXX) $(CXX_FLAGS) test/test77.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test78.obj::
	@$(CXX) $(CXX_FLAGS) test/test78.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test79.obj::
	@$(CXX) $(CXX_FLAGS) test/test79.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test82.obj::
	@$(CXX) $(CXX_FLAGS) test/test82.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test83.obj::
	@$(CXX) $(CXX_FLAGS) test/test83.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test84.obj::
	@$(CXX) $(CXX_FLAGS) test/test84.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test86.obj::
	@$(CXX) $(CXX_FLAGS) test/test86.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test87.obj::
	@$(CXX) $(CXX_FLAGS) test/test87.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test88.obj::
	@$(CXX) $(CXX_FLAGS) test/test88.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test89.obj::
	@$(CXX) $(CXX_FLAGS) test/test89.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test90.obj::
	@$(CXX) $(CXX_FLAGS) test/test90.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test92.obj::
	@$(CXX) $(CXX_FLAGS) test/test92.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test93.obj::
	@$(CXX) $(CXX_FLAGS) test/test93.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test94.obj::
	@$(CXX) $(CXX_FLAGS) test/test94.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"


$(INTDIR)\$(LIBPQ)::
	@copy "$(LIBDIR)\$(LIBPQ)" "$(INTDIR)"

$(INTDIR)\$(PQXX)::
	@copy "$(LIBDIR)\$(PQXX)" "$(INTDIR)"
