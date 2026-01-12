`timescale 1ns / 1ps

//  i_cell_ptr structure:
//      |15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
//                  | ports map |     |   cell_number   |

module admission #(
  parameter DATA_WIDTH = 512,
  parameter PD_WIDTH = 128,
  parameter PKT_PTR_WIDTH = 16,
  parameter CELL_PTR_WIDTH = 16,

  parameter DATA_SRAM_ADDR_WIDTH = 13,
  parameter PORT_NUM = 8 
)(
    input clk,
    input rstn,

    input      [DATA_WIDTH - 1:0] data_in,
    input              data_wr,
    input      [PKT_PTR_WIDTH - 1:0] i_cell_ptr_fifo_din,
    input              i_cell_ptr_fifo_wr,

    // TO sram 
    output [DATA_SRAM_ADDR_WIDTH - 1:0] sram_addr,
    output [DATA_WIDTH - 1:0] sram_din,
    output         sram_wr,

    // TO free queue 
    output reg       FQ_rd,
    output reg       FQ_rd_ack,
    input            FQ_empty,
    input      [CELL_PTR_WIDTH - 1:0] ptr_dout_s,
    input      [CELL_PTR_WIDTH - 1:0] FQ_head,

    output reg [PORT_NUM - 1:0] pd_qc_wr_ptr_wr_en,
    output reg [PD_WIDTH - 1:0] pd_qc_wr_ptr_din,
    input              pd_qc_ptr_full,

    // TO statistic 
    output reg        in,
    output reg [ 3:0] in_port,
    output reg [10:0] pkt_len_in,
    input      [PORT_NUM - 1:0] bitmap
);

    reg  [3:0] qc_portmap;

    reg          i_cell_data_fifo_rd;
    wire [DATA_WIDTH - 1:0] i_cell_data_fifo_dout;
    wire         i_cell_data_fifo_empty;

    reg  [  5:0] cell_number;
    reg  [  5:0] cell_number_pd;

    reg          i_cell_ptr_fifo_rd;
    wire [PKT_PTR_WIDTH - 1:0] i_cell_ptr_fifo_dout;
    wire         i_cell_ptr_fifo_full;
    wire         i_cell_ptr_fifo_empty;


    reg          drop;


    reg [CELL_PTR_WIDTH - 1:0] cell_head;
    reg [CELL_PTR_WIDTH - 1:0] cell_tail;

    // next_* for pipeline
    reg [5:0] next_cell_number;
    reg [5:0] next_cell_number_pd;
    reg [3:0] next_qc_portmap;

    reg select;
    reg [CELL_PTR_WIDTH - 1:0] ptr_dout_reg;

    reg [3:0] wr_state;
    always @(posedge clk) begin
        if (!rstn) begin
            wr_state            <=  0;
            FQ_rd               <=  0;
            i_cell_data_fifo_rd <=  0;
            i_cell_ptr_fifo_rd  <=  0;
            qc_portmap          <=  0;
            cell_number         <=  0;
            cell_number_pd      <=  0;
            pd_qc_wr_ptr_wr_en  <=  0;
            pd_qc_wr_ptr_din    <=  0;
            in                  <=  0;
            in_port             <=  0;
            pkt_len_in          <=  0;
            drop                <=  0;

            cell_head           <=  0;
            cell_tail <= 0;

            // next_* for pipeline
            next_cell_number    <=  0;
            next_cell_number_pd <=  0;
            next_qc_portmap     <=  0;

            ptr_dout_reg <= 0;

        end else begin

            case (wr_state)
                0: begin
                    pd_qc_wr_ptr_wr_en <= 0;
                    in <= 0;

                    if (~i_cell_ptr_fifo_empty & ~i_cell_data_fifo_empty & ~pd_qc_ptr_full & ~FQ_empty) begin
                        i_cell_ptr_fifo_rd <=  1;
                        i_cell_data_fifo_rd <= 1;
                        cell_number[5:0]    <=  i_cell_ptr_fifo_dout[5:0];
                        cell_number_pd[5:0] <=  i_cell_ptr_fifo_dout[5:0];

                        if (bitmap[i_cell_ptr_fifo_dout[11:8]] == 1'b0) begin
                            wr_state <=  4;
                            drop     <=  1;
                            FQ_rd <= 0;
                        end else begin
                            FQ_rd <=  1;
                            qc_portmap   <=  i_cell_ptr_fifo_dout[11:8];

                            select <= 1;
                            wr_state <= 1;
                        end
                    end
                    else begin 
                        i_cell_data_fifo_rd <= 0;
                        FQ_rd_ack <= 0;
                    end
                end
                1: begin
                    select <= 0;
                    FQ_rd <=  0; 
                    i_cell_ptr_fifo_rd  <=  0;
                    cell_number <=  cell_number - 1;

                    pkt_len_in <=  {cell_number_pd, 6'd0};

                    cell_head <= ptr_dout_s;

                    wr_state <=  2;
                end
                2: begin 
                    pd_qc_wr_ptr_wr_en <= 0;
                    in <= 0;
                    cell_number <= cell_number - 1;
                    if (cell_number == 1) begin 
                        cell_tail <= ptr_dout_s;

                        if (~i_cell_ptr_fifo_empty & ~i_cell_data_fifo_empty & ~pd_qc_ptr_full & ~FQ_empty) begin
                          i_cell_ptr_fifo_rd          <=  1;
                          next_cell_number[5:0]       <=  i_cell_ptr_fifo_dout[5:0];
                          next_cell_number_pd[5:0]    <=  i_cell_ptr_fifo_dout[5:0];
                          if (bitmap[i_cell_ptr_fifo_dout[11:8]]) begin 
                            wr_state                    <=  5; // in state 5: do 3 & 1
                          end else begin 
                            wr_state                    <=  6; // in state 5: do 3 & 4 
                          end
                          // FQ_rd                       <=  1; // DONT GO BACK TO HEAD, KEEP GOING!!!
                          next_qc_portmap             <=  i_cell_ptr_fifo_dout[11:8];
                        end else begin
                            FQ_rd_ack <= 1;
                            wr_state <= 3;
                            i_cell_data_fifo_rd <= 0;
                            // FQ_rd_ack <= 1;
                        end 
                    end
                end
                3: begin 
                    
                    update_regs();
                    
                    FQ_rd_ack <= 0;
                    wr_state <= 0;
                end
                4: begin 
                    pd_qc_wr_ptr_wr_en <= 0;
                    in <= 0;
                    i_cell_ptr_fifo_rd <= 0;
                    if (cell_number == 1) begin 
                        if (~i_cell_ptr_fifo_empty & ~i_cell_data_fifo_empty & ~pd_qc_ptr_full & ~FQ_empty) begin
                          i_cell_ptr_fifo_rd <= 1;
                          cell_number[5:0]    <=  i_cell_ptr_fifo_dout[5:0];
                          cell_number_pd[5:0] <=  i_cell_ptr_fifo_dout[5:0];
                          if (bitmap[i_cell_ptr_fifo_dout[11:8]] == 1'b0) begin

                          end else begin 
                            qc_portmap   <=  i_cell_ptr_fifo_dout[11:8];
                            FQ_rd <= 1;
                            select <= 1;
                            wr_state <= 1; 
                          end
                        end
                        else begin 
                          wr_state <= 0;
                          i_cell_data_fifo_rd <= 0;
                          drop <= 0;
                        end
                    end
                    else cell_number <= cell_number - 1;
                end
                5: begin  
                    // state 3
                    select <= 1;
                    cell_head <= ptr_dout_s;
                    update_regs(); 
                    FQ_rd_ack <= 0;

                    // next -> current
                    // cell_number <= next_cell_number [ref: //cell_number <=  cell_number - 1;]
                    cell_number     <=  next_cell_number - 1;
                    cell_number_pd  <=  next_cell_number_pd;
                    qc_portmap      <=  next_qc_portmap;

                    // state 1
                    FQ_rd               <=  0; 
                    i_cell_ptr_fifo_rd  <=  0;
                    //cell_number <=  cell_number - 1; [ref: //cell_number <=  next_cell_number;]

                    pkt_len_in <=  {cell_number_pd, 6'd0};

                    wr_state <=  2;

                end
                6: begin 
                    cell_head <= ptr_dout_s;
                    update_regs();
                    FQ_rd_ack <= 0;
                    
                    cell_number <= next_cell_number - 1;
                    cell_number_pd <= next_cell_number_pd;
                    qc_portmap <= next_qc_portmap;
                    
                    i_cell_ptr_fifo_rd <= 0;
                    wr_state <= 4;
                end
            endcase
        end
    end


    assign sram_wr   = (i_cell_data_fifo_rd & !drop);
    assign sram_addr = select ? FQ_head[DATA_SRAM_ADDR_WIDTH - 1:0] : ptr_dout_s[DATA_SRAM_ADDR_WIDTH - 1:0];
    assign sram_din  = i_cell_data_fifo_dout[DATA_WIDTH - 1:0];


    sfifo_ft_w512_d256 u_i_cell_fifo (
        .clk       (clk),
        .rst       (!rstn),
        .din       (data_in[DATA_WIDTH - 1:0]),
        .wr_en     (data_wr),
        .rd_en     (i_cell_data_fifo_rd),
        .dout      (i_cell_data_fifo_dout[DATA_WIDTH - 1:0]),
        .full      (),
        .empty     (i_cell_data_fifo_empty),
        .data_count()
    );

    sfifo_ft_w16_d32 u_i_ptr_fifo (
        .clk       (clk),
        .rst       (!rstn),
        .din       (i_cell_ptr_fifo_din),
        .wr_en     (i_cell_ptr_fifo_wr),
        .rd_en     (i_cell_ptr_fifo_rd),
        .dout      (i_cell_ptr_fifo_dout),
        .full      (i_cell_ptr_fifo_full),
        .empty     (i_cell_ptr_fifo_empty),
        .data_count()
    );

task update_regs();
  begin
    pd_qc_wr_ptr_din <= {
        47'b0,
        cell_head,
        cell_tail,
        {cell_number_pd, 6'd0}, // pkt_len = cell_num * 64
        cell_number_pd[5:0],
        32'b0
    };  
    case (qc_portmap)
      0: pd_qc_wr_ptr_wr_en <= 4'b0001;
      1: pd_qc_wr_ptr_wr_en <= 4'b0010;
      2: pd_qc_wr_ptr_wr_en <= 4'b0100;
      3: pd_qc_wr_ptr_wr_en <= 4'b1000;
      default: pd_qc_wr_ptr_wr_en <= 4'b0000;
    endcase 
    in <= 1;
    in_port <= qc_portmap;
  end
endtask;

endmodule
