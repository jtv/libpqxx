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
# Generated from template '/home/jtv/proj/libpqxx/trunk/win32/vc-test-unit.mak.template'.
################################################################################
# Visual C++ Makefile for libpqxx unit tests
# This file was written by Sam Kapilivsky, based on Bart Samwel's original.

default:
	@echo LIBPQXX unit test suite Makefile for Visual C++ Available Targets
	@echo -----------------------------------------------------------------
	@echo.
	@echo ALL: perform all tests
	@echo TEST_xxx: perform specific test xxx
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
    /I "include" /I "test" /I $(PGSQLINC) /I $(LIBPQINC) \
    /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" $(PQXX_SHARED)

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


INTDIR=Unit$(PQXXLIBTYPE)$(BUILDMODE)
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
	@$(LINK) $(LINK_FLAGS) $(OBJS) $(PQXXLIB) /out:"$(INTDIR)\\runner.exe"


$(INTDIR)\runner.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/runner.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_binarystring.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_binarystring.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_cancel_query.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_cancel_query.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_error_verbosity.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_error_verbosity.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_errorhandler.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_errorhandler.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_escape.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_escape.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_exceptions.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_exceptions.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_float.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_float.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_notification.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_notification.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_parameterized.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_parameterized.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_pipeline.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_pipeline.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_prepared_statement.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_prepared_statement.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_read_transaction.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_read_transaction.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_result_slicing.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_result_slicing.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_simultaneous_transactions.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_simultaneous_transactions.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_sql_cursor.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_sql_cursor.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_stateless_cursor.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_stateless_cursor.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_string_conversion.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_string_conversion.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_test_helpers.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_test_helpers.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"
$(INTDIR)\test_thread_safety_model.obj:
	@$(CXX) $(CXX_FLAGS) test/unit/test_thread_safety_model.cxx /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"


$(INTDIR)\$(LIBPQ):
	@copy $(LIBDIR)\$(LIBPQ) $(INTDIR)


$(INTDIR)\$(PQXX):
	@copy $(LIBDIR)\$(PQXX) $(INTDIR)
