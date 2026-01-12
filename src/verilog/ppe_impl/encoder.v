`timescale 1ns / 1ps
module encoder #(
  parameter WIDTH = 1024,
  parameter LOG_W = 10
)(
    input              [WIDTH-1: 0]        in                         ,
    output reg         [LOG_W-1: 0]        out
);

integer i;
  always @(*) begin
    out = 0;
    for (i = 0; i < WIDTH; i = i + 1) begin : search
      if (in[i]) begin
        out = i[LOG_W-1:0];  // When signal[i] is 1, output its position
      end
    end 
  end

endmodule
