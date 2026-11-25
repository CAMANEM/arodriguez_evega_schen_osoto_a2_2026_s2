module datapath_ray_intersection (
    input  logic               clk,
    input  logic               reset_n,
    input  logic               input_valid,
    input  logic signed [31:0] ray_ox,
    input  logic signed [31:0] ray_oy,
    input  logic signed [31:0] ray_dx,
    input  logic signed [31:0] ray_dy,
    input  logic signed [31:0] scene_cx,
    input  logic signed [31:0] scene_cy,
    input  logic signed [31:0] scene_r2,
    output logic               result_valid,
    output logic               hit,
    output logic signed [63:0] discriminant
);

    logic signed [31:0] ocx;
    logic signed [31:0] ocy;
    logic signed [63:0] dx_squared;
    logic signed [63:0] dy_squared;
    logic signed [63:0] dx_ocx;
    logic signed [63:0] dy_ocy;
    logic signed [63:0] ocx_squared;
    logic signed [63:0] ocy_squared;
    logic signed [63:0] a_q16_16;
    logic signed [63:0] b_q16_16;
    logic signed [63:0] c_q16_16;
    logic signed [127:0] b_squared;
    logic signed [127:0] four_ac;
    logic signed [127:0] delta_full;

    always_comb begin
        ocx = $signed(ray_ox) - $signed(scene_cx);
        ocy = $signed(ray_oy) - $signed(scene_cy);

        dx_squared = $signed(ray_dx) * $signed(ray_dx);
        dy_squared = $signed(ray_dy) * $signed(ray_dy);
        dx_ocx = $signed(ray_dx) * $signed(ocx);
        dy_ocy = $signed(ray_dy) * $signed(ocy);
        ocx_squared = $signed(ocx) * $signed(ocx);
        ocy_squared = $signed(ocy) * $signed(ocy);

        // Products are Q32.32; shift back to Q16.16.
        a_q16_16 = (dx_squared + dy_squared) >>> 16;
        b_q16_16 = ((dx_ocx + dy_ocy) <<< 1) >>> 16;
        c_q16_16 = ((ocx_squared + ocy_squared) >>> 16) - $signed(scene_r2);

        // b² and 4ac are retained at the wider intermediate precision.
        b_squared = $signed(b_q16_16) * $signed(b_q16_16);
        four_ac = ($signed(a_q16_16) * $signed(c_q16_16)) <<< 2;
        delta_full = b_squared - four_ac;
    end

    always_ff @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            result_valid <= 1'b0;
            hit           <= 1'b0;
            discriminant <= '0;
        end else begin
            result_valid <= input_valid;
            if (input_valid) begin
                discriminant <= delta_full[63:0];
                hit <= (a_q16_16 != 0) && (delta_full >= 0);
            end
        end
    end

endmodule
