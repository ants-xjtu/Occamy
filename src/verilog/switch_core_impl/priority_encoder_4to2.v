`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2024/10/15 10:15:22
// Design Name: 
// Module Name: priority_encoder_4to2
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


module priority_encoder_4to2(
    input [3:0] in, 
    output [1:0] out, 
    output valid
    );
    assign out[0] = ~in[0] & in[1] | ~in[0] & ~in[2] & in[3];
    assign out[1] = ~in[0] & ~in[1] & in[2] | ~in[0] & ~in[1] & ~in[2] & in[3];
    assign valid = in[0] | in[1] | in[2] | in[3];
endmodule
