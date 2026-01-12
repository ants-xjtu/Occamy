`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2024/08/27 10:19:46
// Design Name: 
// Module Name: switch_core_v2
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module switch_core_v2 #(
  parameter TOP_DATA_WIDTH = 512,
  parameter TOP_ENABLE_HEADDROP = 0,

  parameter TOP_PD_WIDTH = 128,
  parameter TOP_PKT_PTR_WIDTH = 16,
  parameter TOP_CELL_PTR_WIDTH = 16,

  parameter TOP_DATA_SRAM_ADDR_WIDTH = 13,
  parameter TOP_CELL_PTR_SRAM_ADDR_WIDTH = 13,
  parameter TOP_PORT_NUM = 8,
  parameter TOP_QLEN_WIDTH = 11,
  parameter TOP_BUFFER_SIZE = 2048,

  parameter TOP_USE_DRR = 1'b1
) (
    input clk,
    input rstn,
    input [TOP_DATA_WIDTH - 1:0] data_in,
    input data_wr,
    input [TOP_CELL_PTR_WIDTH - 1:0] i_cell_ptr_fifo_din,
    input i_cell_ptr_fifo_wr,

    output data_valid,
    output [TOP_DATA_WIDTH - 1:0] data_dout, 
    
    output [TOP_QLEN_WIDTH*TOP_PORT_NUM - 1:0] qlen,

    output out_signal,
    output headdrop_out,
    output [3:0] out_port,
    output [3:0] headdrop_out_port
);

    // output 
    // wire [TOP_DATA_WIDTH - 1:0] data_dout;
    // wire [TOP_DATA_WIDTH - 1:0] data_in;

    // admission TO data_sram
    wire [TOP_DATA_SRAM_ADDR_WIDTH - 1:0] sram_addr_a;
    wire [TOP_DATA_WIDTH - 1:0] sram_din_a;
    wire         sram_wr_a;

    // admission TO cell_ptr_mem
    wire         FQ_rd;
    wire         FQ_rd_ack;
    wire         FQ_empty;
    wire [TOP_CELL_PTR_WIDTH - 1:0] ptr_dout_s;
    wire [TOP_CELL_PTR_WIDTH - 1:0] FQ_head;

    // admission TO pd_mem
    wire         pd_qc_ptr_full;
    wire [TOP_PORT_NUM - 1:0] pd_qc_wr_ptr_wr_en;
    wire [TOP_PD_WIDTH - 1:0] pd_qc_wr_ptr_din;

    // admission TO statistics
    wire         in;
    wire [3:0] in_port;
    wire [ 10:0] pkt_len_in;
    wire [TOP_PORT_NUM - 1:0] bitmap_dt;

    // cell_ptr_mem TO cell_read
    wire         FQ_wr;
    wire [TOP_CELL_PTR_WIDTH - 1:0] FQ_din_head, FQ_din_tail;
    wire         cell_mem_rd;
    wire [TOP_CELL_PTR_SRAM_ADDR_WIDTH - 1:0] cell_mem_addr;
    wire [TOP_CELL_PTR_WIDTH*2 - 1:0] cell_mem_dout;

    // pd_mem TO cell_read
    wire [TOP_PORT_NUM - 1:0] pd_ptr_ack;
    wire [TOP_PORT_NUM - 1:0] pd_ptr_rdy;
    wire [TOP_PD_WIDTH * TOP_PORT_NUM - 1:0] pd_ptr_dout;

    // data_sram TO cell_read
    wire [TOP_DATA_SRAM_ADDR_WIDTH - 1:0] sram_addr_b;
    wire [TOP_DATA_WIDTH - 1:0] sram_dout_b;

    // cell_read TO statistics
    // wire         out;
    // wire [  3:0] out_port;
    wire [ 10:0] pkt_len_out;

    // headdrop TO cell_ptr_mem
    wire         FQ_wr_hd;
    wire [TOP_CELL_PTR_WIDTH - 1:0] FQ_din_head_hd;
    wire [TOP_CELL_PTR_WIDTH - 1:0] FQ_din_tail_hd;

    // cell_read TO headdrop
    wire         cell_rd_cell_buzy;

    // headdrop TO statistics
    // wire         headdrop_out;
    // wire [  3:0] headdrop_out_port;
    wire [10:0] headdrop_pkt_len_out;
    wire [TOP_PORT_NUM - 1:0] bitmap;

    // headdrop TO pd_mem
    wire [TOP_PORT_NUM - 1:0] pd_ptr_ack_hd;
    assign data_dout = sram_dout_b;

    admission #(
      .DATA_WIDTH(TOP_DATA_WIDTH),
      .PORT_NUM(TOP_PORT_NUM) 
    ) ad (
        .clk (clk),
        .rstn(rstn),

        .data_in            (data_in),
        .data_wr            (data_wr),
        .i_cell_ptr_fifo_din(i_cell_ptr_fifo_din),
        .i_cell_ptr_fifo_wr (i_cell_ptr_fifo_wr),

        .sram_addr(sram_addr_a),
        .sram_din (sram_din_a),
        .sram_wr  (sram_wr_a),

        .FQ_rd     (FQ_rd),
        .FQ_rd_ack (FQ_rd_ack),
        .FQ_empty  (FQ_empty),
        .ptr_dout_s(ptr_dout_s),
        .FQ_head(FQ_head),

        .pd_qc_wr_ptr_wr_en(pd_qc_wr_ptr_wr_en),
        .pd_qc_wr_ptr_din  (pd_qc_wr_ptr_din),
        .pd_qc_ptr_full    (pd_qc_ptr_full),

        .in        (in),
        .in_port   (in_port),
        .pkt_len_in(pkt_len_in),
        .bitmap    (bitmap_dt)
    );

    cell_pointer_memory_control #(
      .DATA_WIDTH(TOP_PORT_NUM),
      .PD_WIDTH(TOP_PD_WIDTH),
      .PKT_PTR_WIDTH(TOP_PKT_PTR_WIDTH),
      .CELL_PTR_WIDTH(TOP_CELL_PTR_WIDTH),
      .DATA_SRAM_ADDR_WIDTH(TOP_DATA_SRAM_ADDR_WIDTH),
      .CELL_PTR_SRAM_ADDR_WIDTH(TOP_CELL_PTR_SRAM_ADDR_WIDTH),
      .BUFFER_SIZE(TOP_BUFFER_SIZE)
    ) cpm (
        .clk (clk),
        .rstn(rstn),

        .FQ_rd     (FQ_rd),
        .FQ_empty  (FQ_empty),
        .ptr_dout_s(ptr_dout_s),
        .FQ_head(FQ_head),
        .FQ_rd_ack (FQ_rd_ack),

        .FQ_wr        (FQ_wr),
        .FQ_din_head  (FQ_din_head),
        .FQ_din_tail  (FQ_din_tail),
        .cell_mem_rd  (cell_mem_rd),
        .cell_mem_dout(cell_mem_dout),
        .cell_mem_addr(cell_mem_addr),

        .FQ_wr_hd      (FQ_wr_hd),
        .FQ_din_head_hd(FQ_din_head_hd),
        .FQ_din_tail_hd(FQ_din_tail_hd)
    );

    pd_memory_control_FIFO #(
      .PORT_NUM(TOP_PORT_NUM)
    ) pdm_o (
        .clk (clk),
        .rstn(rstn),

        .pd_qc_wr_ptr_wr_en(pd_qc_wr_ptr_wr_en),
        .pd_qc_wr_ptr_din  (pd_qc_wr_ptr_din),
        .pd_qc_ptr_full    (pd_qc_ptr_full),

        .pd_ptr_rdy (pd_ptr_rdy),
        .pd_ptr_ack (pd_ptr_ack),
        .pd_ptr_dout(pd_ptr_dout),

        .pd_ptr_ack_hd(pd_ptr_ack_hd)
    );


    cell_read_v2 #(
      .DATA_WIDTH(TOP_DATA_WIDTH),
      .PORT_NUM(TOP_PORT_NUM),
      .USE_DRR(TOP_USE_DRR)
    ) cr (
        .clk        (clk),
        .rstn       (rstn),
        .FQ_wr      (FQ_wr),
        .FQ_din_head(FQ_din_head),
        .FQ_din_tail(FQ_din_tail),

        .cell_mem_rd  (cell_mem_rd),
        .cell_mem_dout(cell_mem_dout),
        .cell_mem_addr(cell_mem_addr),


        .pd_ptr_rdy (pd_ptr_rdy),
        .pd_ptr_ack (pd_ptr_ack),
        .pd_ptr_dout(pd_ptr_dout),

        .data_sram_addr_b(sram_addr_b),
        .data_valid      (data_valid),

        .out        (out_signal),
        .out_port   (out_port),
        .pkt_len_out(pkt_len_out),

        .cell_rd_cell_buzy(cell_rd_cell_buzy)
    );


    headdrop_v5 #(
      .ENABLE_HEADDROP(TOP_ENABLE_HEADDROP),
      .PORT_NUM(TOP_PORT_NUM)
    ) hd (
        .clk        (clk),
        .rstn       (rstn),
        .FQ_wr_out      (FQ_wr_hd),
        .FQ_din_head(FQ_din_head_hd),
        .FQ_din_tail(FQ_din_tail_hd),
        .pd_ptr_rdy (pd_ptr_rdy),
        .pd_ptr_ack (pd_ptr_ack_hd),
        .pd_ptr_dout(pd_ptr_dout),

        .cell_rd_pd_buzy  (pd_ptr_ack),
        .cell_rd_cell_buzy(cell_rd_cell_buzy),
        .cell_rd_sts_buzy(out_signal),

        .headdrop_out        (headdrop_out),
        .headdrop_out_port   (headdrop_out_port),
        .headdrop_pkt_len_out(headdrop_pkt_len_out),

        .bitmap(bitmap)
    );


    dpsram_w512_d8k u_data_ram (
        .clka (clk),
        .wea  (sram_wr_a),
        .addra(sram_addr_a[TOP_DATA_SRAM_ADDR_WIDTH - 1:0]),
        .dina (sram_din_a),
        .douta(),
        .clkb (clk),
        .web  (1'b0),
        .addrb(sram_addr_b[TOP_DATA_SRAM_ADDR_WIDTH - 1:0]),
        .dinb (0),
        .ena  (1'b1),
        .enb  (1'b1),
        .doutb(sram_dout_b)
    );

    statistics_v2 #(
      .PORT_NUM(TOP_PORT_NUM),
      .QLEN_WIDTH(TOP_QLEN_WIDTH),
      .USE_DRR(TOP_USE_DRR)
    ) sts (
        .clk        (clk),
        .rstn       (rstn),
        .in         (in),
        .out        (out_signal | headdrop_out),
        .in_port    (in_port),
        .out_port   ((!out_signal && headdrop_out) ? headdrop_out_port : out_port),

        .bitmap              (bitmap),
        .bitmap_dt           (bitmap_dt), 

        .qlen_all(qlen)
    );


endmodule
