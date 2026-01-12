
module ppe_64_new (
  input clk,
  input rstn,
  input [63:0] data_in,
  output reg [5:0] data_out,
  output reg valid
);

reg [7:0] ppe_top_din;
wire [2:0] ppe_top_dout;
integer i;
always@(*) begin
  ppe_top_din = 0;
  for(i = 0; i < 8; i = i + 1) begin
    ppe_top_din[i] = |data_in[i *8+: 8];
  end
end

reg clk_top;

wire [7:0] ppe_din[7:0];
genvar j;
generate
    for (j = 0; j < 8; j = j + 1) begin
        assign ppe_din[j] = data_in[j*8+: 8];
    end
endgenerate

wire [2:0] ppe_dout[7:0];
wire top_ppe_valid;
wire [7:0] ppe_valid;
wire [7:0] ppe_over;
reg [2:0] curr_top_dout;
reg [7:0] enable;
reg valid_1;
always@(posedge clk or negedge rstn) begin
    if(!rstn) begin
        curr_top_dout <= 0;
        clk_top <= 0;
        enable <= 8'b1;
        valid <= 0;
        valid_1 <= 0;
    end
    else begin
        data_out <= {curr_top_dout, ppe_dout[curr_top_dout]};
        valid <= ppe_valid[curr_top_dout];
        valid_1 <= valid;
        if(ppe_over[curr_top_dout] || (valid ^ valid_1)) begin
            curr_top_dout <= ppe_top_dout;
        enable[curr_top_dout] <= 0;
        enable[ppe_top_dout] <= 1;
        clk_top<= 1;
        end
        else if(clk_top == 1) clk_top <= 0;
    end
end




ppe_8 ppe0( .clk(clk), .rstn(rstn), .data_in(ppe_din[0]),  .b(ppe_dout[0]),  .enable(enable[0]),  .valid(ppe_valid[0]) , .over(ppe_over[0]));
ppe_8 ppe1( .clk(clk), .rstn(rstn), .data_in(ppe_din[1]),  .b(ppe_dout[1]),  .enable(enable[1]),  .valid(ppe_valid[1]) , .over(ppe_over[1]));
ppe_8 ppe2( .clk(clk), .rstn(rstn), .data_in(ppe_din[2]),  .b(ppe_dout[2]),  .enable(enable[2]),  .valid(ppe_valid[2]) , .over(ppe_over[2]));
ppe_8 ppe3( .clk(clk), .rstn(rstn), .data_in(ppe_din[3]),  .b(ppe_dout[3]),  .enable(enable[3]),  .valid(ppe_valid[3]) , .over(ppe_over[3]));
ppe_8 ppe4( .clk(clk), .rstn(rstn), .data_in(ppe_din[4]),  .b(ppe_dout[4]),  .enable(enable[4]),  .valid(ppe_valid[4]) , .over(ppe_over[4]));
ppe_8 ppe5( .clk(clk), .rstn(rstn), .data_in(ppe_din[5]),  .b(ppe_dout[5]),  .enable(enable[5]),  .valid(ppe_valid[5]) , .over(ppe_over[5]));
ppe_8 ppe6( .clk(clk), .rstn(rstn), .data_in(ppe_din[6]),  .b(ppe_dout[6]),  .enable(enable[6]),  .valid(ppe_valid[6]) , .over(ppe_over[6]));
ppe_8 ppe7( .clk(clk), .rstn(rstn), .data_in(ppe_din[7]),  .b(ppe_dout[7]),  .enable(enable[7]),  .valid(ppe_valid[7]) , .over(ppe_over[7]));

ppe_8 ppe_top( .clk(clk_top), .rstn(rstn), .data_in(ppe_top_din), .b(ppe_top_dout), .enable(1'b1), .valid(top_ppe_valid), .over());
endmodule
