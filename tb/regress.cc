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
#include <string>
#include <random>

template<typename T>
class UniqueRandomIntegral {
 public:
  UniqueRandomIntegral(T hi = std::numeric_limits<T>::max(),
                       T lo = std::numeric_limits<T>::min()) {
  }

  // Accessors:
  T hi() const { return hi_; }
  T lo() const { return lo_; }

  std::set<T>& ts() { return ts_; }
  const std::set<T>& ts() const { return ts_; }

  T operator()() {
    T t;
    do {
      // Generate random T,
      t = tb::Random::uniform(hi(), lo());
      // until successfully inserted in the set.
    } while (!ts_.insert(t).second);
    return t;
  }

 private:
  std::set<T> ts_;

  // Permissible range
  T hi_, lo_;
};

class TestcaseBuilder {
 public:
  TestcaseBuilder() = default;

  // Number of packets to generate.
  std::size_t n = 1024;

  // Maximum number of bytes within a packet
  std::size_t max_len = 1500;

  // Maximum number of symbols to match [0. 4]; if 0, there are no
  // matching symbols therefore no match occurs.
  std::size_t symbol_n = 4;

  // Probability of invalid words within the stream (typically low).
  double bubble_probability = 0.05;


  // Probability of a match not taking place.
  double fail_match_probability = 0.1;

  void build(std::deque<tb::TestCase>& tc) {
    for (std::size_t i = 0; i < n; i++) {
      tc.push_back(generate_testcase());
    }
  }

 private:
  tb::TestCase generate_testcase() const {
    tb::TestCase tc;

    // Generate stimulus
    std::int64_t bytes = tb::Random::uniform<std::size_t>(max_len, 1);

    UniqueRandomIntegral<vluint64_t> gen_data;
    while (bytes > 0) {
      tb::In in;
      const bool is_bubble = tb::Random::boolean(bubble_probability);
      if (!is_bubble) {
        // SOP on first word
        in.sop = tc.in.empty();
        in.eop = (bytes <= 8);
        if (in.eop) { in.length = bytes - 1; }
        in.data = gen_data();

        // Advance to next byte.
        bytes -= 8;
      }

      // Insert stimulus packet.
      tc.in.push_back(in);
    }


    // Generate expected output In -> Out:
    for (const tb::In& in : tc.in) {
      // Drop bubbles as we are not interested in these cases.
      if (in.valid) {
        tb::Out out;
        out.valid = true;
        out.sop = in.sop;
        out.eop = in.eop;
        out.length = in.length;
        out.data = in.data;

        tc.out.push_back(out);
      }
    }

    const bool fail_match_type =
        tb::Random::boolean(fail_match_probability);

    // Type
    generate_type(tc, fail_match_type);

    const bool fail_match_symbol =
        tb::Random::boolean(fail_match_probability);

    // Set final buffer on expected EOP.
    generate_symbol_table(tc, gen_data, fail_match_symbol);

    if (fail_match_type) {
      // If fail on type has been set, explicitly zero out the buffer has
      // this may been set during generate_symbol_table.
      tb::Out& out = tc.out.back();
      out.buffer = 0;
    }

    return tc;
  }

  // For the input, select some random 4B value within a word and set the
  // type field.
  void generate_type(tb::TestCase& tc, bool fail_match = false) const {
    std::size_t word_index = tb::Random::uniform<std::size_t>(tc.in.size() - 1);

    // Generate expected offset in the word: 0, 1, 2, 3, 4.
    std::size_t off_index = tb::Random::uniform<std::size_t>(4);

    // Compute the final byte-aligned offset into the packet
    tc.type.off = (word_index * 8) + off_index;

    // Compute the 'type' field at the nominated regino
    tc.type.type = (tc.in[word_index].data >> (off_index * 8)) & 0xFFFFFFFF;

    if (fail_match) {
      // If require this match to fail, intentionally corrupt the match
      // word at this location so that a match cannot possibly occur.
      tc.type.type = ~tc.type.type;
    }
  }

  void generate_symbol_table(tb::TestCase& tc,
                             UniqueRandomIntegral<vluint64_t>& uri,
                             bool fail_match = false) const {
    const std::size_t symbols_n = tb::Random::uniform<std::size_t>(4, 0);

    vluint8_t buffer = 0;
    std::vector<tb::SymbolMatch> match;

    // Populate symbol table with entries which are guareneed not to
    // match (from UniqueRandomIntegral).
    for (std::size_t i = 0; i < symbols_n; i++) {
      tb::SymbolMatch symbol;
      symbol.valid = true;
      symbol.off = 0;
      symbol.match = uri();
      symbol.buffer = tb::Random::uniform<vluint8_t>();
      match.push_back(symbol);
    }

    if (!fail_match) {
      // Now, generate an entry which we expect to match.
      auto it = tb::Random::select_one(match.begin(), match.end());
      it->valid = true;

      const std::size_t index = tb::Random::uniform<std::size_t>(tc.in.size() - 1);
      it->off = index;
      it->match = tc.in[index].data;

      if (index == (tc.in.size() - 1)) {
        // If nominated index is the final word in the packet, a match against the
        // symbol can occur only when the final word is 8B in length. If not, the
        // match is killed.
        const tb::In& in = tc.in.back();
        fail_match = (in.length != 7);
      }

      // Get matching buffer if still matching
      buffer = !fail_match ? it->buffer : 0;
    }

    tc.match = match;

    tb::Out& out = tc.out.back();
    out.buffer = buffer;
  }
};

class RegressEnvironment {
 public:
  RegressEnvironment(const std::string& name, unsigned seed) : name_(name) {
    tb::Random::init(seed);
  }

  std::string name() const { return name_; }

  void run() const {
    tb::Options opts;
#ifdef OPT_ENABLE_VCD
    // Enable waveforms
    opts.enable_vcd = true;
    opts.vcd_name = name_ + ".vcd";
#endif
  
    tb::TB tb(opts);
    TestcaseBuilder tcb;
    tcb.n = 1024;
    
    std::deque<tb::TestCase> tests;
    tcb.build(tests);
    tb.run(tests);
  }

 private:
  // Test name
  std::string name_;
};

TEST(regress, regress) {
  // Fully randomized, self-checking testbench.
  for (std::size_t round = 0; round < 100; round++) {
    const unsigned seed = tb::Random::uniform<unsigned>();
    const std::string testname = "regress" + std::to_string(round);
    const RegressEnvironment r{testname, seed};
    r.run();
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

