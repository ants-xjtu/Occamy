`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2024/09/12 15:22:48
// Design Name: 
// Module Name: statistics_v2
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

module statistics_v2 #(
  QLEN_WIDTH = 11,
  PORT_NUM = 8,
  ALPHA_SHIFT = 3,
  USE_DRR = 1'b1
)(
    input clk,
    input rstn,
    input in,
    input out,
    input [3:0] in_port,
    input [3:0] out_port,

    output reg [PORT_NUM - 1:0] bitmap,
    output reg [PORT_NUM - 1:0] bitmap_dt,

    output [QLEN_WIDTH * PORT_NUM - 1:0] qlen_all
    );
    
    parameter alpha_shift = ALPHA_SHIFT;
    parameter alpha_shift_pq = 3;
    parameter buffer_size = 2047;// 128 pkts 

    wire [1:0] sig;
    reg  [QLEN_WIDTH - 1:0] qlen[PORT_NUM - 1:0];
    reg [QLEN_WIDTH - 1:0] T;

    genvar i;
    generate 
        for (i = 0; i < PORT_NUM; i = i + 1) begin: loop 
            assign qlen_all[QLEN_WIDTH*(i+1) - 1:QLEN_WIDTH*i] = qlen[i];
        end
    endgenerate 

    assign sig = {in, out};
    wire [PORT_NUM - 1:0] bitmap_1;
    wire [PORT_NUM - 1:0] bitmap_dt_1;


    integer k;
    integer occ_size;
    always @(posedge clk) begin
        if (!rstn) begin
            for (k = 0; k < PORT_NUM; k = k + 1) begin 
                qlen[k] <= 0;
            end
        end else begin
            case (sig)
            2'b01: qlen[out_port] <= qlen[out_port] - 1;
            2'b10: qlen[in_port] <= qlen[in_port] + 1;
            2'b11: begin 
            if (in_port != out_port) begin 
                qlen[in_port] <= qlen[in_port] + 1;
                qlen[out_port] <= qlen[out_port] - 1;
            end
            end
            endcase

            occ_size = 0;
            for (k = 0; k < PORT_NUM; k = k + 1) begin 
                occ_size = occ_size + qlen[k];
            end
            T <= buffer_size - occ_size;

            bitmap <= bitmap_1;
            bitmap_dt <= bitmap_dt_1;
        end
    end


    // integer j;
    // always @* begin 
    //     T = buffer_size;
    //     for (j = 0; j < PORT_NUM; j = j + 1) begin 
    //         T = T - qlen[j];
    //     end
    // end

generate if (~USE_DRR) begin:a
  
  wire [14:0] prot_T = T > buffer_size ? 0 : T;
  genvar m;
  // generate 
      for (m = 1; m < PORT_NUM; m = m + 1) begin: loop2
          assign bitmap_1[m] = (qlen[m] < prot_T);
          assign bitmap_dt_1[m] = (qlen[m] + 1 < prot_T);
      end
  // endgenerate
  assign bitmap_1[0] = ((qlen[0]) < (prot_T << alpha_shift_pq));
  assign bitmap_dt_1[0] = (((qlen[0] + 1)) < (prot_T << alpha_shift_pq));

end else begin:b
  wire [14:0] alpha_T;
  // avoid overflow 
  assign alpha_T = T > buffer_size ? 0 : T << alpha_shift;
  genvar m;
  // generate 
      for (m = 0; m < PORT_NUM; m = m + 1) begin: loop2
          assign bitmap_1[m] = ((qlen[m]) < alpha_T) ;
          assign bitmap_dt_1[m] = ((qlen[m] + 1) < alpha_T);
      end
  // endgenerate
end
endgenerate

endmodule
