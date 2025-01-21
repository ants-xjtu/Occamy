module ppe_8(
  input clk,            
  input rstn,
  input [7:0] data_in,
  input enable,
  output [2:0] b,   
  output valid,
  output over
);

wire [2:0] pe_dout;

wire [7:0] data_in_pe;
wire valid;

reg [2:0] shift_amt;

wire [2:0] pe_dout_rmo;

assign data_in_pe = data_in;

wire [7:0] shift_amt_rmo;
tothermo8 tormo(.x(shift_amt), .y(shift_amt_rmo));


wire [7:0] data_in_rmo;
assign data_in_rmo = ~shift_amt_rmo & data_in;

priority_encoder_8to3 pe( .in(data_in), .out(pe_dout), .valid(valid));
priority_encoder_8to3 pe_rmo( .in(data_in_rmo), .out(pe_dout_rmo), .valid(valid_rmo));

assign b = (valid_rmo == 1'b1) ? pe_dout_rmo : pe_dout;


always @(posedge clk or negedge rstn) begin
  if (!rstn) begin
    shift_amt <= 0;
  end else begin
    if(enable)
      shift_amt <= b + 1;
    else if(over)
      shift_amt <= 0;
  end
end

assign shift_amt_1 = shift_amt[0] & shift_amt[1] & shift_amt[2];
assign b_1 = b[0] & b[1] & b[2];

assign over = (b < shift_amt) | (b_1);

endmodule
