`timescale 1ns / 1ps
module ppe_h_p #(
  parameter PPE_H_WIDTH_T = 8,
  parameter PPE_H_WIDTH = 8,
  parameter PPE_H_LOG_W_T = 3,
  parameter PPE_H_LOG_W = 3 
) (
    input clk,
    input rst,
    input [PPE_H_WIDTH*PPE_H_WIDTH_T-1:0] Req,
    output [PPE_H_WIDTH*PPE_H_WIDTH_T-1:0] Gnt_o,
    output [PPE_H_WIDTH_T-1:0]valid
);

    genvar i;
    wire [PPE_H_WIDTH_T-1:0] top_req;
    reg [PPE_H_LOG_W_T-1:0] top_p_enc;
    generate 
      for (i = 0; i < PPE_H_WIDTH_T; i = i + 1) begin 
        assign top_req[i] = |Req[(i+1)*PPE_H_WIDTH-1:i*PPE_H_WIDTH];
      end
    endgenerate


    wire [PPE_H_WIDTH_T-1:0] Gnt_top;
    ppe #(
      .PPE_WIDTH(PPE_H_WIDTH_T),
      .PPE_LOG_W(PPE_H_LOG_W_T)
    ) ppe_top (
        .Req(top_req),
        .P_enc(top_p_enc),
        .Gnt(Gnt_top),
        .valid(valid_top)
    );

    wire [PPE_H_LOG_W_T-1:0] o_top;
    encoder #(
      .WIDTH(PPE_H_WIDTH_T),
      .LOG_W(PPE_H_LOG_W_T)
    ) encoder_top (
      .in(Gnt_top << 1 | Gnt_top >> (PPE_H_WIDTH_T-1)),
      .out(o_top)

    );

    // registers for pipeline 
    reg [PPE_H_WIDTH_T-1:0] Gnt_top_reg;
    reg [PPE_H_WIDTH_T*PPE_H_WIDTH-1:0] Req_reg;
    always@(posedge clk) begin 
      if (rst) begin 
        Gnt_top_reg <= 0;
        Req_reg <= 0;
      end else begin 
        Gnt_top_reg <= Gnt_top;
        Req_reg <= Req;
      end
    end



    reg [PPE_H_LOG_W-1:0] P_enc[PPE_H_WIDTH_T-1:0]; // N * P_ecn
    wire [PPE_H_WIDTH-1:0] Gnt[PPE_H_WIDTH_T-1:0];
    wire [PPE_H_LOG_W-1:0] o_value[PPE_H_WIDTH_T-1:0];

    wire [PPE_H_WIDTH-1:0] Reqi[PPE_H_WIDTH_T-1:0];
    generate 
      for (i = 0; i < PPE_H_WIDTH_T; i = i + 1) begin 
        assign Reqi[i] = Req_reg[(i+1)*PPE_H_WIDTH-1:i*PPE_H_WIDTH];
      end
    endgenerate


    integer j;
    always @(posedge clk) begin 
      if (rst) begin 
        top_p_enc <= 0;
        for (j = 0; j < PPE_H_WIDTH_T; j = j + 1) begin 
          P_enc[j] <= 0;
        end
      end else begin 
        for (j = 0; j < PPE_H_WIDTH_T; j = j + 1) begin 
          P_enc[j] <= Gnt_top_reg[j] ? o_value[j] : P_enc[j];
          // P_enc[o_top] <= o_value[o_top];
        end

        top_p_enc <= o_top;
      end
    end


    generate 
      for (i = 0; i < PPE_H_WIDTH_T; i = i + 1) begin 
        assign Gnt_o[(i+1)*PPE_H_WIDTH-1:i*PPE_H_WIDTH] = Gnt_top_reg[i] ? Gnt[i] : 0;
      end
    endgenerate

    generate 
      for (i = 0; i < PPE_H_WIDTH_T; i = i + 1) begin 
          ppe #(
            .PPE_WIDTH(PPE_H_WIDTH),
            .PPE_LOG_W(PPE_H_LOG_W)
          ) ppe_inst (
              .Req(Reqi[i]),
              .P_enc(P_enc[i]),
              .Gnt(Gnt[i]),
              .valid(valid[i])
          );

          encoder #(
            .WIDTH(PPE_H_WIDTH),
            .LOG_W(PPE_H_LOG_W)
          ) encoder_inst (
          .in(Gnt[i] << 1 | Gnt[i] >> (PPE_H_WIDTH-1)),
          .out(o_value[i]) 
          );
      end
    endgenerate
endmodule
