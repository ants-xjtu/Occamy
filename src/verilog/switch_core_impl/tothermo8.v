`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2024/10/15 17:00:37
// Design Name: 
// Module Name: tothermo8
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



module tothermo8(
  input [2:0] x,
  output [7:0] y
);

assign y[0] = ((x[0]|x[1])|x[2]);
assign y[1] = (x[1]|x[2]);
assign y[2] = ((x[0]&x[1])|x[2]);
assign y[3] = x[2];
assign y[4] = ((x[0]|x[1])&x[2]);
assign y[5] = (x[1]&x[2]);
assign y[6] = ((x[0]&x[1])&x[2]);
assign y[7] = 0;

endmodule
