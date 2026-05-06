# I2C Implementation

The I2C implementation is a **software bit-bang master** using the AVR's USI (Universal Serial Interface) peripheral as a shift register.

## Physical Layer

The USI hardware on the ATtiny handles the actual bit shifting. `USICR` configures it in two-wire mode with external clock, and `USIDR` holds the data register. The `_usitc()` call toggles SCL via the USI control register, which also clocks the shift register. SDA is manipulated directly as a GPIO.

Start and stop conditions are generated manually by controlling SDA/SCL timing: start = SDA goes low while SCL is high; stop = SDA goes high while SCL is high.

## Command Queue

Rather than a blocking loop, the driver is **non-blocking and cooperative**. All I2C operations are described as a compile-time byte sequence of commands stored in flash (program memory via `Pgm`). The command opcodes are:

| Opcode | Meaning |
|---|---|
| `C_start` | Generate I2C start condition |
| `C_stop` | Generate I2C stop condition |
| `C_send_bytes_inline` | Send N bytes embedded in the command stream |
| `C_send_bytes` | Send N bytes from a pointer |
| `C_send_bytes_rep` | Repeat a single byte N times (for clearing display RAM) |
| `C_send_bytes_rep_len` | Same but length comes from a RAM struct at runtime |
| `C_send_bytes_ram_len` | Send N bytes from a RAM pointer (for dynamic data) |
| `C_callback` | Call a function mid-sequence without stopping |
| `C_end` | End of sequence, call finalizer |

`_next_cmd()` walks this byte stream, dispatching each opcode. The `resume` function pointer holds where to continue next time `step()` is called from the main task loop.

## Byte Transmission

`_send_byte<get_dr>()` loads `USIDR` with the next byte, then `_send_bits_loop()` clocks it out 8 times using `_usitc()`. After 8 bits (`USISR & USIOIF`), it switches SDA to input to receive the ACK/NACK from the device. If NACK, it aborts and calls `_stop()`; if ACK and more bytes remain, it recurses; otherwise advances to `_next_cmd()`.

## Do-While Loop Construct

`do_while_end<condition, finish>(code)` is a compile-time helper that builds a command sequence with a tail callback that re-jumps to the start of the sequence if `condition()` returns true. This is how `Display::time()` iterates over 5 digit positions — the loop state is kept in `Disp_time::state` in RAM, and the condition lambda increments `state.pos` each pass.

## Command Construction with `mk_cmds`

`mk_cmds(...)` uses variadic templates and `Packed_tuple` to concatenate heterogeneous command structs into a single tightly-packed byte array at compile time, which then gets placed in flash via `Pgm(...)`. The result is zero runtime overhead for building the command sequences — they're fully laid out at compile time.

## Summary

A non-blocking I2C master where the entire operation (init OLED, clear, draw digits, loop) is described as a flash-resident command program, executed one step per main-loop tick, with a function pointer as the "program counter". Very tight for an 8-bit MCU with 512 bytes of RAM.
