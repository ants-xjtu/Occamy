`timescale 1ns / 1ps
module thermometer #(
  parameter WIDTH = 1024,
  parameter LOG_W = 10
)(
    input              [LOG_W-1: 0]        enc                        ,
    output             [WIDTH-1: 0]        thermo                      
);
    genvar i;
    generate
        for (i = 0; i < WIDTH; i = i + 1) begin : gen_loop
            assign thermo[i] = (i < enc) ? 1'b1 : 1'b0;
        end
    endgenerate
endmodule
