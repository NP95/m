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
  , input m_pkg::sym_match_t [3:0]                symbol_match_w

  // ======================================================================== //
  // Clk/Reset
  , input                                         clk_net
  , input                                         rst_net

  , input                                         clk_host
  , input                                         rst_host
);

  // ======================================================================== //
  //                                                                          //
  // Types                                                                    //
  //                                                                          //
  // ======================================================================== //

  typedef enum logic [1:0] { IDLE       = 2'b00,
                             IN_PACKET  = 2'b01
                             } state_t;

  typedef struct packed {
    logic                got_type;
    logic                got_symbol;
    m_pkg::buffer_t      buffer;
  } match_t;

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
  logic                                 fsm_can_match;

  logic                                 net_out_vld;
  m_pkg::out_t                          net_out;

  // AFIFO
  logic                                 afifo_push;
  m_pkg::out_t                          afifo_push_data;
  logic                                 afifo_pop;
  m_pkg::out_t                          afifo_pop_data;
  logic                                 afifo_empty_r;
  logic                                 afifo_empty_w;

  // FSM oprands:
  m_pkg::packet_off_t                   packet_type_off_r;
  m_pkg::packet_type_t                  packet_type_r;
  m_pkg::sym_match_t [3:0]              symbol_match_r;

  // match_packet_type_PROC
  logic                                 match_type_in_word;
  logic [7:0]                           match_valid_mask;
  logic                                 match_type_off_in_range;
  logic [4:0]                           match_type_found_pos;
  logic                                 match_type_found;

  // match_symbol_PROC
  logic                                 match_symbol_can_match_word;
  logic [3:0]                           match_symbol_can_match;
  logic                                 match_symbol_did_match;
  logic                                 match_symbol_found;
  m_pkg::buffer_t                       match_symbol_buffer;

  // match concensus
  match_t                               match_r;
  match_t                               match_w;
  logic                                 match_en;

  logic                                 match_got_type;
  logic                                 match_got_symbol;
  m_pkg::buffer_t                       match_buffer;

  // ======================================================================== //
  //                                                                          //
  // Comb.                                                                    //
  //                                                                          //
  // ======================================================================== //
  
  // ------------------------------------------------------------------------ //
  //
  always_comb begin : in_PROC

    // Latch input.
    in_en  = in_vld_w;

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

    // Flag denotes whether the word present in in_r contains state
    // that is currently matchable (contains valid payload data). This
    // is derived from the current FSM state.
    //
    fsm_can_match     = 'b0;

    // Outputs to AFIFO
    net_out_vld       = 'b0;

    // For all fields aside from 'buffer' simply forward input to
    // output.
    net_out           = '0;
    net_out.sop       = in_r.sop;
    net_out.eop       = in_r.eop;
    net_out.length    = in_r.length;
    net_out.data      = in_r.data;

    // FSM state update:
    //
    case (fsm_state_r)
      IDLE: begin
        casez ({in_vld_r, in_r.sop, in_r.eop})
          3'b1_1_0: begin
            // Detected pakcet that is > 8B in length

            // Advance packet offset
            fsm_word_off_inc  = 'b1;

            // Is matchable word
            fsm_can_match      = 'b1;

            // Drive output
            net_out_vld       = 'b1;

            // Advance to packet body state
            fsm_state_en      = 'b1;
            fsm_state_w       = IN_PACKET;
          end
          3'b1_1_1: begin
            // Detected packet that is <= 8B in length

            // Drive buffer as this is a valid EOP.
            fsm_buffer_set  = 'b1;

            // Is matchable word
            fsm_can_match   = 'b1;

            // Drive output
            net_out_vld     = 'b1;

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

            // Is matchable word
            fsm_can_match     = 'b1;

            // Drive output
            net_out_vld       = 'b1;

            // Remain in current state
          end
          2'b1_1: begin
            // Final word in current packet.

            // Drive buffer as this is a valid EOP
            fsm_buffer_set  = 'b1;

            // Is matchable word
            fsm_can_match    = 'b1;

            // Drive output
            net_out_vld     = 'b1;

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
    //
    fsm_word_off_en   = (fsm_oprand_en | fsm_word_off_inc);
    fsm_word_off_w    = fsm_oprand_en ? '0 : fsm_word_off_r + 'd1;

    // Compute 'got match' status as a function of the word on the current
    // cycle, or the matched status retained from prior cycles.
    //
    match_got_type    =
      match_r.got_type | (fsm_can_match ? match_type_found : '0);
    match_got_symbol  =
      match_r.got_symbol | (fsm_can_match ? match_symbol_found : '0);
    match_buffer      =
      (fsm_can_match & match_symbol_found) ? match_symbol_buffer : match_r.buffer;
    
    // Drive computed 'buffer' oprand based upon whether a match has
    // been encountered during the current packet.
    //
    casez ({fsm_buffer_set, match_got_type, match_got_symbol})
      3'b1_1_1:
        // Assign matched buffer on EOP if type and symbol have both
        // been detected in the payload
        net_out.buffer  = match_buffer;
      default:
        // Otherwise, match conditions were not met, drive zero.
        net_out.buffer   = '0;
    endcase // casez ({fsm_buffer_set})
    
  end // block: fsm_PROC
  
  // ------------------------------------------------------------------------ //
  // Type is a 4B quantity which is constrained to fall within a
  // single 8B word. The type field may not straddle multiple
  // words. Therefore, the type field may be aligned to the following
  // locations within the word:
  //
  //   [3:0], [4:1], [5:2], [6:3], [7:4]
  //
  // Locations:
  //
  //   [<=2:A] and [B:>=5]
  //
  // are impermissible and are therefore ingored.
      
  always_comb begin : match_type_PROC
    
    // Mask denoting the valid bytes within the current word. Length
    // is only considered on EOP.
    //
    match_valid_mask    =
      in_r.eop ? m_pkg::len_to_unary_mask(in_r.length) : '1;

    // Flag denoting that the current type is expected somewhere
    // within the current word.
    //
    match_type_in_word  = (fsm_word_off_r == packet_type_off_r.word);

    // Byte comparison logic for each of the valid matching regions.
    //
    for (int i = 0; i < 5; i++) begin
      match_type_found_pos [i]  = '1;
      for (int j = 0; j < 4; j++) begin
        match_type_found_pos[i] &= match_valid_mask[i + j]
          ? (packet_type_r[j] == in_r.data[i + j]) : '0;
      end
    end

    // The type falls within a single 8B word and is 4B in length. As
    // such, the only permissible starting locations for the type in
    // the 8B word are: 0, 1, 2, 3, 4 (as above).
    //
    match_type_off_in_range  = (packet_type_off_r.off <= 'd4);

    // Compute final type match within current word.
    //
    casez ({in_vld_r, match_type_in_word, match_type_off_in_range})
      3'b1_1_1:
        // Possibly found whenever current word is valid and we are in the
        // word where the type is expected to be found.
        match_type_found  = match_type_found_pos [packet_type_off_r.off];
      default:
        // Otherwise, not found:
        match_type_found  = 'b0;
    endcase

  end // block: match_type_PROC

  // ------------------------------------------------------------------------ //
  //
  // Block to detect the presence of the associate 'match'
  // symbol.
  //
  //  Caveat: The problem statement makes no explicit reference
  // to the expected alignment of the 'symbol'. As the preceeding
  // operation is constrained such that the type may not straddle
  // multiple 8B words, it is too assumed that this remains the case
  // here.
  always_comb begin : match_symbol_PROC

    // A symbol is 8B therefore a match can occur only when the entire
    // 8B word is valid.
    //
    match_symbol_can_match_word  = (~in_r.eop) | (in_r.length == 'd7);

    // Each match entity contains a specific SYMBOL_OFFSET value which
    // denotes the word in which the match operation can take
    // place. The match is not attempted it the current word is not at
    // the required location. Caveat: I have added an additional valid
    // field to the structure such that the match will not take place
    // unless the value has been appropriately configured.
    //
    for (int i = 0; i < 4; i++) begin
      match_symbol_can_match [i]  =
        symbol_match_r [i].valid &
         (fsm_word_off_r == symbol_match_r [i].off);
    end

    // Flag denoting when a match occurred in the current word (an
    // explicit flag is necessary here as if this detection was only
    // carried out on the "_buffer" value when non-zero, the logic
    // could not detect the buffer == '0 case).
    //
    match_symbol_did_match     = 'b0;

    // Comparator logic:
    //
    // Note: this is a 4-Way priority decoded structure. The problem
    // statement does not specifically reference what to do whenever
    // multiple matches occur in the same cycle. It would be assumed
    // that this case would represent a misconfigured system which
    // would not occur in practice, however to prevent corruption on
    // the key, the code simply selects the highest endian match.
    //
    match_symbol_buffer        = '0;
    for (int i = 0; i < 4; i++) begin
      if (match_symbol_can_match [i] &&
         (in_r.data == symbol_match_r [i].match)) begin
        match_symbol_did_match |= 'b1;
        match_symbol_buffer     = symbol_match_r [i].buffer;
      end
    end

    // Compute final match decision.
    //
    casez ({in_vld_r, match_symbol_can_match_word})
      2'b1_1:
        // Take the did_match value if the current word is
        // "matchable".
        match_symbol_found  = match_symbol_did_match;
      default:
        // Otherwise, no match took place.
        match_symbol_found  = 'b0;
    endcase // casez ({in_vld_r, match_symbol_can_match})
    
  end // block: match_symbol_PROC

  // ------------------------------------------------------------------------ //
  //
  always_comb begin : match_PROC

    //
    casez ({fsm_oprand_en, fsm_can_match})
      2'b1_?:
        // Clear retained state
        match_en  = 'b1;
      2'b0_1:
        // Latch update state as necessary.
        match_en  = (match_type_found | match_symbol_found);
      default:
        // Otherwise, retain prior
        match_en  = 'b0;
    endcase // casez ({fsm_oprand_en, fsm_can_match})

    // Defaults:
    match_w  = match_r;

    // Updates
    casez ({fsm_oprand_en, fsm_can_match})
      2'b1_?:
        // New packet in next cycle therefore clear retained state.
        match_w   = 'b0;
      2'b0_1: begin
        // In a matchable word, therefore latch any matches.
        
        if (match_type_found)
          match_w.got_type  = 'b1;

        if (match_symbol_found) begin
          match_w.got_symbol  = 'b1;
          match_w.buffer  = match_symbol_buffer;
        end

      end
      default:
        // Otherwise, retain
        match_w  = match_r;
    endcase

  end // block: match_PROC
  
  // ------------------------------------------------------------------------ //
  //
  always_comb begin : afifo_PROC

    // Push from FSM
    afifo_push       = net_out_vld;
    afifo_push_data  = net_out;

    // Self-pop whenever non-empty.
    afifo_pop        = (~afifo_empty_r);

  end // block: afifo_PROC
  
  // ------------------------------------------------------------------------ //
  //
  always_comb begin : out_PROC

    // Emit output on pop
    out_vld_w  = afifo_pop;

    // Latch output state
    out_en     = afifo_pop;
    out_w      = afifo_pop_data;

  end // block: out_PROC
  
  // ======================================================================== //
  //                                                                          //
  // Flops                                                                    //
  //                                                                          //
  // ======================================================================== //

  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk_net)
    if (rst_net)
      in_vld_r <= 'b0;
    else
      in_vld_r <= in_vld_w;

  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk_net)
    if (in_en)
      in_r <= in_w;
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk_net)
    if (fsm_word_off_en)
      fsm_word_off_r <= fsm_word_off_w;
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk_net)
    if (rst_net)
      fsm_state_r <= IDLE;
    else if (fsm_state_en)
      fsm_state_r <= fsm_state_w;
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk_net)
    if (fsm_oprand_en) begin
      packet_type_off_r <= packet_type_off_w;
      packet_type_r     <= packet_type_w;

      symbol_match_r    <= symbol_match_w;
    end
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk_net)
    if (match_en)
      match_r <= match_w;
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk_host)
    if (rst_net)
      out_vld_r <= 'b0;
    else
      out_vld_r <= out_vld_w;
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk_host)
    if (out_en)
      out_r <= out_w;

  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge clk_host)
    if (rst_host)
      afifo_empty_r <= 'b1;
    else
      afifo_empty_r <= afifo_empty_w;
  
  // ======================================================================== //
  //                                                                          //
  // Instances                                                                //
  //                                                                          //
  // ======================================================================== //

  // ------------------------------------------------------------------------ //
  // Asynchronous queue to communciate packet data from the network
  // clock to the host clock. As host clock > network clock, and in
  // the absence of back pressure, a queue with greater than two
  // entries is gaurenteed not to overflow. There are perhaps other
  // cheaper options that could be used here, but an asynchronous fifo
  // is a common primitive that one would expect to be an
  // "off-the-shelf" IP that can be easily generated in an FPGA
  // context.
  //
  // Notes: the decision here was to perform the packet parsing in the
  // slower network clock-domain as there is potentially some small
  // (probably neglible) power saving to be had from performing this
  // on a slower clock.
  //
  afifo #(.W($bits(m_pkg::out_t)), .N(16)) u_afifo (
    //
      .wclk                   (clk_net                 )
    , .wrst                   (rst_net                 )
    //
    , .rclk                   (clk_host                )
    , .rrst                   (rst_host                )
    //
    , .push                   (afifo_push              )
    , .push_data              (afifo_push_data         )
    //
    , .pop                    (afifo_pop               )
    , .pop_data               (afifo_pop_data          )
    //
    , .empty_w                (afifo_empty_w           )
    , .full_w                 ()
  );

endmodule // m
