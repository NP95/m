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

`default_nettype none
`timescale 1ns/1ps

module sync_ff #(parameter int W = 1) (/*AUTOARG*/
                                       // Outputs
                                       q,
                                       // Inputs
                                       d, clk, rst
                                       );

  // The is a ASIC-like/behavior version of a clock-domain
  // synchronizer. Although there is a logical path between D -> Q,
  // from a timing perspecitve this is known as a "false-path" and can
  // be disregarded. In an FPGA setting, one would not typically infer
  // ones own synchronizer and would instead instantiate the devices
  // synchronizer primitive.
  
  typedef logic [W-1:0] w_t;

  input [W-1:0]         d;
  output logic [W-1:0]  q;
  input                 clk;
  input                 rst;

  w_t d_1, d_2;

  always_ff @(posedge clk) begin
    if (rst) begin
      d_1 <= 'b0;
      d_2 <= 'b0;
    end else begin
      d_1 <= d;
      d_2 <= d_1;
    end
  end

  always_comb q = d_2;

endmodule // sync_ff
