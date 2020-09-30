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


TEST(smoke, passthru) {
  // Simple scenario; expect data in and data out. No matching
  // activity, but data read out should be the same as that originally
  // sent in.

  tb::Random::init(1);
  
  tb::Options opts;
#ifdef OPT_ENABLE_VCD
  // Enable waveforms
  opts.enable_vcd = true;
  opts.vcd_name = "passthru.vcd";
#endif
  
  tb::TB tb(opts);

  std::deque<tb::TestCase> tests;

  const std::size_t beats = 2;

  // Testcase
  tb::TestCase tc;

  // Construct input stimulus:
  //
  for (std::size_t i = 0; i < beats; i++) {
    tb::In in;
    in.valid = true;
    in.sop = (i == 0);
    in.eop = (i == (beats - 1));
    in.length = 7;
    in.data = tb::Random::uniform<vluint64_t>();
    tc.in.push_back(in);
  }

  // Construct expected output:
  //
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

TEST(smoke, simple_match) {
  // Basic match case, send a simple packet through with the match
  // elements at known locations within the payload.
  tb::Random::init(1);
  
  tb::Options opts;
#ifdef OPT_ENABLE_VCD
  // Enable waveforms
  opts.enable_vcd = true;
  opts.vcd_name = "simple_match.vcd";
#endif

  std::deque<tb::TestCase> tests;

  // Issue the same packet 'rounds' times; ensures that internal
  // retained state is appropriately flushed/cleared between packets.
  const std::size_t rounds = 1024;
    const vluint8_t buffer = tb::Random::uniform<vluint8_t>(15);

  for (std::size_t round = 0; round < rounds; round++) {
    tb::TestCase tc;

    // Create packet with some arbitrary length.
    const std::size_t beats = tb::Random::uniform<std::size_t>(1500 / 8, 1);

    // In:
    for (std::size_t i = 0; i < beats; i++) {
      tb::In in;

      in.valid = true;
      in.sop = (i == 0);
      in.eop = (i == (beats - 1));
      in.length = in.eop ? 7 : 0;
      in.data = tb::Random::uniform<vluint64_t>();

      tc.in.push_back(in);
    }

    // Out:
    for (std::size_t i = 0; i < beats; i++) {
      tb::Out out;
      out.valid = true;
      out.sop = (i == 0);
      out.eop = (i == (beats - 1));
      out.length = out.eop ? 7 : 0;
      // In -> Out; data is not changed.
      out.data = tc.in[i].data;
      out.buffer = out.eop ? buffer : 0;
      tc.out.push_back(out);
    }

    // Packet type
    tb::PacketType t;
    t.off = 0;
    t.type = tc.in[0].data & 0xFFFFFFFF;
    tc.type = t;

    std::vector<tb::SymbolMatch> m(4);
    // Some arbitrary slot within the match set.
    const std::size_t pos = tb::Random::uniform<std::size_t>(3, 0);
    const std::size_t index = tb::Random::uniform<std::size_t>(beats - 1, 0);
    m[pos].valid = true;
    m[pos].off = index;
    m[pos].match = tc.in[index].data;
    m[pos].buffer = buffer;
    tc.match = m;

    tests.push_back(tc);
  }
  
  tb::TB tb(opts);
  tb.run(tests);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
