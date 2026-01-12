`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2024/08/05 17:21:49
// Design Name: 
// Module Name: cell_linked_list_ios
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


module cell_pointer_memory_control #(
    parameter RST    = 3'b000,
    parameter IDLE   = 3'b001,
    parameter ENDRST = 3'b110,


    parameter DATA_WIDTH = 512,

    parameter PD_WIDTH = 128,
    parameter PKT_PTR_WIDTH = 16,
    parameter CELL_PTR_WIDTH = 16,

    parameter DATA_SRAM_ADDR_WIDTH = 13,
    parameter CELL_PTR_SRAM_ADDR_WIDTH = 13,
    parameter BUFFER_SIZE = 2048
) (

    input clk,  // Clock input
    input rstn, // Asynchronous reset, active low

    // Admission control - read from free queue
    input        FQ_rd,      // Read signal for the queue
    input        FQ_rd_ack,      
    output       FQ_empty,   // Queue empty status signal
    output [CELL_PTR_WIDTH - 1:0] ptr_dout_s, // Pointer data output (short)

    output reg [CELL_PTR_WIDTH - 1:0] FQ_head,

    // Cell read operations - write into queue
    input             FQ_wr,          // Write signal for the queue
    input      [CELL_PTR_WIDTH - 1:0] FQ_din_head,    // Data input for the queue
    input      [CELL_PTR_WIDTH - 1:0] FQ_din_tail,
    // Cell read operations - read from qc
    input             cell_mem_rd,
    output [CELL_PTR_WIDTH*2 - 1:0] cell_mem_dout,
    input      [CELL_PTR_SRAM_ADDR_WIDTH - 1:0] cell_mem_addr,

    input FQ_wr_hd,
    input [CELL_PTR_WIDTH - 1:0] FQ_din_head_hd,
    input [CELL_PTR_WIDTH - 1:0] FQ_din_tail_hd
);
    reg  [2:0] main_state;
    wire [CELL_PTR_WIDTH*2 - 1:0] ram_out_data_b;

    reg  [CELL_PTR_WIDTH - 1:0] ram_addr_a_rst;
    reg         ram_in_enable_a_rst;
    reg  [CELL_PTR_WIDTH*2 - 1:0] ram_in_data_a_rst;
    wire [CELL_PTR_WIDTH*2 - 1:0] ram_out_data_a;

    reg  [CELL_PTR_WIDTH - 1:0] FQ_tail;
    assign FQ_empty   = (FQ_head == FQ_tail) ? 1 : 0;

    assign ptr_dout_s = FQ_rd ? FQ_head : ram_out_data_b;

    reg [2:0] fq_rd_state;
    always @(posedge clk) begin 
        if (!rstn) begin 
            FQ_head <= 0;
            fq_rd_state <= 0;
        end else begin 
            if (FQ_rd_ack) begin 
                FQ_head <= ram_out_data_b[CELL_PTR_SRAM_ADDR_WIDTH - 1:0];
            end
        end
    end

    always@(posedge clk) begin 
        if (!rstn) begin 
            FQ_tail <= BUFFER_SIZE-1; 
        end else begin 
            if (FQ_wr) begin 
                FQ_tail         <=  FQ_din_tail[CELL_PTR_WIDTH - 1:0];
            end else if (FQ_wr_hd) begin 
                FQ_tail         <=  FQ_din_tail_hd[CELL_PTR_WIDTH - 1:0];
            end
        end
    end

    assign cell_mem_dout = ram_out_data_a;

    reg  [CELL_PTR_SRAM_ADDR_WIDTH - 1:0] rst_index;
    always @(posedge clk) begin
        if (!rstn) begin
            main_state <=  RST;
            rst_index  <=  0;
        end else begin
            case (main_state)
                // Reset state:
                //  In the reset state, the pointer list stored in ptr_list_memory 
                //  is set in the order of 0->1->2->...->n.
                //  Pointer: 16bits-Next Ptr + 16bits-Current Ptr
                //
                //      head                            tail
                //      0       1       2       3       4
                //      {1,0} {2,1}    {3,2}    {4,3}   {Y,4}  
                //          
                //      {next pointer, pointer to cell data memory}
                RST: begin
                    if (&rst_index) begin
                        rst_index           <=  0;
                        ram_in_data_a_rst   <= 0;
                        ram_addr_a_rst      <=  rst_index;
                        ram_in_enable_a_rst <=  1;

                        main_state          <=  ENDRST;
                    end else begin
                        rst_index           <=  rst_index + 1;
                        ram_in_data_a_rst   <= {rst_index[CELL_PTR_SRAM_ADDR_WIDTH - 1:0] + 1};
                        ram_addr_a_rst      <=  rst_index;
                        ram_in_enable_a_rst <=  1;

                        main_state          <=  RST;
                    end
                end

                ENDRST: begin
                    ram_in_enable_a_rst <=  0;
                    main_state          <=  IDLE;
                end

                default: begin
                    main_state <=  IDLE;
                end
            endcase
        end
    end

    wire [CELL_PTR_SRAM_ADDR_WIDTH - 1:0] ram_addr_a_s;
    wire [CELL_PTR_SRAM_ADDR_WIDTH - 1:0] ram_addr_b_s;
    wire [CELL_PTR_WIDTH*2 - 1:0] ram_in_data_a_s;
    wire        ram_in_enable_a_s;

    assign ram_addr_a_s = (ram_in_enable_a_rst) ? ram_addr_a_rst : 
                          FQ_wr ? FQ_tail:
                          cell_mem_rd ? cell_mem_addr:
                          FQ_wr_hd ? FQ_tail: 
                          ram_out_data_a[CELL_PTR_SRAM_ADDR_WIDTH - 1:0];

    assign ram_in_data_a_s = (ram_in_enable_a_rst) ? ram_in_data_a_rst :
                            FQ_wr ? FQ_din_head :
                            FQ_wr_hd ? FQ_din_head_hd:
                            FQ_din_head;

    assign ram_addr_b_s = FQ_rd ? FQ_head : ram_out_data_b[CELL_PTR_SRAM_ADDR_WIDTH - 1:0]; 
    assign ram_in_enable_a_s = ram_in_enable_a_rst | FQ_wr | FQ_wr_hd;

    

    dpsram_w32_d8k ptr_list_memory (
        .clka(clk),
        .clkb(clk),

        .addra(ram_addr_a_s),
        .addrb(ram_addr_b_s),

        .wea (ram_in_enable_a_s),
        .dina(ram_in_data_a_s),

        .web (),
        .dinb(),

        .ena  (1'b1),
        .douta(ram_out_data_a),

        .enb  (1'b1),
        .doutb(ram_out_data_b)
    );

endmodule
