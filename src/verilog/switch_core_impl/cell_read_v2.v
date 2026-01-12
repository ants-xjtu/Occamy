`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2024/08/20 09:46:06
// Design Name: 
// Module Name: cell_read_v2
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

module cell_read_v2 #(
  parameter DATA_WIDTH = 512,

  parameter PD_WIDTH = 128,
  parameter PKT_PTR_WIDTH = 16,
  parameter CELL_PTR_WIDTH = 16,

  parameter DATA_SRAM_ADDR_WIDTH = 13,
  parameter CELL_PTR_SRAM_ADDR_WIDTH = 13,
  parameter PORT_NUM = 8,
  parameter USE_DRR= 1'b1
) (
    input clk,
    input rstn,

    // for cell memory 
    output reg FQ_wr,
    output reg [CELL_PTR_WIDTH - 1:0] FQ_din_head,
    output reg [CELL_PTR_WIDTH - 1:0] FQ_din_tail,
    output reg cell_mem_rd,
    input [CELL_PTR_WIDTH*2-1:0] cell_mem_dout,
    output reg [CELL_PTR_SRAM_ADDR_WIDTH - 1:0] cell_mem_addr,

    // for PD memory 
    input [PORT_NUM - 1:0] pd_ptr_rdy,
    output reg [PORT_NUM - 1:0] pd_ptr_ack,
    input [PD_WIDTH*PORT_NUM - 1:0] pd_ptr_dout,

    // for data
    output [DATA_SRAM_ADDR_WIDTH-1:0] data_sram_addr_b,
    output reg data_valid,

    output reg out,
    output reg [3:0] out_port,
    output reg [10:0] pkt_len_out,

    output reg cell_rd_cell_buzy
);


    wire [PD_WIDTH - 1:0] pd_head[PORT_NUM - 1:0];

    genvar k;
    generate 
        for (k = 0; k < PORT_NUM; k = k + 1) begin: loop
            assign pd_head[k] = pd_ptr_dout[PD_WIDTH*(k+1) - 1:PD_WIDTH*k];
        end
    endgenerate 


    reg [3:0] state;

    reg [2:0] shift_amt;
    wire [2:0] ppe_out;
    wire ppe_valid;

    reg [5:0] cell_num;
    
    reg select;

    assign data_sram_addr_b = select ? cell_mem_addr : {cell_mem_dout[DATA_SRAM_ADDR_WIDTH - 1:0]};

    reg [10:0] counter     [PORT_NUM - 1:0];
    reg [PORT_NUM - 1:0] counter_rst;
    wire [PORT_NUM - 1:0] next_rdy;

    integer i;
    always @(posedge clk) begin
        if (!rstn) begin
            for (i = 0; i < PORT_NUM; i = i + 1) begin 
                counter[i] <=  0;
            end
        end else begin
            for (i = 0; i < PORT_NUM; i = i + 1) begin 
                if (counter_rst[i]) counter[i] <= PORT_NUM * 4 - 2;
                else if (counter[i] > 0) counter[i] <= counter[i] - 1;
            end
        end
    end

    // wire all_zero_temp;
    // wire all_zero;
    // assign all_zero_temp = &counter[0]; // Start by checking the first counter (reduces its bits)
    // genvar j;
    // generate
    //     for (j = 1; j < PORT_NUM; j = j + 1) begin : loop1
    //         assign all_zero_temp = all_zero_temp & &counter[j];
    //     end
    // endgenerate
    // assign all_zero = all_zero_temp;
  
generate if (~USE_DRR) begin
    genvar j;
    // generate 
        for (j = 0; j < PORT_NUM; j = j + 1) begin: loop2
            assign next_rdy[j] = counter[0] == 0 && counter[1] == 0;
        end
    // endgenerate 
  end else begin 
    genvar j;
    // generate 
        for (j = 0; j < PORT_NUM; j = j + 1) begin: loop1
            assign next_rdy[j] = (counter[j] == 0);
        end
    // endgenerate 
  end
endgenerate


    wire [PORT_NUM - 1:0] all_rdy;
    assign all_rdy = next_rdy & pd_ptr_rdy;

    wire rdy;
    assign rdy = |all_rdy;

    always @(posedge clk) begin
        if (!rstn) begin
            state             <=  0;
            shift_amt <= 0; 
            cell_num          <=  0;
            FQ_din_head       <=  0;
            FQ_din_tail       <=  0;
            pd_ptr_ack        <=  0;
            cell_mem_addr     <=  0;
            cell_mem_rd       <=  0;
            data_valid        <=  0;

            out               <=  0;
            out_port          <=  0;
            pkt_len_out       <=  0;

            cell_rd_cell_buzy <=  0;

            select <= 0;

            counter_rst       <=  0;
        end else begin
            case (state)
                0: begin
                    out               <=  0;
                    FQ_wr             <=  0;
                    data_valid        <=  0;
                    if (all_rdy[ppe_out]) begin 
                        // pd fifo output signal 
                        pd_ptr_ack[ppe_out]      <=  1;
                        // for rate limiter
                        counter_rst[ppe_out]     <=  1;
                        // for ppe
                        shift_amt                <=  ppe_out + 1;
                        // Input pkt's head cell ptr
                        cell_mem_rd             <=  1;
                        // aviod conflict with headdrop
                        cell_rd_cell_buzy       <=  1;

                        update_ppe_out_regs();
                        select <= 1;
                        state           <=  1;
                    end else begin 
                        cell_rd_cell_buzy <= 0; 
                    end
                end
                1: begin 
                    select <= 0;
                    data_valid <= 1;
                    pd_ptr_ack <= 0;
                    counter_rst <= 0;
                    cell_mem_rd <= 0;
                    state <= 2;
                end
                2: begin 
                    cell_num <= cell_num - 1;
                    if (cell_num == 2) begin 
                        FQ_wr <= 1; 
                        out <= 1;
                    end else if (cell_num == 1) begin 
                        data_valid <= 0;
                        FQ_wr <= 0;
                        out <= 0;
                        if (all_rdy[ppe_out]) begin 
                            pd_ptr_ack[ppe_out] <= 1;
                            counter_rst[ppe_out] <= 1;
                            shift_amt <= ppe_out + 1;
                            cell_mem_rd <= 1;
                            update_ppe_out_regs();

                            select <= 1;

                            state <= 1;
                        end else begin 
                            state <= 0; 
                            cell_rd_cell_buzy <= 0;
                        end
                    end
                end
            endcase
        end
    end


task update_ppe_out_regs();
    begin
        cell_mem_addr            <=  pd_head[ppe_out][81:66];
        FQ_din_head              <=  pd_head[ppe_out][81:66];
        FQ_din_tail              <=  pd_head[ppe_out][65:50];
        pkt_len_out              <=  pd_head[ppe_out][49:38];
        cell_num                 <=  pd_head[ppe_out][37:32];
        out_port                 <=  ppe_out;
    end
endtask

wire [2:0]ppe_out_1;
wire ppe_valid_1;
generate if (~USE_DRR) begin
  ppe_8_func ppe(
    .data_in(all_rdy[PORT_NUM - 1:0]),
    .shift_amt(shift_amt[2:0]),
    .b(ppe_out_1[2:0]),
    .valid(ppe_valid_1)
  );
  assign ppe_valid = pd_ptr_rdy[0] & next_rdy[0] | ((~pd_ptr_rdy[0]) & ppe_valid_1);
  assign ppe_out = pd_ptr_rdy[0] ? 0 : ppe_out_1[2:0];
end else begin 
  ppe_8_func ppe(
    .data_in(all_rdy[PORT_NUM - 1:0]),
    .shift_amt(shift_amt[2:0]),
    .b(ppe_out_1[2:0]),
    .valid(ppe_valid)
  );
  assign ppe_out = ppe_out_1[2:0];
end
endgenerate

endmodule







