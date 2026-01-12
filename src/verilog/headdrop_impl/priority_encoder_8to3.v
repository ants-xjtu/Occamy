module priority_encoder_8to3 (
    input  wire [7:0] in,   // 8位输入
    output [2:0] out,  // 3位编码输出
    output valid       // 输出有效标志
);


wire [3:0] in0;
wire [3:0] in1;

assign in0 = in[3:0];
assign in1 = in[7:4];

wire [1:0] out0; 
wire [1:0] out1;

wire valid0, valid1;

assign valid = valid0 | valid1;
assign out[2] = ~valid0 & valid1;
assign out[1] = valid0 & out0[1] | ~valid0 & valid1 & out1[1];
assign out[0] = valid0 & out0[0] | ~valid0 & valid1 & out1[0];

priority_encoder_4to2 pe0(.in(in0), .out(out0), .valid(valid0));
priority_encoder_4to2 pe1(.in(in1), .out(out1), .valid(valid1));

endmodule

