# Run with Quartus Programmer Tcl, not the System Console Tcl prompt:
# quartus_stp -t virtual_jtag_register_test.tcl

set hardware_names [get_hardware_names]
if {[llength $hardware_names] == 0} {
    error "No JTAG hardware found. Connect the DE1-SoC USB-Blaster."
}
set hardware_name [lindex $hardware_names 0]

set device_names [get_device_names -hardware_name $hardware_name]
set device_name ""
foreach candidate $device_names {
    if {[string match "*5CSE(*" $candidate]} {
        set device_name $candidate
        break
    }
}
if {$device_name eq ""} {
    error "Cyclone V FPGA entry (5CSE) not found on $hardware_name. Devices detected: [join $device_names {, }]"
}

open_device -hardware_name $hardware_name -device_name $device_name
device_lock -timeout 10000

set IR_WRITE 1
set IR_READ_DATA 2
set IR_STATUS 3
set IR_SET_ADDRESS 4
set IR_READ_REQUEST 5
set zero_hex "00000000"
set zero_bits [format "%032b" 0]

proc bits32 {value} {
    return [format "%032b" [expr {$value & 0xFFFFFFFF}]]
}

proc read_mailbox_ready {} {
    global IR_STATUS
    device_virtual_ir_shift -instance_index 0 -ir_value $IR_STATUS -no_captured_ir_value
    set ready_hex [device_virtual_dr_shift \
        -instance_index 0 \
        -length 1 \
        -dr_value 0 \
        -value_in_hex]
    return [expr 0x$ready_hex]
}

proc wait_for_mailbox_ready {phase} {
    for {set attempt 0} {$attempt < 100} {incr attempt} {
        if {[read_mailbox_ready]} {
            return
        }
    }
    error "Virtual JTAG mailbox did not acknowledge the $phase operation."
}

proc select_register {address} {
    global IR_SET_ADDRESS
    device_virtual_ir_shift -instance_index 0 -ir_value $IR_SET_ADDRESS -no_captured_ir_value
    device_virtual_dr_shift \
        -instance_index 0 \
        -length 32 \
        -dr_value [bits32 $address] \
        -no_captured_dr_value
}

proc write_register {address value} {
    global IR_WRITE
    wait_for_mailbox_ready "before writing register $address"
    select_register $address
    device_virtual_ir_shift -instance_index 0 -ir_value $IR_WRITE -no_captured_ir_value
    device_virtual_dr_shift \
        -instance_index 0 \
        -length 32 \
        -dr_value [bits32 $value] \
        -no_captured_dr_value
    wait_for_mailbox_ready "write to register $address"
}

proc read_register {address} {
    global IR_READ_REQUEST IR_READ_DATA zero_bits zero_hex
    wait_for_mailbox_ready "before reading register $address"
    select_register $address
    device_virtual_ir_shift -instance_index 0 -ir_value $IR_READ_REQUEST -no_captured_ir_value
    device_virtual_dr_shift \
        -instance_index 0 \
        -length 32 \
        -dr_value $zero_bits \
        -no_captured_dr_value
    wait_for_mailbox_ready "read request for register $address"

    device_virtual_ir_shift -instance_index 0 -ir_value $IR_READ_DATA -no_captured_ir_value
    set value_hex [device_virtual_dr_shift \
        -instance_index 0 \
        -length 32 \
        -dr_value $zero_hex \
        -value_in_hex]
    return [expr 0x$value_hex]
}

proc run_and_check_case {case_name expected_hit expected_disc_hi expected_disc_lo} {
    write_register 7 6
    write_register 7 1

    set completed 0
    for {set attempt 0} {$attempt < 100} {incr attempt} {
        set status [read_register 7]
        if {$status & 8} {
            error [format "%s: FPGA reported protocol error (status 0x%08X)." \
                $case_name $status]
        }
        if {(($status & 2) != 0) && (($status & 1) == 0)} {
            set completed 1
            break
        }
    }
    if {!$completed} {
        error "$case_name: ray intersection timed out."
    }

    set actual_hit [expr {($status & 4) != 0}]
    if {$actual_hit != $expected_hit} {
        error [format "%s: expected hit=%d, got status 0x%08X." \
            $case_name $expected_hit $status]
    }

    set discriminant_lo [read_register 8]
    set discriminant_hi [read_register 9]
    if {$discriminant_hi != $expected_disc_hi ||
        $discriminant_lo != $expected_disc_lo} {
        error [format "%s: expected discriminant 0x%08X%08X, got 0x%08X%08X." \
            $case_name $expected_disc_hi $expected_disc_lo \
            $discriminant_hi $discriminant_lo]
    }

    puts [format "PASS: %s hit=%d discriminant=0x%08X%08X" \
        $case_name $actual_hit $discriminant_hi $discriminant_lo]
}

set input_registers {
    0 -131072
    1 0
    2 65536
    3 0
    4 0
    5 0
    6 65536
}

foreach {address value} $input_registers {
    write_register $address $value
    set readback [read_register $address]
    set expected [expr {$value & 0xFFFFFFFF}]
    if {$readback != $expected} {
        set status [read_register 7]
        error [format "Register %d mismatch: expected 0x%08X, got 0x%08X; accelerator status=0x%08X (busy=bit0, done=bit1, hit=bit2, protocol_error=bit3)" \
            $address $expected $readback $status]
    }
}

run_and_check_case "forward hit" 1 4 0

write_register 0 131072
run_and_check_case "ray pointing away" 0 4 0

write_register 0 0
run_and_check_case "origin inside circle" 0 4 0

write_register 0 -131072
write_register 1 65536
run_and_check_case "tangent" 1 0 0

write_register 0 -65536
write_register 1 0
run_and_check_case "near root exactly zero" 0 4 0

device_unlock
close_device

puts "PASS: all JTAG ray-circle cases"
