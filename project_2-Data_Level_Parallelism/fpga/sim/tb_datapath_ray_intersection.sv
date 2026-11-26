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

    task automatic check_case(
        input string case_name,
        input logic signed [31:0] ox,
        input logic signed [31:0] oy,
        input logic signed [31:0] dx,
        input logic signed [31:0] dy,
        input logic signed [31:0] r2,
        input logic expected_hit,
        input logic signed [63:0] expected_discriminant
    );
        begin
            @(negedge clk);
            ray_ox = ox;
            ray_oy = oy;
            ray_dx = dx;
            ray_dy = dy;
            scene_cx = 32'sd0;
            scene_cy = 32'sd0;
            scene_r2 = r2;
            input_valid = 1'b1;

            @(posedge clk);
            #1;
            if (!result_valid)
                $fatal(1, "%s: result_valid was not asserted", case_name);
            if (hit !== expected_hit)
                $fatal(1, "%s: expected hit=%0d, got hit=%0d",
                       case_name, expected_hit, hit);
            if (discriminant !== expected_discriminant)
                $fatal(1, "%s: expected discriminant=%0d, got %0d",
                       case_name, expected_discriminant, discriminant);

            input_valid = 1'b0;
        end
    endtask

    initial begin
        #25 reset_n = 1'b1;

        check_case(
            "forward hit",
            -32'sd131072, 32'sd0, Q_ONE, 32'sd0, Q_ONE,
            1'b1, 64'sd17179869184
        );
        check_case(
            "ray pointing away",
            32'sd131072, 32'sd0, Q_ONE, 32'sd0, Q_ONE,
            1'b0, 64'sd17179869184
        );
        check_case(
            "origin inside circle",
            32'sd0, 32'sd0, Q_ONE, 32'sd0, Q_ONE,
            1'b0, 64'sd17179869184
        );
        check_case(
            "miss",
            -32'sd131072, 32'sd131072, Q_ONE, 32'sd0, Q_ONE,
            1'b0, -64'sd51539607552
        );
        check_case(
            "tangent",
            -32'sd131072, Q_ONE, Q_ONE, 32'sd0, Q_ONE,
            1'b1, 64'sd0
        );
        check_case(
            "near root exactly zero",
            -Q_ONE, 32'sd0, Q_ONE, 32'sd0, Q_ONE,
            1'b0, 64'sd17179869184
        );
        check_case(
            "zero direction",
            -32'sd131072, 32'sd0, 32'sd0, 32'sd0, Q_ONE,
            1'b0, 64'sd0
        );

        $display("PASS: all ray-circle datapath cases");
        $finish;
    end

endmodule
