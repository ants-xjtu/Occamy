`timescale 1ns / 1ps

module headdrop_scheduler
#(parameter NUM_PORT = 64)
(
    input clk,
    input rstn,

    // qlen
    input wire [32*64-1:0]qlen_s,
    // threshold
    input [31:0] T,
    output [5:0] out,
    output valid 
);

reg [63:0] bitmap;
wire [31:0]qlen[63:0];

integer i;
always @(*) begin
    bitmap = 0;
    for (i = 0; i < NUM_PORT; i = i + 1) begin
        if (qlen[i] > T) 
            bitmap[i] = 1'b1;
    end
end
genvar j;
generate
    for (j = 0; j < NUM_PORT; j = j + 1) begin : assign_loop
        assign qlen[j] = qlen_s[j * 32 +: 32];
    end
endgenerate

ppe_64_new ppe(
  .clk(clk),
  .rstn(rstn),
  .data_in(bitmap),
  .data_out(out),
  .valid(valid)
);

endmodule

