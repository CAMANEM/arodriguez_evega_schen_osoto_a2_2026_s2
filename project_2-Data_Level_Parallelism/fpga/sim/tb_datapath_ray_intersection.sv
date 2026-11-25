`timescale 1ns/1ps

module tb_datapath_ray_intersection;

    logic clk = 1'b0;
    logic reset_n = 1'b0;
    logic input_valid = 1'b0;
    logic signed [31:0] ray_ox;
    logic signed [31:0] ray_oy;
    logic signed [31:0] ray_dx;
    logic signed [31:0] ray_dy;
    logic signed [31:0] scene_cx;
    logic signed [31:0] scene_cy;
    logic signed [31:0] scene_r2;
    logic result_valid;
    logic hit;
    logic signed [63:0] discriminant;

    localparam logic signed [31:0] Q_ONE = 32'sd65536;

    always #10 clk = ~clk;

    datapath_ray_intersection dut (
        .clk,
        .reset_n,
        .input_valid,
        .ray_ox,
        .ray_oy,
        .ray_dx,
        .ray_dy,
        .scene_cx,
        .scene_cy,
        .scene_r2,
        .result_valid,
        .hit,
        .discriminant
    );

    initial begin
        ray_ox = -32'sd131072; // -2.0
        ray_oy = 32'sd0;
        ray_dx = Q_ONE;
        ray_dy = 32'sd0;
        scene_cx = 32'sd0;
        scene_cy = 32'sd0;
        scene_r2 = Q_ONE; // radius squared = 1.0

        #25 reset_n = 1'b1;
        @(negedge clk);
        input_valid = 1'b1;
        @(posedge clk);
        #1;
        input_valid = 1'b0;

        if (!result_valid || !hit)
            $fatal(1, "Expected a hit for ray (-2,0) -> (1,0)");

        $display("PASS: hit=%0d discriminant=%0d", hit, discriminant);
        $finish;
    end

endmodule
