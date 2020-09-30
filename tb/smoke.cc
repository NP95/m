//========================================================================== //
// Copyright (c) 2020, Stephen Henry
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//========================================================================== //

#include "gtest/gtest.h"
#include "tb.h"
#include <deque>

TEST(m, scenario0) {
  tb::Random::init(1);
  
  tb::Options opts;
  opts.enable_vcd = true;
  
  tb::TB tb(opts);

  std::deque<tb::TestCase> tests;

  const std::size_t beats = 2;
  tb::TestCase tc;
  for (std::size_t i = 0; i < beats; i++) {
    tb::In in;
    in.valid = true;
    in.sop = (i == 0);
    in.eop = (i == (beats - 1));
    in.length = 7;
    in.data = tb::Random::uniform<vluint64_t>();
    tc.in.push_back(in);
  }
  for (std::size_t i = 0; i < beats; i++) {
    tb::Out out;
    out.valid = true;
    out.sop = (i == 0);
    out.eop = (i == (beats - 1));
    out.length = 7;
    // In -> Out; data is not changed.
    out.data = tc.in[i].data;
    out.buffer = 0;
    tc.out.push_back(out);
  }
  tests.push_back(tc);
  
  tb.run(tests);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
