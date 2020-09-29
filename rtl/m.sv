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

`include "m_pkg.vh"

module m (

  // ======================================================================== //
  // Ingress
    input logic                                   in_vld_w
  , input m_pkg::in_t                             in_w

  // ======================================================================== //
  // Egress
  , output logic                                  out_vld_r
  , output m_pkg::out_t                           out_r

  // ======================================================================== //
  // Packet type match oprand
  //
  , input m_pkg::packet_off_t                     packet_type_off_w
  , input m_pkg::packet_type_t                    packet_type_w

  // ======================================================================== //
  // Mystery ID oprand
  , input m_pkg::sym_match_t                      sym_match_w

  // ======================================================================== //
  // Clk/Reset
  , input                                         clk
  , input                                         rst
);

  // ======================================================================== //
  //                                                                          //
  // Types                                                                    //
  //                                                                          //
  // ======================================================================== //

  typedef enum logic [1:0] { IDLE       = 2'b00,
                             IN_PACKET  = 2'b01
                             } state_t;

  // ======================================================================== //
  //                                                                          //
  // Wires                                                                    //
  //                                                                          //
  // ======================================================================== //

  // Input flops
  logic                                 in_vld_r;
  logic                                 in_en;
  m_pkg::in_t                           in_r;

  // Output flops
  logic                                 out_vld_w;
  logic                                 out_en;
  m_pkg::out_t                          out_w;

  // FSM:
  logic                                 fsm_state_en;
  state_t                               fsm_state_r;
  state_t                               fsm_state_w;
  logic                                 fsm_oprand_en;
  logic                                 fsm_word_off_en;
  m_pkg::packet_word_off_t              fsm_word_off_r;
  m_pkg::packet_word_off_t              fsm_word_off_w;
  logic                                 fsm_word_off_inc;
  logic                                 fsm_buffer_set;

  // FSM oprands:
  m_pkg::packet_off_t                   packet_type_off_r;
  m_pkg::packet_type_t                  packet_type_r;
  m_pkg::sym_match_t                    sym_match_r;

  // match_packet_type_PROC
  logic                                 matched_packet_type;

  // ======================================================================== //
  //                                                                          //
  // Comb.                                                                    //
  //                                                                          //
  // ======================================================================== //
  
  // ------------------------------------------------------------------------ //
  //
  always_comb begin : in_PROC

    in_en  = in_vld_r;

  end // block: in_PROC
  
  // ------------------------------------------------------------------------ //
  //
  always_comb begin : fsm_PROC

    // Defaults
    fsm_state_en      = 'b0;
    fsm_state_w       = fsm_state_r;

    // Latch oprands on cycle preceeding SOP at the input latch such
    // that the the state is preloaded for the first word.
    fsm_oprand_en     = (in_vld_w & in_w.sop);

    fsm_word_off_inc  = 'b0;

    fsm_buffer_set    = 'b0;

    out_vld_w         = 'b0;
    out_en            = '0;

    out_w             = '0;
    out_w.sop         = in_r.sop;
    out_w.eop         = in_r.eop;
    out_w.length      = in_r.length;
    out_w.data        = in_r.data;
    
    case (fsm_state_r)
      IDLE: begin
        casez ({in_vld_r, in_r.sop, in_r.eop})
          3'b1_1_0: begin
            // Detected pakcet that is > 8B in length

            // Drive output
            out_vld_w         = 'b1;
            out_en            = 'b1;

            // Advance packet offset
            fsm_word_off_inc  = 'b1;

            // Advance to packet body state
            fsm_state_en      = 'b1;
            fsm_state_w       = IN_PACKET;
          end
          3'b1_1_1: begin
            // Detected packet that is <= 8B in length

            // Drive buffer as this is a valid EOP.
            fsm_buffer_set  = 'b1;

            // Drive output
            out_vld_w       = 'b1;
            out_en          = 'b1;

            // Remain in IDLE state
          end
          default: begin
            // Otherwise, synchronize to the most recent SOP.
          end
        endcase
      end
      IN_PACKET: begin
        casez ({in_vld_r, in_r.eop})
          2'b1_0: begin
            // Word within the body of the current packet (not the
            // tail word).
            fsm_word_off_inc  = 'b1;
          end
          2'b1_1: begin
            // Final word in current packet.

            // Drive buffer as this is a valid EOP
            fsm_buffer_set  = 'b1;

            // Return to IDLE state; packet is complete.
            fsm_state_en    = 'b1;
            fsm_state_w     = IDLE;
          end
          default: begin
            // Otherwise, ignore cycle with no valid state.
          end
        endcase
      end
      default: begin
      end
    endcase // case (state_r)

    // Counter denoting the current 8B word within the current packet.
    fsm_word_off_en  = (fsm_oprand_en | fsm_word_off_inc);
    fsm_word_off_w   = fsm_oprand_en ? '0 : fsm_word_off_r + 'd1;

    // Drive computed 'buffer' oprand based upon whether a match has
    // been encountered during the current packet.
    casez ({fsm_buffer_set})
      default: out_w.buffer  = '0;
    endcase

  end // block: fsm_PROC
  
  // ------------------------------------------------------------------------ //
  //
  function logic [7:0] to_one_hot(m_pkg::len_t len); begin
    return ('b1 << len)  - 'b1;
  end endfunction

  logic [7:0] match_in_vld_1h;
  logic [3:0] match_type_found;
      
  always_comb begin : match_PROC

    match_in_vld_1h  = to_one_hot(in_r.length);

  end // block: match_PROC

  // ======================================================================== //
  //                                                                          //
  // Flops                                                                    //
  //                                                                          //
  // ======================================================================== //

  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk)
    if (rst)
      in_vld_r <= 'b0;
    else
      in_vld_r <= in_vld_w;

  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk)
    if (in_en)
      in_r <= in_w;
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk)
    if (fsm_word_off_en)
      fsm_word_off_r <= fsm_word_off_w;
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk)
    if (rst)
      fsm_state_r <= IDLE;
    else if (fsm_state_en)
      fsm_state_r <= fsm_state_w;
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk)
    if (fsm_oprand_en) begin
      packet_type_off_r <= packet_type_off_w;
      packet_type_r     <= packet_type_w;

      sym_match_r       <= sym_match_w;
    end
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk)
    if (rst)
      out_vld_r <= 'b0;
    else
      out_vld_r <= out_vld_w;
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk)
    if (out_en)
      out_r <= out_w;

endmodule // m

  
