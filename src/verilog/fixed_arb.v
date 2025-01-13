`timescale 1ns / 1ps

module fixed_arb
(
input 	[5:0]	req,
output 	[5:0]	grant
);
wire	[5:0] req_sub_one;
assign	req_sub_one = req - 1'b1;
 
assign	grant = req & (~req_sub_one);
endmodule
