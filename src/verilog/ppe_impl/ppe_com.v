module ppe_com #(
  parameter PPE_C_WIDTH = 8,
  parameter PPE_C_LOG_W = 3 
) (
    input clk,
    input rst,
    input [PPE_C_WIDTH-1:0] Req,
    output reg[PPE_C_WIDTH-1:0] Gnt_reg,
    output reg valid_reg
);
    
    reg [PPE_C_LOG_W-1:0] P_enc_reg;
    reg [PPE_C_WIDTH-1:0] Req_reg;
    wire [PPE_C_LOG_W-1:0] o_value;
    wire [PPE_C_WIDTH-1:0] Gnt;
    wire valid;

    ppe #(
      .PPE_WIDTH(PPE_C_WIDTH),
      .PPE_LOG_W(PPE_C_LOG_W)
    )u_ppe_w1024 (
        .Req                                (Req_reg),
        .P_enc                              (P_enc_reg                 ),
        .Gnt                                (Gnt),
        .valid                              (valid)
    );

    encoder #(
      .WIDTH(PPE_C_WIDTH),
      .LOG_W(PPE_C_LOG_W)
    )u_encoder (
    .in                                 (Gnt << 1 | Gnt >> (PPE_C_WIDTH-1)),
    .out                                (o_value) 
    );

    always@(posedge clk) begin 
      if (rst) begin 
        Req_reg <= 0;
        Gnt_reg <= 0;
        valid_reg <= 0;
        P_enc_reg <= 0;
      end
      else begin 
        Req_reg <= Req;
        Gnt_reg <= Gnt;
        valid_reg <= valid;
        P_enc_reg <= o_value;
      end
    end


endmodule
