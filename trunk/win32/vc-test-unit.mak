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
  $(INTDIR)\test_binarystring.obj \
  $(INTDIR)\test_cancel_query.obj \
  $(INTDIR)\test_error_verbosity.obj \
  $(INTDIR)\test_errorhandler.obj \
  $(INTDIR)\test_escape.obj \
  $(INTDIR)\test_exceptions.obj \
  $(INTDIR)\test_float.obj \
  $(INTDIR)\test_notification.obj \
  $(INTDIR)\test_parameterized.obj \
  $(INTDIR)\test_pipeline.obj \
  $(INTDIR)\test_prepared_statement.obj \
  $(INTDIR)\test_read_transaction.obj \
  $(INTDIR)\test_result_slicing.obj \
  $(INTDIR)\test_simultaneous_transactions.obj \
  $(INTDIR)\test_sql_cursor.obj \
  $(INTDIR)\test_stateless_cursor.obj \
  $(INTDIR)\test_string_conversion.obj \
  $(INTDIR)\test_test_helpers.obj \
  $(INTDIR)\test_thread_safety_model.obj \
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
