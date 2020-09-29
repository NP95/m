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

`ifndef M_RTL_M_PKG_VH
`define M_RTL_M_PKG_VH

package m_pkg;

  // Length type
  typedef logic [2:0] len_t;

  // From a len, derive the corresponding unary (thermometer) mask.
  function logic [7:0] len_to_unary_mask(len_t len); begin
    // TBD: verify that len == 0, results in 'b1.
    return ('b1 << int'(len) + 'b1)  - 'b1;
  end endfunction

  // Data type
  typedef logic [7:0][7:0] data_t;

  // Input packet type
  typedef struct packed {
    logic        sop;
    logic        eop;
    len_t        length;
    data_t       data;
  } in_t;

  // "Buffer" match token type
  typedef logic [7:0] buffer_t;

  // Output packet type
  typedef struct packed {
    logic        sop;
    logic        eop;
    len_t        length;
    data_t       data;
    buffer_t     buffer;
  } out_t;

  // Packet type, type.
  typedef logic [3:0][7:0] packet_type_t;

  typedef logic [7:0]      packet_word_off_t;

  // Match definition.
  typedef struct packed {
    // Match validity
    logic        valid;
    // Match word offet
    packet_word_off_t off;
    // Matching 8B token
    data_t       match;
    // Resultant key emitted to host on successful match.
    buffer_t     buffer;
  } sym_match_t;
  
  // Packet offset location type
  typedef struct packed {
    packet_word_off_t   word;
    logic [2:0]         off;
  } packet_off_t;

endpackage // m_pkg

`endif
