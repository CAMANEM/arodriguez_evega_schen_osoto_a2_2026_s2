module virtual_jtag_register (
    input  logic                    clk_sys,
    input  logic                    reset_n,
    input  logic                    busy,
    input  logic                    done,
    input  logic                    hit,
    input  logic signed [63:0]      discriminant,
    input  logic        [63:0]      cycle_count,
    output logic signed [31:0]      ray_ox,
    output logic signed [31:0]      ray_oy,
    output logic signed [31:0]      ray_dx,
    output logic signed [31:0]      ray_dy,
    output logic signed [31:0]      scene_cx,
    output logic signed [31:0]      scene_cy,
    output logic signed [31:0]      scene_r2,
    output logic                    jtag_start
);

    localparam logic [2:0] IR_WRITE         = 3'b001;
    localparam logic [2:0] IR_READ_DATA     = 3'b010;
    localparam logic [2:0] IR_STATUS        = 3'b011;
    localparam logic [2:0] IR_SET_ADDRESS   = 3'b100;
    localparam logic [2:0] IR_READ_REQUEST  = 3'b101;

    localparam logic [3:0] REG_RAY_OX       = 4'd0;
    localparam logic [3:0] REG_RAY_OY       = 4'd1;
    localparam logic [3:0] REG_RAY_DX       = 4'd2;
    localparam logic [3:0] REG_RAY_DY       = 4'd3;
    localparam logic [3:0] REG_SCENE_CX     = 4'd4;
    localparam logic [3:0] REG_SCENE_CY     = 4'd5;
    localparam logic [3:0] REG_SCENE_R2     = 4'd6;
    localparam logic [3:0] REG_CONTROL      = 4'd7;
    localparam logic [3:0] REG_STATUS       = 4'd7;
    localparam logic [3:0] REG_DISCRIMINANT_LO = 4'd8;
    localparam logic [3:0] REG_DISCRIMINANT_HI = 4'd9;
    localparam logic [3:0] REG_CYCLES_LO    = 4'd10;
    localparam logic [3:0] REG_CYCLES_HI    = 4'd11;

    wire [2:0] ir_in;
    wire tck;
    wire tdi;
    wire tdo;
    wire capture_dr;
    wire shift_dr;
    wire update_dr;

    logic [31:0] shift_register;
    logic [36:0] mailbox_command;
    logic [3:0] selected_address_tck;
    logic request_toggle;
    logic request_sync_1;
    logic request_sync_2;
    logic acknowledge_toggle;
    logic acknowledge_sync_1;
    logic acknowledge_sync_2;
    logic [31:0] readback_system;
    logic [31:0] readback_tck;
    logic done_sticky;
    logic protocol_error;
    logic [1:0] sys_reset_sync;
    logic [1:0] tck_reset_sync;
    logic [31:0] read_data_system;

    wire sys_reset_n = sys_reset_sync[1];
    wire tck_reset_n = tck_reset_sync[1];
    wire mailbox_ready = acknowledge_sync_2 == request_toggle;
    wire command_is_read = mailbox_command[36];
    wire [3:0] command_address = mailbox_command[35:32];
    wire [31:0] command_value = mailbox_command[31:0];

    assign tdo = shift_register[0];

    always_comb begin
        case (command_address)
            REG_RAY_OX:          read_data_system = ray_ox;
            REG_RAY_OY:          read_data_system = ray_oy;
            REG_RAY_DX:          read_data_system = ray_dx;
            REG_RAY_DY:          read_data_system = ray_dy;
            REG_SCENE_CX:        read_data_system = scene_cx;
            REG_SCENE_CY:        read_data_system = scene_cy;
            REG_SCENE_R2:        read_data_system = scene_r2;
            REG_STATUS:          read_data_system = {
                28'd0, protocol_error, hit, done_sticky, busy
            };
            REG_DISCRIMINANT_LO: read_data_system = discriminant[31:0];
            REG_DISCRIMINANT_HI: read_data_system = discriminant[63:32];
            REG_CYCLES_LO:       read_data_system = cycle_count[31:0];
            REG_CYCLES_HI:       read_data_system = cycle_count[63:32];
            default:             read_data_system = 32'd0;
        endcase
    end

    sld_virtual_jtag #(
        .sld_auto_instance_index("NO"),
        .sld_instance_index(0),
        .sld_ir_width(3)
    ) virtual_jtag (
        .ir_out(3'b000),
        .tdo(tdo),
        .ir_in(ir_in),
        .tck(tck),
        .tdi(tdi),
        .virtual_state_cdr(capture_dr),
        .virtual_state_cir(),
        .virtual_state_e1dr(),
        .virtual_state_e2dr(),
        .virtual_state_pdr(),
        .virtual_state_sdr(shift_dr),
        .virtual_state_udr(update_dr),
        .virtual_state_uir()
    );

    always_ff @(posedge clk_sys or negedge reset_n) begin
        if (!reset_n)
            sys_reset_sync <= 2'b00;
        else
            sys_reset_sync <= {sys_reset_sync[0], 1'b1};
    end

    always_ff @(posedge tck or negedge reset_n) begin
        if (!reset_n)
            tck_reset_sync <= 2'b00;
        else
            tck_reset_sync <= {tck_reset_sync[0], 1'b1};
    end

    always_ff @(posedge clk_sys or negedge sys_reset_n) begin
        if (!sys_reset_n) begin
            request_sync_1    <= 1'b0;
            request_sync_2    <= 1'b0;
            acknowledge_toggle <= 1'b0;
            readback_system    <= 32'd0;
            ray_ox             <= 32'sd0;
            ray_oy             <= 32'sd0;
            ray_dx             <= 32'sd0;
            ray_dy             <= 32'sd0;
            scene_cx           <= 32'sd0;
            scene_cy           <= 32'sd0;
            scene_r2           <= 32'sd0;
            jtag_start         <= 1'b0;
            done_sticky        <= 1'b0;
            protocol_error     <= 1'b0;
        end else begin
            request_sync_1 <= request_toggle;
            request_sync_2 <= request_sync_1;
            jtag_start     <= 1'b0;

            if (done)
                done_sticky <= 1'b1;

            if (request_sync_2 != acknowledge_toggle) begin
                if (command_is_read) begin
                    readback_system <= read_data_system;
                    if (command_address > REG_CYCLES_HI)
                        protocol_error <= 1'b1;
                end else if (busy && command_address <= REG_SCENE_R2) begin
                    protocol_error <= 1'b1;
                end else begin
                    case (command_address)
                        REG_RAY_OX:   ray_ox   <= $signed(command_value);
                        REG_RAY_OY:   ray_oy   <= $signed(command_value);
                        REG_RAY_DX:   ray_dx   <= $signed(command_value);
                        REG_RAY_DY:   ray_dy   <= $signed(command_value);
                        REG_SCENE_CX: scene_cx <= $signed(command_value);
                        REG_SCENE_CY: scene_cy <= $signed(command_value);
                        REG_SCENE_R2: scene_r2 <= $signed(command_value);
                        REG_CONTROL: begin
                            if (command_value[2])
                                protocol_error <= 1'b0;
                            if (command_value[1])
                                done_sticky <= 1'b0;
                            if (command_value[0]) begin
                                if (busy) begin
                                    protocol_error <= 1'b1;
                                end else begin
                                    jtag_start  <= 1'b1;
                                    done_sticky <= 1'b0;
                                end
                            end
                        end
                        default: protocol_error <= 1'b1;
                    endcase
                end
                acknowledge_toggle <= request_sync_2;
            end
        end
    end

    always_ff @(posedge tck or negedge tck_reset_n) begin
        if (!tck_reset_n) begin
            shift_register        <= 32'd0;
            mailbox_command       <= 37'd0;
            selected_address_tck  <= 4'd0;
            request_toggle        <= 1'b0;
            acknowledge_sync_1    <= 1'b0;
            acknowledge_sync_2    <= 1'b0;
            readback_tck          <= 32'd0;
        end else begin
            acknowledge_sync_1 <= acknowledge_toggle;
            acknowledge_sync_2 <= acknowledge_sync_1;

            if (mailbox_ready)
                readback_tck <= readback_system;

            if (capture_dr) begin
                case (ir_in)
                    IR_READ_DATA: shift_register <= readback_tck;
                    IR_STATUS:    shift_register <= {31'd0, mailbox_ready};
                    default:      shift_register <= 32'd0;
                endcase
            end else if (shift_dr) begin
                shift_register <= {tdi, shift_register[31:1]};
            end else if (update_dr) begin
                case (ir_in)
                    IR_SET_ADDRESS: begin
                        selected_address_tck <= shift_register[3:0];
                    end
                    IR_WRITE: begin
                        if (mailbox_ready) begin
                            mailbox_command <= {
                                1'b0, selected_address_tck, shift_register
                            };
                            request_toggle <= ~request_toggle;
                        end
                    end
                    IR_READ_REQUEST: begin
                        if (mailbox_ready) begin
                            mailbox_command <= {
                                1'b1, selected_address_tck, 32'd0
                            };
                            request_toggle <= ~request_toggle;
                        end
                    end
                    default: begin
                    end
                endcase
            end
        end
    end

endmodule
