`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2024/11/21 10:14:23
// Design Name: 
// Module Name: ppe_8_func
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// The PPE currently used by the Occamy switch is small(8ports).
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module ppe_8_func(
  input [7:0] data_in,
  input [2:0] shift_amt,
  output [2:0] b,   
  output valid
);

wire [2:0] pe_dout;
wire [2:0] pe_dout_rmo;
wire valid_rmo;

wire [7:0] shift_amt_rmo;
tothermo8 tormo(.x(shift_amt), .y(shift_amt_rmo));

wire [7:0] data_in_rmo;
assign data_in_rmo = ~shift_amt_rmo & data_in;

priority_encoder_8to3 pe( .in(data_in), .out(pe_dout), .valid(valid));
priority_encoder_8to3 pe_rmo( .in(data_in_rmo), .out(pe_dout_rmo), .valid(valid_rmo));

assign b = (valid_rmo == 1'b1) ? pe_dout_rmo : pe_dout;

endmodule
