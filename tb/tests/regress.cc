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
#include "../tb.h"
#include "../utility.h"
#include <deque>
#include <string>
#include <random>
#include <iostream>

template<typename T>
class UniqueRandomIntegral {
 public:
  UniqueRandomIntegral(T hi = std::numeric_limits<T>::max(),
                       T lo = std::numeric_limits<T>::min())
      : hi_(hi), lo_(lo) {}

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

  // Enable build logging
  bool logging_enable = false;

  void build(std::deque<tb::TestCase>& tc) {
    for (std::size_t i = 0; i < n; i++) {
      tb::TestCase t;
      t.id = i;
      generate_testcase(t);
      tc.push_back(t);
#ifndef OPT_ENABLE_LOGGING
      if (logging_enable) {
        std::cout << "[Regress] Generate testcase: " << t.to_string() << "\n";
      }
#endif
    }
  }

 private:
  void generate_testcase(tb::TestCase& tc) const {

    // Generate stimulus
    std::int64_t bytes = tb::Random::uniform<std::size_t>(max_len, 1);

    // Set meta-data
    tc.bytes = bytes;

    // Generate input stimulus; interleave packet with some empty
    // bubble cycles to emulate flow-control on the channel.
    UniqueRandomIntegral<vluint64_t> gen_data;
    for (std::size_t i = 0; bytes > 0; ) {
      tb::In in;
      // Constrain stimulus such that bubble cannot occur on the SOP
      const bool is_bubble =
          (i != 0) && tb::Random::boolean(bubble_probability);
      if (!is_bubble) {
        // SOP on first word
        in.valid = true;
        in.sop = (i == 0);
        in.eop = (bytes <= 8);
        in.length = in.eop ? (bytes - 1) : 0;
        in.data = gen_data();
        if (in.eop) { in.data &= tb::utility::mask<vluint64_t>(bytes * 8); }
        i++;
        bytes -= 8;
      }
      // Otherwise, bubble; Insert empty word.
      
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
        out.buffer = 0;

        tc.out.push_back(out);
      }
    }

    bool fail = false;
    
    // Generate type oprand
    if (generate_type(tc)) { fail = true; }

    // Generate symbol table oprand
    if (generate_symbol_table(tc, gen_data)) { fail = true; }

    tb::Out& out = tc.out.back();
    if (fail) { out.buffer = 0; }

    // Update testcase meta-data
    tc.should_match = !fail;
    if (tc.should_match) { tc.predicted_match = out.buffer; }
  }

  // For the input, select some random 4B value within a word and set the
  // type field.
  bool generate_type(tb::TestCase& tc) const {
    bool fail = tb::Random::boolean(fail_match_probability);

    std::size_t word_index = tb::Random::uniform<std::size_t>(tc.out.size() - 1);

    // Generate expected offset in the word: 0, 1, 2, 3, 4.
    std::size_t off_index = tb::Random::uniform<std::size_t>(4);

    // Compute the final byte-aligned offset into the packet
    tc.type.off = (word_index * 8) + off_index;

    // Compute the 'type' field at the nominated regino
    tc.type.type = (tc.out[word_index].data >> (off_index * 8)) & 0xFFFFFFFF;

    if (fail) {
      // If require this match to fail, intentionally corrupt the match
      // word at this location so that a match cannot possibly occur.
      tc.type.type = ~tc.type.type;
    }

    // If 'type' fields falls on the last word of the packet, we need
    // to double check that the word itself constains sufficient bytes
    // to contain the type field as a function of the alignment. If
    // not, the RTL will not match against the data.
    const bool is_last_word = (word_index == tc.out.size() - 1);
    if (is_last_word) {
      const tb::Out& out = tc.out.back();
      
      if ((out.length + 1) < (off_index + 4)) { fail = true; }
    }

    return fail;
  }

  bool generate_symbol_table(tb::TestCase& tc,
                             UniqueRandomIntegral<vluint64_t>& uri) const {

    bool fail = tb::Random::boolean(fail_match_probability);
        
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

    if (match.empty()) { fail = true; }

    if (!fail) {
      // Now, generate an entry which we expect to match.
      auto it = tb::Random::select_one(match.begin(), match.end());
      it->valid = true;

      // This is a bit fiddly. When choosing a match symbol in the
      // symbol, consider the predict output packet as by this point
      // in the TB the input stimulus has already been inleaved with
      // bubbles. Would be nice to have this as some final
      // post-processing set, but would complicate matters somewhat as
      // the stimulus when then need to be retained in a list instead
      // of a simpler deque.
      const std::size_t index = tb::Random::uniform<std::size_t>(tc.out.size() - 1);
      it->off = index;
      it->match = tc.out[index].data;

      if (index == (tc.out.size() - 1)) {
        // If nominated index is the final word in the packet, a match against the
        // symbol can occur only when the final word is 8B in length. If not, the
        // match is killed.
        const tb::In& in = tc.in.back();
        fail = (in.length != 7);
      }
      // Get matching buffer if still matching
      buffer = it->buffer;
    }

    tc.match = match;

    tb::Out& out = tc.out.back();
    out.buffer = fail ? 0 : buffer;

    return fail;
  }
};

class RegressEnvironment {
 public:

  // Unique environment identifier.
  std::size_t id = 0;

  // Total number of test cases.
  std::size_t n = 1;

  // Maximum number of bytes within a packet
  std::size_t max_len = 1500;

  // Maximum number of symbols to match [0. 4]; if 0, there are no
  // matching symbols therefore no match occurs.
  std::size_t symbol_n = 4;

  // Probability of invalid words within the stream (typically low).
  double bubble_probability = 0.05;

  // Probability of a match not taking place.
  double fail_match_probability = 0.1;

  // Enable verbose logging in the testbench
  bool logging_enable = false;

  RegressEnvironment(const std::string& name, unsigned seed) : name_(name) {
    tb::Random::init(seed);
  }

  std::string name() const { return name_; }

  std::string to_string() const {
    using std::to_string;

    tb::utility::KVListRenderer r;
    r.add_field("n", to_string(n));
    r.add_field("max_len", to_string(max_len));
    r.add_field("symbol_n", to_string(symbol_n));
    r.add_field("bubble_probability", to_string(bubble_probability));
    r.add_field("fail_match_probability", to_string(fail_match_probability));
    return r.to_string();
  }

  void run() const {
    tb::Options opts;
#ifdef OPT_VCD_ENABLE
    // Enable waveforms
    opts.vcd_enable = true;
    opts.vcd_name = name_ + ".vcd";
#endif
#ifdef OPT_LOGGING_ENABLE
    opts.logging_enable = logging_enable;

    std::cout << "[Regress] Constructing test environment: "
              << to_string() << "\n";
#endif
  
    tb::TB tb(opts);
    TestcaseBuilder tcb;
#ifdef OPT_LOGGING_ENABLE
    tcb.logging_enable = logging_enable;
#endif
    tcb.n = n;
    tcb.max_len = max_len;
    tcb.symbol_n = symbol_n;
    tcb.bubble_probability = bubble_probability;
    tcb.fail_match_probability = fail_match_probability;
    
    std::deque<tb::TestCase> tests;
    tcb.build(tests);
    tb.run(tests);
  }

 private:
  // Test name
  std::string name_;
};

TEST(regress, single_word_packet) {
  // Fully randomized, self-checking testbench.
  for (std::size_t round = 0; round < 100; round++) {
    const unsigned seed = tb::Random::uniform<unsigned>();
    const std::string testname = "regress" + std::to_string(round);
    RegressEnvironment r{testname, seed};
    r.id = round;
    r.n = 1000;
    r.max_len = tb::Random::uniform<std::size_t>(8, 1);
    r.symbol_n = tb::Random::uniform<std::size_t>(4, 1);
    r.bubble_probability = tb::Random::uniform<double>(0.0, 0.2);
    r.fail_match_probability = tb::Random::uniform<double>(0.1, 0.9);
#ifdef OPT_LOGGING_ENABLE
    r.logging_enable = true;
#endif
    r.run();
  }
}

TEST(regress, full) {
  // Fully randomized, self-checking testbench.
  for (std::size_t round = 0; round < 10; round++) {
    const unsigned seed = tb::Random::uniform<unsigned>();
    const std::string testname = "regress" + std::to_string(round);
    RegressEnvironment r{testname, seed};
    r.id = round;
    r.n = 1000;
    r.max_len = tb::Random::uniform<std::size_t>(1500, 1);
    r.symbol_n = tb::Random::uniform<std::size_t>(4, 1);
    r.bubble_probability = tb::Random::uniform<double>(0.0, 0.2);
    r.fail_match_probability = tb::Random::uniform<double>(0.1, 0.9);
#ifdef OPT_LOGGING_ENABLE
    r.logging_enable = true;
#endif
    r.run();
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
