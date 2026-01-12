`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2024/11/19 10:27:12
// Design Name: 
// Module Name: switch_top
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

// switch_core_v2 switch_core(
//     .clk(clk),
//     .rstn(rstn),
// 
//     .data_in(data_in),
//     .data_wr(data_wr),
//     .i_cell_ptr_fifo_din(i_cell_ptr_fifo_din),
//     .i_cell_ptr_fifo_wr(i_cell_ptr_fifo_wr),
//     .qlen(qlen_all),
// 
//     .data_valid(data_valid),
//     .data_dout(data_dout)
// );


module switch_top(
    input clk,
    input rstn,
    output reg [511:0] data_in,
    output reg data_wr,
    output reg [15:0] i_cell_ptr_fifo_din,
    output reg i_cell_ptr_fifo_wr
);

reg [9:0] cnt1; 
reg [9:0] cnt2;
reg [5:0] sleep_t;

reg [3:0] state;
reg [3:0] state2;
reg [1:0] num;



always@(posedge clk) begin 
  if (!rstn) begin 
    cnt1 <= 0;
    cnt2 <= 0;
  end
  else begin 
    cnt1 <= cnt1 + 1;
    if (cnt1 == 1023) cnt2 <= cnt2 + 1;
  end
end


always@(posedge clk) begin 
  if (!rstn) begin 
    state2 <= 0;
    state <= 0;
    data_wr <= 0;
    i_cell_ptr_fifo_wr<= 0;
    num <= 0;
  end
  else begin 
  if (cnt2 < 70) begin 
    case(state)
      0: begin 
        data_wr<= 1; 
        i_cell_ptr_fifo_din <= 16'b0000_0001_00_000100;
        i_cell_ptr_fifo_wr <= 1;
        state <= 1; 
      end
      1: begin 
        i_cell_ptr_fifo_wr <= 0; 
        state <= 2; 
      end
      2: begin 
        state <= 3; 
      end
      3: begin 
        state <= 4; 
      end
      4: begin 
        data_wr <= 0;
        sleep_t <= 11;
        state <= 5;
      end
      5: begin 
        sleep_t <= sleep_t - 1;  
        if (sleep_t == 1) begin 
          state <= 0;
        end
      end
    endcase 
  end
  else if (cnt2 < 120) begin 
    case (state2) 
      0: begin 
        data_wr <= 1; 
        i_cell_ptr_fifo_din <= 16'b0000_0001_00_000100;
        i_cell_ptr_fifo_wr <= 1; 
        num <= 3;
        state2 <= 1; 
      end
      1: begin 
        i_cell_ptr_fifo_wr <= 0;
        state2 <= 2; 
      end
      2: begin 
        state2 <= 3; 
      end
      3: begin 
        state2 <= 4; 
      end
      4: begin 
        if (num == 0) begin 
          state2 <= 0;
        end
        else begin 
          i_cell_ptr_fifo_din <= 16'b0000_0000_00_000100;
          i_cell_ptr_fifo_wr <= 1; 
          num <= num - 1; 
          state2 <= 5; 
        end
      end
      5: begin 
        i_cell_ptr_fifo_wr <= 0;
        state2 <= 6; 
      end
      6: begin 
        state2 <= 7;
      end
      7: begin 
        state2 <= 4;  
      end
    endcase
  end
  else begin 
    data_wr <= 0;
    i_cell_ptr_fifo_wr <= 0;
  end
end
end
endmodule
