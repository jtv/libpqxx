################################################################################
# AUTOMATICALLY GENERATED FILE--DO NOT EDIT
#
# This file is generated automatically by libpqxx's template2mak.py script.
#
# If you modify this file, chances are your modifications will be lost because
# the file will be re-written from time to time.
#
# The template2mak.py script should be available in the tools directory of the
# libpqxx source archive.
#
# Generated from template '/home/jtv/proj/libpqxx/win32/vc-test.mak.template'.
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
    /I "include" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" $(PQXX_SHARED)

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
  $(INTDIR)\test000.obj \
  $(INTDIR)\test001.obj \
  $(INTDIR)\test002.obj \
  $(INTDIR)\test004.obj \
  $(INTDIR)\test007.obj \
  $(INTDIR)\test010.obj \
  $(INTDIR)\test011.obj \
  $(INTDIR)\test012.obj \
  $(INTDIR)\test013.obj \
  $(INTDIR)\test014.obj \
  $(INTDIR)\test015.obj \
  $(INTDIR)\test016.obj \
  $(INTDIR)\test017.obj \
  $(INTDIR)\test018.obj \
  $(INTDIR)\test020.obj \
  $(INTDIR)\test021.obj \
  $(INTDIR)\test023.obj \
  $(INTDIR)\test026.obj \
  $(INTDIR)\test029.obj \
  $(INTDIR)\test030.obj \
  $(INTDIR)\test031.obj \
  $(INTDIR)\test032.obj \
  $(INTDIR)\test033.obj \
  $(INTDIR)\test034.obj \
  $(INTDIR)\test035.obj \
  $(INTDIR)\test036.obj \
  $(INTDIR)\test037.obj \
  $(INTDIR)\test039.obj \
  $(INTDIR)\test046.obj \
  $(INTDIR)\test048.obj \
  $(INTDIR)\test049.obj \
  $(INTDIR)\test050.obj \
  $(INTDIR)\test051.obj \
  $(INTDIR)\test052.obj \
  $(INTDIR)\test053.obj \
  $(INTDIR)\test054.obj \
  $(INTDIR)\test055.obj \
  $(INTDIR)\test056.obj \
  $(INTDIR)\test057.obj \
  $(INTDIR)\test058.obj \
  $(INTDIR)\test059.obj \
  $(INTDIR)\test060.obj \
  $(INTDIR)\test061.obj \
  $(INTDIR)\test062.obj \
  $(INTDIR)\test063.obj \
  $(INTDIR)\test064.obj \
  $(INTDIR)\test065.obj \
  $(INTDIR)\test066.obj \
  $(INTDIR)\test067.obj \
  $(INTDIR)\test069.obj \
  $(INTDIR)\test070.obj \
  $(INTDIR)\test071.obj \
  $(INTDIR)\test072.obj \
  $(INTDIR)\test073.obj \
  $(INTDIR)\test074.obj \
  $(INTDIR)\test075.obj \
  $(INTDIR)\test076.obj \
  $(INTDIR)\test077.obj \
  $(INTDIR)\test078.obj \
  $(INTDIR)\test079.obj \
  $(INTDIR)\test082.obj \
  $(INTDIR)\test083.obj \
  $(INTDIR)\test084.obj \
  $(INTDIR)\test086.obj \
  $(INTDIR)\test087.obj \
  $(INTDIR)\test088.obj \
  $(INTDIR)\test089.obj \
  $(INTDIR)\test090.obj \
  $(INTDIR)\test092.obj \
  $(INTDIR)\test093.obj \
  $(INTDIR)\test094.obj \
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
$(INTDIR)\test000.obj::
	@$(CXX) $(CXX_FLAGS) test/test000.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test001.obj::
	@$(CXX) $(CXX_FLAGS) test/test001.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test002.obj::
	@$(CXX) $(CXX_FLAGS) test/test002.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test004.obj::
	@$(CXX) $(CXX_FLAGS) test/test004.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test007.obj::
	@$(CXX) $(CXX_FLAGS) test/test007.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test010.obj::
	@$(CXX) $(CXX_FLAGS) test/test010.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test011.obj::
	@$(CXX) $(CXX_FLAGS) test/test011.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test012.obj::
	@$(CXX) $(CXX_FLAGS) test/test012.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test013.obj::
	@$(CXX) $(CXX_FLAGS) test/test013.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test014.obj::
	@$(CXX) $(CXX_FLAGS) test/test014.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test015.obj::
	@$(CXX) $(CXX_FLAGS) test/test015.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test016.obj::
	@$(CXX) $(CXX_FLAGS) test/test016.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test017.obj::
	@$(CXX) $(CXX_FLAGS) test/test017.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test018.obj::
	@$(CXX) $(CXX_FLAGS) test/test018.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test020.obj::
	@$(CXX) $(CXX_FLAGS) test/test020.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test021.obj::
	@$(CXX) $(CXX_FLAGS) test/test021.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test023.obj::
	@$(CXX) $(CXX_FLAGS) test/test023.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test026.obj::
	@$(CXX) $(CXX_FLAGS) test/test026.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test029.obj::
	@$(CXX) $(CXX_FLAGS) test/test029.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test030.obj::
	@$(CXX) $(CXX_FLAGS) test/test030.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test031.obj::
	@$(CXX) $(CXX_FLAGS) test/test031.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test032.obj::
	@$(CXX) $(CXX_FLAGS) test/test032.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test033.obj::
	@$(CXX) $(CXX_FLAGS) test/test033.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test034.obj::
	@$(CXX) $(CXX_FLAGS) test/test034.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test035.obj::
	@$(CXX) $(CXX_FLAGS) test/test035.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test036.obj::
	@$(CXX) $(CXX_FLAGS) test/test036.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test037.obj::
	@$(CXX) $(CXX_FLAGS) test/test037.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test039.obj::
	@$(CXX) $(CXX_FLAGS) test/test039.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test046.obj::
	@$(CXX) $(CXX_FLAGS) test/test046.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test048.obj::
	@$(CXX) $(CXX_FLAGS) test/test048.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test049.obj::
	@$(CXX) $(CXX_FLAGS) test/test049.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test050.obj::
	@$(CXX) $(CXX_FLAGS) test/test050.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test051.obj::
	@$(CXX) $(CXX_FLAGS) test/test051.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test052.obj::
	@$(CXX) $(CXX_FLAGS) test/test052.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test053.obj::
	@$(CXX) $(CXX_FLAGS) test/test053.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test054.obj::
	@$(CXX) $(CXX_FLAGS) test/test054.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test055.obj::
	@$(CXX) $(CXX_FLAGS) test/test055.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test056.obj::
	@$(CXX) $(CXX_FLAGS) test/test056.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test057.obj::
	@$(CXX) $(CXX_FLAGS) test/test057.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test058.obj::
	@$(CXX) $(CXX_FLAGS) test/test058.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test059.obj::
	@$(CXX) $(CXX_FLAGS) test/test059.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test060.obj::
	@$(CXX) $(CXX_FLAGS) test/test060.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test061.obj::
	@$(CXX) $(CXX_FLAGS) test/test061.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test062.obj::
	@$(CXX) $(CXX_FLAGS) test/test062.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test063.obj::
	@$(CXX) $(CXX_FLAGS) test/test063.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test064.obj::
	@$(CXX) $(CXX_FLAGS) test/test064.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test065.obj::
	@$(CXX) $(CXX_FLAGS) test/test065.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test066.obj::
	@$(CXX) $(CXX_FLAGS) test/test066.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test067.obj::
	@$(CXX) $(CXX_FLAGS) test/test067.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test069.obj::
	@$(CXX) $(CXX_FLAGS) test/test069.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test070.obj::
	@$(CXX) $(CXX_FLAGS) test/test070.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test071.obj::
	@$(CXX) $(CXX_FLAGS) test/test071.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test072.obj::
	@$(CXX) $(CXX_FLAGS) test/test072.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test073.obj::
	@$(CXX) $(CXX_FLAGS) test/test073.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test074.obj::
	@$(CXX) $(CXX_FLAGS) test/test074.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test075.obj::
	@$(CXX) $(CXX_FLAGS) test/test075.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test076.obj::
	@$(CXX) $(CXX_FLAGS) test/test076.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test077.obj::
	@$(CXX) $(CXX_FLAGS) test/test077.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test078.obj::
	@$(CXX) $(CXX_FLAGS) test/test078.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test079.obj::
	@$(CXX) $(CXX_FLAGS) test/test079.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test082.obj::
	@$(CXX) $(CXX_FLAGS) test/test082.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test083.obj::
	@$(CXX) $(CXX_FLAGS) test/test083.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test084.obj::
	@$(CXX) $(CXX_FLAGS) test/test084.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test086.obj::
	@$(CXX) $(CXX_FLAGS) test/test086.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test087.obj::
	@$(CXX) $(CXX_FLAGS) test/test087.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test088.obj::
	@$(CXX) $(CXX_FLAGS) test/test088.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test089.obj::
	@$(CXX) $(CXX_FLAGS) test/test089.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test090.obj::
	@$(CXX) $(CXX_FLAGS) test/test090.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test092.obj::
	@$(CXX) $(CXX_FLAGS) test/test092.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test093.obj::
	@$(CXX) $(CXX_FLAGS) test/test093.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test094.obj::
	@$(CXX) $(CXX_FLAGS) test/test094.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"


$(INTDIR)\$(LIBPQ)::
	@copy "$(LIBDIR)\$(LIBPQ)" "$(INTDIR)"

$(INTDIR)\$(PQXX)::
	@copy "$(LIBDIR)\$(PQXX)" "$(INTDIR)"
