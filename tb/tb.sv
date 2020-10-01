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

module tb (

  // ======================================================================== //
  // Ingress
    input                                         in_vld_w
  , input logic                                   in_sop_w
  , input logic                                   in_eop_w
  , input m_pkg::len_t                            in_length_w
  , input m_pkg::data_t                           in_data_w

  // ======================================================================== //
  // Egress
  , output logic                                  out_vld_r
  , output logic                                  out_sop_r
  , output logic                                  out_eop_r
  , output m_pkg::len_t                           out_length_r
  , output m_pkg::data_t                          out_data_r
  , output m_pkg::buffer_t                        out_buffer_r

  // ======================================================================== //
  // Packet type interface
  , input m_pkg::packet_off_t                     packet_type_off_w
  , input m_pkg::packet_type_t                    packet_type_w

  // ======================================================================== //
  // Match interface

  // Interface 0
  , input                                         match0_vld_w
  , input m_pkg::packet_word_off_t                match0_off_w
  , input m_pkg::data_t                           match0_match_w
  , input m_pkg::buffer_t                         match0_buffer_w

  // Interface 1
  , input                                         match1_vld_w
  , input m_pkg::packet_word_off_t                match1_off_w
  , input m_pkg::data_t                           match1_match_w
  , input m_pkg::buffer_t                         match1_buffer_w

  // Interface 2
  , input                                         match2_vld_w
  , input m_pkg::packet_word_off_t                match2_off_w
  , input m_pkg::data_t                           match2_match_w
  , input m_pkg::buffer_t                         match2_buffer_w

  // Interface 3
  , input                                         match3_vld_w
  , input m_pkg::packet_word_off_t                match3_off_w
  , input m_pkg::data_t                           match3_match_w
  , input m_pkg::buffer_t                         match3_buffer_w

  // ======================================================================== //
  // Clk/Reset
  , input                                         clk_net
  , input                                         rst_net
  //
  , input                                         clk_host
  , input                                         rst_host
);

  //
  m_pkg::in_t                      in_w;
  m_pkg::out_t                     out_r;

  m_pkg::sym_match_t [3:0]         symbol_match_w;

  // ------------------------------------------------------------------------ //
  //
  always_comb begin : in_PROC

    in_w                       = '0;
    in_w.sop                   = in_sop_w;
    in_w.eop                   = in_eop_w;
    in_w.length                = in_length_w;
    in_w.data                  = in_data_w;

    symbol_match_w             = '0;

    //
    symbol_match_w [0].valid   = match0_vld_w;
    symbol_match_w [0].off     = match0_off_w;
    symbol_match_w [0].match   = match0_match_w;
    symbol_match_w [0].buffer  = match0_buffer_w;

    //
    symbol_match_w [1].valid   = match1_vld_w;
    symbol_match_w [1].off     = match1_off_w;
    symbol_match_w [1].match   = match1_match_w;
    symbol_match_w [1].buffer  = match1_buffer_w;

    //
    symbol_match_w [2].valid   = match2_vld_w;
    symbol_match_w [2].off     = match2_off_w;
    symbol_match_w [2].match   = match2_match_w;
    symbol_match_w [2].buffer  = match2_buffer_w;

    //
    symbol_match_w [3].valid   = match3_vld_w;
    symbol_match_w [3].off     = match3_off_w;
    symbol_match_w [3].match   = match3_match_w;
    symbol_match_w [3].buffer  = match3_buffer_w;
        
  end // block: in_PROC

  // ------------------------------------------------------------------------ //
  //
  m u_m (
    //
      .in_vld_w               (in_vld_w                )
    , .in_w                   (in_w                    )
    //
    , .out_vld_r              (out_vld_r               )
    , .out_r                  (out_r                   )
    //
    , .packet_type_off_w      (packet_type_off_w       )
    , .packet_type_w          (packet_type_w           )
    //
    , .symbol_match_w         (symbol_match_w          )
    //
    , .clk_net                (clk_net                 )
    , .rst_net                (rst_net                 )
    //
    , .clk_host               (clk_host                )
    , .rst_host               (rst_host)
  );

  // ------------------------------------------------------------------------ //
  //
  always_comb begin : out_PROC

    out_sop_r     = out_r.sop;
    out_eop_r     = out_r.eop;
    out_length_r  = out_r.length;
    out_data_r    = out_r.data;
    out_buffer_r  = out_r.buffer;

  end // block: out_PROC
  
endmodule // tb
