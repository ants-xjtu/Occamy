`timescale 1ns / 1ps

module pd_memory_control_FIFO #(
  parameter PD_WIDTH = 128,
  parameter PORT_NUM = 8 
)(
    input clk,  // Clock input
    input rstn, // Asynchronous reset, active low

    // [Admission]  - write into qc
    input      [PORT_NUM - 1:0] pd_qc_wr_ptr_wr_en,
    input      [PD_WIDTH - 1:0] pd_qc_wr_ptr_din,
    output reg pd_qc_ptr_full,

    // [Cell read]  - read from qc
    output [PORT_NUM - 1:0] pd_ptr_rdy,
    input  [PORT_NUM - 1:0] pd_ptr_ack,
    output [PD_WIDTH*PORT_NUM - 1:0] pd_ptr_dout,

    // [Headdrop] - read from qc
    input [PORT_NUM - 1:0] pd_ptr_ack_hd
);

    wire [PD_WIDTH - 1:0] pd_ptr_dout_s[PORT_NUM - 1:0];

    wire [PORT_NUM - 1:0] pd_rd_en;
    assign pd_rd_en = pd_ptr_ack | pd_ptr_ack_hd;

    wire [PORT_NUM - 1:0] pd_empty;
    assign pd_ptr_rdy = ~pd_empty;

    genvar i;
    generate 
        for (i = 0; i < PORT_NUM; i = i + 1) begin: loop 
            assign pd_ptr_dout[PD_WIDTH*(i+1)-1:PD_WIDTH*i] = pd_ptr_dout_s[i];
        end
    endgenerate 

    wire [PORT_NUM - 1:0] pd_qc_ptr_full_s;
    always @(posedge clk) begin 
        pd_qc_ptr_full <= &pd_qc_ptr_full_s;
    end


    genvar j;
    generate
        for (j = 0; j < PORT_NUM; j = j + 1) begin : pd_list_gen
            sfifo_ft_w128_d2k pd_list_inst (
                .clk       (clk),
                .rst       (!rstn),
                .din       (pd_qc_wr_ptr_din),
                .wr_en     (pd_qc_wr_ptr_wr_en[j]),
                .rd_en     (pd_rd_en[j]),
                .dout      (pd_ptr_dout_s[j]),
                .full      (pd_qc_ptr_full_s[j]),
                .empty     (pd_empty[j]),
                .data_count()
            );
        end
    endgenerate

endmodule

