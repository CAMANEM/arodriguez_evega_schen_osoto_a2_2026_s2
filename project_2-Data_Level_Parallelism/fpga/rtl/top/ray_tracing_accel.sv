module ray_tracing_accel (
    input  logic        clk_50mhz,
    input  logic        reset_n,
    output logic        busy,
    output logic        done,
    output logic        heartbeat,
    output logic        hit
);

    logic [63:0] cycle_count;
    logic ray_result_valid;
    logic signed [63:0] ray_discriminant;
    logic control_exec_start;
    logic signed [31:0] jtag_ray_ox;
    logic signed [31:0] jtag_ray_oy;
    logic signed [31:0] jtag_ray_dx;
    logic signed [31:0] jtag_ray_dy;
    logic signed [31:0] jtag_scene_cx;
    logic signed [31:0] jtag_scene_cy;
    logic signed [31:0] jtag_scene_r2;
    logic jtag_start;

    virtual_jtag_register jtag_register (
        .clk_sys(clk_50mhz),
        .reset_n(reset_n),
        .busy(busy),
        .done(done),
        .hit(hit),
        .discriminant(ray_discriminant),
        .cycle_count(cycle_count),
        .ray_ox(jtag_ray_ox),
        .ray_oy(jtag_ray_oy),
        .ray_dx(jtag_ray_dx),
        .ray_dy(jtag_ray_dy),
        .scene_cx(jtag_scene_cx),
        .scene_cy(jtag_scene_cy),
        .scene_r2(jtag_scene_r2),
        .jtag_start(jtag_start)
    );

    control_unit controller (
        .clk(clk_50mhz),
        .reset_n(reset_n),
        .start(jtag_start),
        .compute_done(ray_result_valid),
        .exec_start(control_exec_start),
        .busy(busy),
        .done(done),
        .cycle_count(cycle_count)
    );

    datapath_ray_intersection ray_datapath (
        .clk(clk_50mhz),
        .reset_n(reset_n),
        .input_valid(control_exec_start),
        .ray_ox(jtag_ray_ox),
        .ray_oy(jtag_ray_oy),
        .ray_dx(jtag_ray_dx),
        .ray_dy(jtag_ray_dy),
        .scene_cx(jtag_scene_cx),
        .scene_cy(jtag_scene_cy),
        .scene_r2(jtag_scene_r2),
        .result_valid(ray_result_valid),
        .hit(hit),
        .discriminant(ray_discriminant)
    );

    always_ff @(posedge clk_50mhz or negedge reset_n) begin
        if (!reset_n) begin
            heartbeat   <= 1'b0;
        end else begin
            heartbeat <= ~heartbeat;
        end
    end

endmodule
