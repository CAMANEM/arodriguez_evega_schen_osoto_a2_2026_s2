module control_unit (
    input  logic        clk,
    input  logic        reset_n,
    input  logic        start,
    input  logic        compute_done,
    output logic        exec_start,
    output logic        busy,
    output logic        done,
    output logic [63:0] cycle_count
);

    typedef enum logic [1:0] {
        IDLE,
        EXECUTE,
        DONE
    } state_t;

    state_t state;

    always_ff @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            state       <= IDLE;
            exec_start  <= 1'b0;
            busy        <= 1'b0;
            done        <= 1'b0;
            cycle_count <= 64'd0;
        end else begin
            exec_start <= 1'b0;
            done       <= 1'b0;

            case (state)
                IDLE: begin
                    if (start) begin
                        busy        <= 1'b1;
                        cycle_count <= 64'd0;
                        exec_start  <= 1'b1;
                        state       <= EXECUTE;
                    end
                end

                EXECUTE: begin
                    cycle_count <= cycle_count + 1'b1;
                    if (compute_done) begin
                        busy  <= 1'b0;
                        done  <= 1'b1;
                        state <= DONE;
                    end
                end

                DONE: begin
                    state <= IDLE;
                end

                default: begin
                    state <= IDLE;
                end
            endcase
        end
    end

endmodule
