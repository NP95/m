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

#include "tb.h"
#include "vobj/Vtb.h"
#ifdef OPT_ENABLE_VCD
#  include "verilated_vcd_c.h"
#endif
#include "gtest/gtest.h"

namespace tb {

void Random::init(unsigned seed) {
  mt_ = std::mt19937{seed};
}

struct InDriver {
  static void drive(Vtb* tb) {
    drive(tb, In{});
  }

  static void drive(Vtb* tb, const In& in) {
    tb->in_vld_w = in.valid;
    tb->in_sop_w = in.sop;
    tb->in_eop_w = in.eop;
    tb->in_length_w = in.length;
    tb->in_data_w = in.data;
  }
};

struct OutMonitor {
  static Out get(Vtb* tb) {
    Out out;
    out.valid = tb->out_vld_r;
    out.sop = tb->out_sop_r;
    out.eop = tb->out_eop_r;
    out.length = tb->out_length_r;
    out.data = tb->out_data_r;
    out.buffer = tb->out_buffer_r;
    return out;
  }
};

struct PacketTypeDriver {
  static void drive(Vtb* tb) {
    drive(tb, PacketType{});
  }

  static void drive(Vtb* tb, const PacketType& t) {
    tb->packet_type_off_w = t.off;
    tb->packet_type_w = t.type;
  }
};

struct SymbolMatchDriver {
  static void drive(Vtb* tb) {
    std::vector<SymbolMatch> matches(4);
    drive(tb, matches);
  }

  static void drive(Vtb* tb, const std::vector<SymbolMatch>& m) {
    for (std::size_t i = 0; i < m.size(); i++) {
      switch (i) {
        case 0: {
          tb->match0_vld_w = m[0].valid;
          tb->match0_off_w = m[0].off;
          tb->match0_match_w = m[0].match;
          tb->match0_buffer_w = m[0].buffer;
        } break;
        case 1: {
          tb->match1_vld_w = m[1].valid;
          tb->match1_off_w = m[1].off;
          tb->match1_match_w = m[1].match;
          tb->match1_buffer_w = m[1].buffer;
        } break;
        case 2: {
          tb->match2_vld_w = m[2].valid;
          tb->match2_off_w = m[2].off;
          tb->match2_match_w = m[2].match;
          tb->match2_buffer_w = m[2].buffer;
        } break;
        case 3: {
          tb->match3_vld_w = m[3].valid;
          tb->match3_off_w = m[3].off;
          tb->match3_match_w = m[3].match;
          tb->match3_buffer_w = m[3].buffer;
        } break;
      }
    }
  }
};

TB::TB(const Options& opts) : opts_(opts) {
#ifdef OPT_ENABLE_VCD
  if (opts.enable_vcd) {
    Verilated::traceEverOn(true);
  }
#endif
  tb_ = new Vtb("tb");
#ifdef OPT_ENABLE_VCD
  if (opts.enable_vcd) {
    vcd_ = new VerilatedVcdC;
    tb_->trace(vcd_, 99);
    vcd_->open("sim.vcd");
  }
#endif
}

TB::~TB() {
  delete tb_;
  if (vcd_) {
    vcd_->close();
    delete vcd_;
  }
}

void TB::run(std::deque<TestCase>& tests) {
  tb_->clk_net = false;
  tb_->rst_net = false;
  
  tb_->clk_host = false;
  tb_->rst_host = false;

  // Drive various interfaces to idle.
  InDriver::drive(tb_);
  PacketTypeDriver::drive(tb_);
  SymbolMatchDriver::drive(tb_);

  net_context_.state = NetState::PreReset;
  net_context_.reset_ticks = 10;

  host_context_.state = HostState::PreReset;
  host_context_.reset_ticks = 10;

  time_ = 0;
  while (!sim_context_.stopped) {
    time_++;

    if (time_ % 10 == 0) {
      if (tb_->clk_net) {
        // Testbench drives on the negative edge of the clock edge
        // for readability in the waveform; no functional impact.
        on_net_clk_negedge(tests);
      }
      tb_->clk_net = !tb_->clk_net;
    }
    if (time_ % 10 == 0) {
      // Testbench samples RTL on negative edge of the host clock to
      // avoid synchronization issues with the RTL.
      if (tb_->clk_host) {
        on_host_clk_negedge();
      }
      tb_->clk_host = !tb_->clk_host;
    }
    tb_->eval();
#ifdef OPT_ENABLE_VCD
    if (vcd_) {
      vcd_->dump(time_);
    }
#endif
  }
}

void TB::on_net_clk_negedge(std::deque<TestCase>& tests) {
  switch (net_context_.state) {
    case NetState::PreReset: {
      tb_->rst_net = true;
      net_context_.state = NetState::InReset;
    } break;
    case NetState::InReset: {
      if (--net_context_.reset_ticks == 0) {
        tb_->rst_net = false;
        net_context_.state = NetState::Active;
      }
    } break;
    case NetState::Active: {
      // Drive to idle.
      InDriver::drive(tb_);
      PacketTypeDriver::drive(tb_);
      SymbolMatchDriver::drive(tb_);

      std::deque<In>& ins = sim_context_.actual_in;
      if (ins.empty()) {
        // Start new test
        if (tests.empty()) {
          // Simulus exhausted; wind-down simulation awaiting state
          // which is currently inflight to be emitted.
          net_context_.state = NetState::PostActive;
          net_context_.reset_ticks = 10;
          return;
        }

        TestCase& test = tests.front();
        for (const In& in : test.in) {
          ins.push_back(in);
        }
        for (const Out& out : test.out) {
          std::deque<Out>& outs{sim_context_.expected_out};
          outs.push_back(out);
        }
        PacketTypeDriver::drive(tb_, test.type);
        SymbolMatchDriver::drive(tb_, test.match);
        tests.pop_front();
      }
      InDriver::drive(tb_, ins.front());
      ins.pop_front();
    } break;
    case NetState::PostActive: {
      // Wind down simulation
      if (--net_context_.reset_ticks == 0) {
        sim_context_.stopped = true;
      }
    } break;
  }
}

void TB::on_host_clk_negedge() {
  bool ret = true;
  switch (host_context_.state) {
    case HostState::PreReset: {
      tb_->rst_host = true;
      host_context_.state = HostState::InReset;
    } break;
    case HostState::InReset: {
      if (--host_context_.reset_ticks == 0) {
        tb_->rst_host = false;
        host_context_.state = HostState::Active;
      }
    } break;
    case HostState::Active: {
      const Out actual = OutMonitor::get(tb_);
      if (actual.valid) {
        std::deque<Out>& outs{sim_context_.expected_out};

        // Error out immediately if receiving unexpected output.
        ASSERT_FALSE(outs.empty());
        
        const Out& expected{outs.front()};

        // Validate actual vs. expected.
        EXPECT_EQ(expected.sop, actual.sop);
        EXPECT_EQ(expected.eop, actual.eop);
        EXPECT_EQ(expected.length, actual.length);
        EXPECT_EQ(expected.data, actual.data);
        EXPECT_EQ(expected.buffer, actual.buffer);

        outs.pop_front();
      }
    } break;
  }
}

} // namespace tb
