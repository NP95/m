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

module async_queue #(
     parameter integer W = 32
   , parameter integer N = 16
) (

   //======================================================================== //
   //                                                                         //
   // Misc.                                                                   //
   //                                                                         //
   //======================================================================== //

     input                                   wclk
   , input                                   wrst
   //
   , input                                   rclk
   , input                                   rrst

   //======================================================================== //
   //                                                                         //
   // Push Interface                                                          //
   //                                                                         //
   //======================================================================== //

   , input                                   push
   , input [W-1:0]                           push_data

   //======================================================================== //
   //                                                                         //
   // Pop Interface                                                           //
   //                                                                         //
   //======================================================================== //

   , input                                   pop

   , output logic [W-1:0]                    pop_data

   //======================================================================== //
   //                                                                         //
   // Control/Status Interface                                                //
   //                                                                         //
   //======================================================================== //

   //
   , output logic                            empty_w
   , output logic                            full_w
);

  typedef struct packed {
    logic                 x;
    logic [$clog2(N)-1:0] a;
  } addr_t;
  localparam int ADDR_W  = $bits(addr_t);

  // ======================================================================== //
  //                                                                          //
  // Wires                                                                    //
  //                                                                          //
  // ======================================================================== //

  //
  addr_t                                rptr_w;
  addr_t                                rptr_r;
  logic                                 rptr_en;
  //
  addr_t                                wptr_w;
  addr_t                                wptr_r;
  logic                                 wptr_en;
  //
  addr_t                                wptr_gray_w;
  addr_t                                wptr_gray_r;
  //
  addr_t                                rptr_gray_w;
  addr_t                                rptr_gray_r;
  //
  addr_t                                wptr_rsync;
  addr_t                                rptr_wsync;
  //
  addr_t                                wptr_gray_rsync_r;
  addr_t                                rptr_gray_wsync_r;
  //
  logic [N - 1:0][W - 1:0]              mem_r;

  // ======================================================================== //
  //                                                                          //
  // Combinatorial Logic                                                      //
  //                                                                          //
  // ======================================================================== //


  // ------------------------------------------------------------------------ //
  //
  always_comb begin : flags_PROC
    
    //
    empty_w  = (rptr_w == wptr_rsync);

    full_w   = (wptr_w.x ^ rptr_wsync.x) & (wptr_w.a == rptr_wsync.a);

  end // block: flags_PROC
  
  // ------------------------------------------------------------------------ //
  //
  always_comb begin : async_cntrl_PROC

    //
    wptr_w          = push ? wptr_r + 'b1 : wptr_r;
    wptr_en         = push;

    //
    rptr_w          = pop ? rptr_r + 'b1 : rptr_r;
    rptr_en         = pop;

  end // block: async_cntrl_PROC

  // ------------------------------------------------------------------------ //
  //
  always_comb begin : pop_data_PROC

    pop_data  = mem_r [rptr_r.a];

  end // block: pop_data_PROC
  
  // ======================================================================== //
  //                                                                          //
  // Flops                                                                    //
  //                                                                          //
  // ======================================================================== //

  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge wclk)
    if (wrst)
      wptr_gray_r <= '0;
    else
      wptr_gray_r <= wptr_gray_w;

  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge rclk)
    if (rrst)
      rptr_gray_r <= '0;
    else
      rptr_gray_r <= rptr_gray_w;

  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge wclk)
    if (wrst)
      wptr_r <= '0;
    else if (wptr_en)
      wptr_r <= wptr_w;

  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge rclk)
    if (rrst)
      rptr_r <= '0;
    else if (rptr_en)
      rptr_r <= rptr_w;
  
  // ------------------------------------------------------------------------ //
  //
  always_ff @(posedge wclk)
    if (push)
      mem_r [wptr_r.a] <= push_data;
  
  // ======================================================================== //
  //                                                                          //
  // Instances                                                                //
  //                                                                          //
  // ======================================================================== //

  // ------------------------------------------------------------------------ //
  //
  gray_encode #(.W(ADDR_W)) u_enc_wptr (.dec(wptr_r), .gray(wptr_gray_w));
  
  // ------------------------------------------------------------------------ //
  //
  gray_encode #(.W(ADDR_W)) u_enc_rptr (.dec(rptr_r), .gray(rptr_gray_w));
  
  // ------------------------------------------------------------------------ //
  //
  gray_decode #(.W(ADDR_W)) u_dec_wptr (.gray(wptr_gray_rsync_r), .dec(wptr_rsync));
  
  // ------------------------------------------------------------------------ //
  //
  gray_decode #(.W(ADDR_W)) u_dec_rptr (.gray(rptr_gray_wsync_r), .dec(rptr_wsync));
  
  // ------------------------------------------------------------------------ //
  //
  sync_ff #(.W(ADDR_W)) u_sync_rptr (
    //
      .clk               (wclk               )
    , .rst               (wrst               )
    //
    , .d                 (rptr_gray_r        )
    , .q                 (rptr_gray_wsync_r  )
  );

  // ------------------------------------------------------------------------ //
  //
  sync_ff #(.W(ADDR_W)) u_sync_wptr (
    //
      .clk               (rclk               )
    , .rst               (rrst               )
    //
    , .d                 (wptr_gray_r        )
    , .q                 (wptr_gray_rsync_r  )
  );

endmodule // async_queue
