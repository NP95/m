##========================================================================== //
## Copyright (c) 2016-2019, Stephen Henry
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##========================================================================== //

# ---------------------------------------------------------------------------- #
# Fetch dependencies

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.12.0
)

# ---------------------------------------------------------------------------- #
# Paramterizations

option(OPT_VCD_ENABLE "Enable waveform tracing (VCD)." OFF)
option(OPT_LOGGING_ENABLE "Enable logging." OFF)

# ---------------------------------------------------------------------------- #
# Verilate

include(rtl)

# Build verilator support library
verilator_build(vlib)

set(VERILATOR_ARGS
  "-cc"
  "-Wall"
  "--Mdir Vobj"
  "--build"
  "--top tb"
  )
if (OPT_VCD_ENABLE)
  list(APPEND VERILATOR_ARGS --trace)
endif ()

set(TB_SOURCES
  "${RTL_SOURCES}"
  "${CMAKE_CURRENT_SOURCE_DIR}/tb.sv")

set(VERILATOR_COMMAND_LIST
  "${VERILATOR_ARGS}"
  "${VERILATOR_INCLUDES}"
  "${TB_SOURCES}")

string(REGEX REPLACE ";" "\n" FILELIST "${VERILATOR_COMMAND_LIST}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/vc.f "${FILELIST}")

add_custom_target(verilate
  COMMAND ${Verilator_EXE} -f ${CMAKE_CURRENT_BINARY_DIR}/vc.f
  COMMENT "Verilating...")

set(VERILATOR_A "${CMAKE_CURRENT_BINARY_DIR}/Vobj/Vtb__ALL.a")

# ---------------------------------------------------------------------------- #
# Driver executable:

configure_file(tb.h.in tb.h)

set(DRIVER_CPP
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/regress.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/tb.cc"
  )

add_executable(driver ${DRIVER_CPP})
target_include_directories(driver PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${VERILATOR_ROOT}/include")
target_link_libraries(driver vlib ${VERILATOR_A})
add_dependencies(driver verilate)
