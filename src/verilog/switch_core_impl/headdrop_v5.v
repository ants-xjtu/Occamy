`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2024/11/11 14:31:00
// Design Name: 
// Module Name: headdrop_v5
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// The latest version of headdrop
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module headdrop_v5 #(
    parameter ENABLE_HEADDROP = 1,

    parameter PD_WIDTH = 128,
    parameter PKT_PTR_WIDTH = 16,
    parameter CELL_PTR_WIDTH = 16,

    parameter DATA_SRAM_ADDR_WIDTH = 13,
    parameter PORT_NUM = 8 
) (
    input clk,
    input rstn,

    output FQ_wr_out,
    output reg [CELL_PTR_WIDTH - 1:0] FQ_din_head,
    output reg [CELL_PTR_WIDTH - 1:0] FQ_din_tail,

    input [PORT_NUM - 1:0] pd_ptr_rdy,
    output reg [PORT_NUM - 1:0] pd_ptr_ack,
    input [PD_WIDTH*PORT_NUM - 1:0] pd_ptr_dout,

    input [PORT_NUM - 1:0] cell_rd_pd_buzy,
    input cell_rd_cell_buzy,
    input cell_rd_sts_buzy,

    output reg headdrop_out,
    output reg [3:0] headdrop_out_port,
    output reg [10:0] headdrop_pkt_len_out,

    input [PORT_NUM - 1:0] bitmap
    );

    reg FQ_wr;
    reg [3:0] state; 
    reg [2:0] shift_amt;
    wire [2:0] ppe_out;
    wire [PORT_NUM - 1:0] headdrop_rdy;
    assign headdrop_rdy = (~bitmap & pd_ptr_rdy);
    wire rdy;
    // you can use this to control the headdrop or NOheaddrop
    assign rdy = ENABLE_HEADDROP ? (| headdrop_rdy) : 0;

    wire [PD_WIDTH - 1:0] pd_head[PORT_NUM - 1:0];

    genvar i;
    generate 
        for (i = 0; i < PORT_NUM; i = i + 1) begin: loop
            assign pd_head[i] = pd_ptr_dout[PD_WIDTH*(i+1) - 1:PD_WIDTH*i];
        end
    endgenerate 

    assign FQ_wr_out = FQ_wr & ~cell_rd_cell_buzy;

    // always @(posedge clk) begin
    //     if (!rstn) begin
    //         FQ_wr                <=  0;
    //         FQ_din_head          <=  0;
    //         FQ_din_tail          <=  0;
    //         pd_ptr_ack           <=  0;
    //         headdrop_out         <=  0;
    //         headdrop_out_port    <=  0;
    //         headdrop_pkt_len_out <=  0;

    //         state                <=  0;
    //         shift_amt <=  0;
    //     end else begin
    //     case(state)
    //         0: begin 
    //             if (!cell_rd_cell_buzy && !cell_rd_pd_buzy)
    //             if (rdy) begin
    //                 FQ_din_head          <=  pd_head[ppe_out][81:66];
    //                 FQ_din_tail          <=  pd_head[ppe_out][65:50];
    //                 pd_ptr_ack[ppe_out]        <=  1;
    //                 headdrop_out_port    <=  ppe_out;
    //                 headdrop_pkt_len_out <=  pd_head[ppe_out][49:38];
    //                 shift_amt <= ppe_out + 1;

    //                 state <= 1;
    //             end
    //         end
    //         1: begin 
    //             pd_ptr_ack <=  0; 
    //             if (cell_rd_pd_buzy == pd_ptr_ack) begin 
    //                 state <=  0; 
    //             end else begin 
    //                 state <= 2; 
    //             end
    //         end
    //         2: begin 
    //             if (~cell_rd_cell_buzy) begin  
    //                 FQ_wr <= 1; 
    //                 headdrop_out <=  1; 
    //                 state <= 3;  
    //             end
    //         end
    //         3: begin 
    //             FQ_wr <= 0; 
    //             headdrop_out <= 0; 
    //             if (cell_rd_cell_buzy && cell_rd_sts_buzy) begin 
    //                 state <= 2; 
    //             end else if (cell_rd_cell_buzy) begin 
    //                 state <= 4; 
    //             end else if (cell_rd_sts_buzy) begin 
    //                 state <= 6;
    //             end else begin 
    //                 state <= 0; 
    //             end
    //         end
    //         4: begin 
    //             FQ_wr <= 1; 
    //             state <= 5; 
    //         end
    //         5: begin 
    //             FQ_wr <= 0; 
    //             if(cell_rd_cell_buzy) begin 
    //                 state <= 4; 
    //             end else begin 
    //                 state <= 0;
    //             end
    //         end
    //         6: begin 
    //             headdrop_out <= 1;
    //             state <= 7;
    //         end
    //         7: begin 
    //             headdrop_out <= 0;
    //             if (cell_rd_sts_buzy) begin 
    //                 state <= 6;
    //             end else begin 
    //                 state <= 0;
    //             end
    //         end

    //         endcase
    //     end
    // end



    reg [3:0] headdrop_out_port_reg;
    reg [CELL_PTR_WIDTH - 1:0] FQ_din_head_reg;
    reg [CELL_PTR_WIDTH - 1:0] FQ_din_tail_reg;

    reg [10:0] headdrop_pkt_len_out_reg;
    reg valid_next;
always@(posedge clk) begin
    if (!rstn) begin

        valid_next <= 0;
        FQ_din_head_reg <= 0;
        FQ_din_tail_reg <= 0;
        headdrop_out_port_reg <= 0;
        headdrop_pkt_len_out_reg <= 0;

        FQ_wr                <=  0;
        FQ_din_head          <=  0;
        FQ_din_tail          <=  0;
        pd_ptr_ack           <=  0;
        headdrop_out         <=  0;
        headdrop_out_port    <=  0;
        headdrop_pkt_len_out <=  0;

        shift_amt <=  0;
   
    end else begin 

        // take a pkt
        if (!cell_rd_cell_buzy && !cell_rd_pd_buzy && rdy) begin
            FQ_din_head_reg          <=  pd_head[ppe_out][81:66];
            FQ_din_tail_reg          <=  pd_head[ppe_out][65:50];
            pd_ptr_ack[ppe_out]        <=  1;
            headdrop_out_port_reg <=  ppe_out;
            headdrop_pkt_len_out_reg <=  pd_head[ppe_out][49:38];
            shift_amt <= ppe_out + 1;
            valid_next <= 1;
        end else begin
            pd_ptr_ack <= 0;
            valid_next <= 0;
        end
    
        // free it && out it 
        if (!cell_rd_cell_buzy && valid_next) begin 
            headdrop_out <= 1;
            headdrop_out_port <= headdrop_out_port_reg; 
            FQ_wr <= 1;
            FQ_din_head <= FQ_din_head_reg;
            FQ_din_tail <= FQ_din_tail_reg;
            headdrop_pkt_len_out <= headdrop_pkt_len_out_reg;
        end else begin
            headdrop_out <= 0;
            FQ_wr <= 0;
        end
    end
end

wire [2:0] ppe_out_1;
ppe_8_func ppe(
  .data_in(headdrop_rdy[PORT_NUM - 1:0]),
  .shift_amt(shift_amt[2:0]),
  .b(ppe_out_1[2:0]),
  .valid(ppe_valid)
);
assign ppe_out = ppe_out_1[2:0];

endmodule
