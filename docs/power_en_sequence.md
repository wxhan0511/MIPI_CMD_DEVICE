# Eight-rail EN sequencing (A0 2B)

## Lua interface

```lua
-- time_us, VCC, IOVCC, VSP, VSN, AVDD, VDD, ELVDD, ELVSS
local status = power.set_en_sequence({
    {0,   1,1,0,0,0,0,0,0},
    {50,  1,1,1,0,0,0,0,0},
    {100, 1,1,1,1,0,0,0,0},
})
assert(status == 0, 'EN sequence failed')
```

Each row sets all eight enables on one latch edge. Accepts 1..8 rows,
first timestamp 0, integer microseconds, increasing by at least 50 us,
last timestamp <=1,000,000 us. Final state is retained. The call blocks until
completion. Time is relative to the MCU sequence, not the Linux call or SPI
arrival. Configure voltage/current limits separately.

50 us is the minimum programmable interval; 1 us is timestamp resolution.
Neither is a measured edge-error guarantee. TIM5 runs at 1 MHz and schedules
absolute compare deadlines. The first compare has a 100 us startup guard;
actual CP edges follow compare handling plus data setup. Interrupt lateness
and driver execution variation must be measured on hardware.

## Firmware implementation

- `Bsp/bsp_power_en_sequence.c/.h`: validation, logical/physical mapping,
  blocking execution and `TIM5_IRQHandler`. TIM5 is configured on demand,
  priority 1, with no RTOS calls in the ISR.
- `bsp_power_en_sequence_execute()`: validates before changing EN, reserves
  the shared latch bus for the whole sequence, waits for completion, returns
  final state. Bus acquisition timeout: 100 ms; execution watchdog: 2 s.
- `bsp_d_trigger_write_byte_owned()`: two BSRR data writes, 1 us setup,
  1 us CP high, 1 us CP low; updates the existing shadow state.
- `bsp_d_trigger_write_byte()`: mutex-protected immediate full-byte write.
  `ALL_POWER_EN` now uses this operation.
- ISR masks interrupts only across the short latch transaction and scheduling
  update. Other latch users wait on the existing mutex for up to the sequence
  duration. Do not use the owned interface without bus ownership.
- VSN/ELVSS status bit selection now matches the existing enable macros.

The current conservative late-interrupt abort threshold is
`SEQ_MAX_LATENESS_US = 5` in the implementation. This is a software rejection
threshold, **not a guaranteed +/-5 us hardware accuracy**. An ISR more than
5 timer ticks late aborts remaining events and retains the last applied state.
Tune only after measuring target-board timing under load. Already applied
events are not rolled back.

## SPI protocol (64 bytes, little endian)

Request: `A0 2B 01 count`, then `count` records of `uint32 at_us, uint8 mask`,
remaining bytes zero. Mask bits 0..7 follow the Lua channel order. Physical
latch mapping is [7,6,5,1,4,3,2,0], negative rails inverted.

Response:

| Offset | Meaning |
|---|---|
| 0..1 | A0 2B |
| 2 | 0 success, 1 failure |
| 3 | Detailed result below |
| 4 | Executed event count |
| 5 | Final logical mask |
| 6 | Final state valid (0/1) |
| 7..10 | Maximum observed ISR-entry lateness, uint32 us |

Detailed results: 0 OK, 1 bad format, 2 bad count, 3 bad time,
4 bad interval, 5 resource busy/unavailable, 6 deadline missed,
7 execution watchdog timeout. No physical EN feedback is available: final
state means the last software-latched output, not a measured voltage.

## Service integration

The implementation lives in `hal/hal_power.py`, with the command header in
`PROTOCOL_HEADERS["SET_POWER_EN_SEQUENCE"]`. Parameter conversion, encoding,
response handling and cache updates are methods of `HalPowerControl`.
`tools/host/gcv5_service_en_sequence.patch` records the service changes.
The Lua proxy exposes HAL methods automatically.
`core/dispatchers/power.py` registers `power.set_en_sequence`
with an `events` parameter. `power.last_en_sequence` holds detailed results.
On an uncertain transfer, cache entries become `None`; `get_en()` reports FAIL
for unknown entries. A known partial final state updates the cache normally.

Firmware build: `cmake -S . -B build`, then `cmake --build build -j 4`.
Host tests: run `test_power_en_sequence.py` with the service virtual environment
and the service root on `PYTHONPATH`. Tests import the actual power HAL and
use fake SPI; Lupa is required for Lua tests. Build artifacts are `build/build.bin` and
`build/build.hex` (application at 0x08010000).

After installing firmware and restarting service workers, measure CP2 and EN
edges for 50 us steps under normal sampling/communication load. Verify data
setup and CP pulse width, simultaneous multi-rail changes, abort reports and
negative-rail mapping. All shared-driver latch devices must support the new
1 us timing. The host tests use fake transport and never switch board power.
