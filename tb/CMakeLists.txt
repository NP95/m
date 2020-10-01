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

include(CMakeParseArguments)


add_verilator_include_path(${CMAKE_CURRENT_SOURCE_DIR})

set(Verilator_SRCS
  ${Verilator_INCLUDE_DIR}/verilated.cpp
  ${Verilator_INCLUDE_DIR}/verilated_dpi.cpp
  ${Verilator_INCLUDE_DIR}/verilated_save.cpp
  ${Verilator_INCLUDE_DIR}/verilated_vcd_c.cpp
  )
set(Verilator_INCLUDE_DIR
  ${Verilator_INCLUDE_DIR}
  ${Verilator_INCLUDE_DIR}/vltstd
  )
add_library(v STATIC ${Verilator_SRCS})
target_include_directories(v PUBLIC
  ${Verilator_INCLUDE_DIR}
  )

if (OPT_ENABLE_VCD)
  add_verilator_option("--trace")
endif ()

get_property(vinclude_path GLOBAL PROPERTY vinclude_path)
get_property(verilator_opts GLOBAL PROPERTY verilator_opts)

add_custom_target(
  build_verilog
  COMMAND ${CMAKE_COMMAND} -E env
  CXXFLAGS="-std=c++17"
  VERILATOR_EXE=${Verilator_EXE}
  CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
  verilator_opts="${verilator_opts}"
  vinclude_path="${vinclude_path}"
  top="${CMAKE_CURRENT_SOURCE_DIR}/tb.sv"
  ${CMAKE_SOURCE_DIR}/scripts/run_verilator.sh
  COMMENT "Verilating (C++): tb.sv"
  )

configure_file(tb.h.in tb.h)
add_library(tb SHARED
  tb.cc
  utility.cc
  )
target_link_libraries(tb
  ${CMAKE_CURRENT_BINARY_DIR}/vobj/Vtb__ALL.a
  v
  gtest_main
  gtest
  pthread
  )
target_include_directories(tb PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  )
add_dependencies(tb build_verilog)

macro (create_test target file)
  add_executable(${target} ${file})
  target_link_libraries(${target} tb)
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  add_test(NAME ${target} COMMAND ${target})
endmacro ()

create_test(smoke smoke.cc)
create_test(regress regress.cc)
