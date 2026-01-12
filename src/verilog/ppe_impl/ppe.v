`timescale 1ns / 1ps
module ppe #(
  parameter PPE_WIDTH = 1024,
  parameter PPE_LOG_W = 10
) (
    input              [PPE_WIDTH-1: 0]        Req                        ,// 请求信号，位宽为1024
    input              [PPE_LOG_W-1: 0]        P_enc                      ,// 编码器输入，位宽为10
    output reg         [PPE_WIDTH-1: 0]        Gnt,
    output                              valid                       // 输出valid信号
);

    wire               [PPE_WIDTH-1: 0]        P_thermo;
    wire               [PPE_WIDTH-1: 0]        Gnt_smpl_pe;
    wire               [PPE_WIDTH-1: 0]        Gnt_smpl_pe_thermo;
    wire               [1: 0]        anyGnt_smpl_pe_thermo;
    wire               [1: 0]        anyGnt_smpl_pe;

    thermometer
    #(
      .WIDTH(PPE_WIDTH),
      .LOG_W(PPE_LOG_W)
    ) rmo_mask (
      .enc(P_enc),
      .thermo(P_thermo)
    );

    smpl_pe 
    # (.WIDTH(PPE_WIDTH/2)) 
    u0_smpl_pe_thermo_w512 
    (
    .Req                                ((~P_thermo[PPE_WIDTH/2-1:0]) & Req[PPE_WIDTH/2-1:   0]),
    .Gnt                                (Gnt_smpl_pe_thermo[PPE_WIDTH/2-1:   0]),
    .valid                              (anyGnt_smpl_pe_thermo[0]  ) 
    );

    smpl_pe 
    # (.WIDTH(PPE_WIDTH/2))
    u0_smpl_pe_w512 
    (
    .Req                                (Req[PPE_WIDTH/2-1:0]),
    .Gnt                                (Gnt_smpl_pe[PPE_WIDTH/2-1:0]),
    .valid                              (anyGnt_smpl_pe[0]) 
    );

    smpl_pe 
    # (.WIDTH(PPE_WIDTH/2))
    u1_smpl_pe_thermo_w512
    (
    .Req                                ((~P_thermo[PPE_WIDTH-1:PPE_WIDTH/2]) & Req[PPE_WIDTH-1:PPE_WIDTH/2]),
    .Gnt                                (Gnt_smpl_pe_thermo[PPE_WIDTH-1: PPE_WIDTH/2]),
    .valid                              (anyGnt_smpl_pe_thermo[1]  ) 
    );

    smpl_pe 
    # (.WIDTH(PPE_WIDTH/2))
    u1_smpl_pe_w512
    (
    .Req                                (Req[PPE_WIDTH-1: PPE_WIDTH/2]),
    .Gnt                                (Gnt_smpl_pe[PPE_WIDTH-1: PPE_WIDTH/2]),
    .valid                              (anyGnt_smpl_pe[1]) 
    );

    assign valid = (anyGnt_smpl_pe[0] | anyGnt_smpl_pe[1]);

    always @* begin
        case(anyGnt_smpl_pe_thermo)
            2'b00: begin
                case(anyGnt_smpl_pe)
                    2'b00: begin
                        Gnt = Gnt_smpl_pe;
                    end
                    2'b01: begin
                        Gnt = Gnt_smpl_pe;
                    end
                    2'b10: begin
                        Gnt = Gnt_smpl_pe;
                    end
                    2'b11: begin
                        Gnt[PPE_WIDTH-1:PPE_WIDTH/2] = 0;
                        Gnt[PPE_WIDTH/2-1:0] = Gnt_smpl_pe[PPE_WIDTH/2-1:0];
                    end
                endcase
            end
            2'b01: begin
                Gnt = Gnt_smpl_pe_thermo;
            end
            2'b10: begin
                Gnt = Gnt_smpl_pe_thermo;
            end
            2'b11: begin
                case (P_enc[PPE_LOG_W-1])
                    1'b0: begin
                        Gnt[PPE_WIDTH-1: PPE_WIDTH/2] = 0;
                        Gnt[PPE_WIDTH/2-1:0] = Gnt_smpl_pe_thermo[PPE_WIDTH/2-1:0];
                    end
                    1'b1: begin
                        Gnt[PPE_WIDTH-1: PPE_WIDTH/2] = Gnt_smpl_pe_thermo[PPE_WIDTH-1:PPE_WIDTH/2];
                        Gnt[PPE_WIDTH/2-1:0] = 0;
                    end
                endcase
            end
        endcase
    end
endmodule
