`timescale 1ns / 1ps

module headdrop_drop #(
  parameter PORT_NUM = 4,
  parameter LOG_PORT_NUM = 2,
  parameter PD_WIDTH = 128,
  parameter CELL_PTR_WIDTH = 16
)(
    input clk,
    input rstn,

    // conbined to Cell Ptr memory 
    output reg FQ_wr,
    output [CELL_PTR_WIDTH-1:0] FQ_din_head,
    output [CELL_PTR_WIDTH-1:0] FQ_din_tail,

    // conbined to PD memory 
    output reg [PORT_NUM-1:0] pd_ptr_ack,
    input [PORT_NUM*PD_WIDTH-1:0] pd_ptr_dout,

    // conbined to RR scheduler 
    input [LOG_PORT_NUM-1:0] RR_port,
    input valid,

    // conbined to statistics 
    output reg headdrop_out,
    output [10:0] headdrop_pkt_len_out
);

    reg  [3:0] state;
    reg  [1:0] RR;

    wire [127:0] pd_head[3:0];
    wire [3:0] headdrop_rdy;

    genvar i;
    generate 
        for (i = 0; i < PORT_NUM; i = i + 1) begin: loop
            assign pd_head[i] = pd_ptr_dout[PD_WIDTH*(i+1) - 1:PD_WIDTH*i];
        end
    endgenerate 


    assign FQ_din_head = pd_head[RR_port][80:65];
    assign FQ_din_tail = pd_head[RR_port][64:49];
    assign headdrop_pkt_len_out = pd_head[RR_port][48:38];

    always @(posedge clk or negedge rstn) begin
        if (!rstn) begin
            FQ_wr                <= #2 0;
            pd_ptr_ack           <= #2 0;

            headdrop_out <= #2 0;

            state                <= #2 0;
        end else begin
            case (state)
                0: begin
                    if (valid) begin
                        pd_ptr_ack[RR_port] <= #2 1;
                        headdrop_out <= #2 1;
                        FQ_wr        <= #2 1;
                        state <= #2 1;
                    end
                end
                1: begin
                    headdrop_out <= #2 0;
                    pd_ptr_ack <= #2 0;
                    FQ_wr <= #2 0;
                    state<= #2 0;
                end
            endcase
        end
    end

endmodule 
