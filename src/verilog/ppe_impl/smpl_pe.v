`timescale 1ns / 1ps
module smpl_pe #(
  parameter WIDTH = 512
)(
    input              [WIDTH-1: 0]        Req                        ,
    output             [WIDTH-1: 0]        Gnt                        ,
    output                              valid                       
);
    assign Gnt = Req & (~Req + 1);
    assign valid = |Req;
                                                                   
endmodule
