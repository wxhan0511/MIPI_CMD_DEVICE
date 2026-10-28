# Resistance / diode reference-device compatibility

## Temporary fixed-10M diagnostic (currently disabled)

ADC initialization currently uses DRATE=0x63 (50 SPS), matching the reference
device, for comparison against the previous 100-SPS capture. This is a global
rate setting and also reduces normal voltage/current scan throughput. Expect
approximately 20-ms sample spacing and 500 samples per 10-second capture.

`RD_FIXED_10M_DIAGNOSTIC` in `Bsp/bsp_ads1256_ctl.c` is currently 0.
Normal resistance/diode commands are restored; 50 SPS is retained. Set it to 1
only when running the diagnostic described below.
One GET_RESISTANCE selects the supplied pins once, holds the 10-Mohm hardware
range continuously for 10000 ms and emits `RD10M,t_ms,seq,delta,raw_V` UART rows.
No polynomial, offset subtraction, ranging or averaging is performed. Startup
samples are intentionally included. The timestamp is task observation time;
delta > 1 flags samples skipped by the task (including UART output overhead).
The END line reports count, missed sequence updates and raw min/max/span.
Cleanup disconnects the path after the capture. Diode acquisition is unchanged.

The normal resistance API does not return a resistance in this mode: the result
is NaN with FAILED status. The host may time out before the capture finishes.
Issue only one request, capture the board UART until RD10M END, and do not retry
or send other commands during capture. Set the macro to 0 and rebuild to restore
normal resistance operation. This diagnostic is deliberately not production mode.

Reference source: `D:/Project/ae_tool_meter_v2` (command_20.c,
task_sample.c, bsp_ads1256.c and bsp_adc_convert.c).

The R/D commands now acquire through `bsp_rd_measure()`:

- Only CH2 / external mux 0 is used. Samples are identified by CH2 sequence.
- In single-channel acquisition the ADC is left converting continuously:
  MUX and SYNC/WAKEUP are issued only when the ADC channel actually changes.
  This removes per-result filter restarts; hardware validation is still required
  to establish whether it resolves the observed alternating high/low voltages.
- Rate-limited `RD sample` logs report task observation time, sequence delta,
  range and raw volts. `RD range` reports min/max of all consumed samples after
  the last range's settling interval. No averaging is applied to the result.
- Initial resistance range is the legacy 91-ohm calculation range; diode
  uses the 4.7-kohm hardware path.
- Initial connection and each resistance range change have a 120-ms settling
  interval. Frames published during settling are discarded.
- Resistance conversion uses the reference source's 0.5-V **calculation
  coordinate**, 1.660-mV subtraction and reference EEPROM polynomial coefficients.
  The 0.5-V coordinate is not a claim that physical excitation is 0.5 V.
- Automatic ranging is enabled only when `dev_vol.res_cali_en` is enabled,
  matching the reference firmware. Disabling calibration leaves the initial
  resistance range fixed; a different fixed range requires a code change.
- Resistance runs for at least 1000 ms; diode for at least 500 ms. Readout uses
  a fresh, valid sample after range settling, with a 2500-ms total acquisition
  limit. Success does not assert steady-state convergence: the reference uses
  timed readout and non-ohmic DUTs may remain time-dependent. No averaging or
  consecutive-sample stability threshold is imposed.
- Invalid pins produce FAILED; missing/invalid samples or unresolved ranging produce TIMEOUT
  and a NaN payload, rather than a successful stale resistance.
- Firmware response units remain ohms for resistance and volts for diode, to
  preserve the new device's API. The reference firmware internally returns mV
  for diode; compare values only after normalizing units.
- All R/D pin groups and the reference selector are disabled after acquisition;
  the previous ADC channel-enable mask is restored.

## Limits and validation

The six resistance ranges now use the actual reference EEPROM coefficients
captured on 2026-10-26 (UID 32303836 30345105 003A004C, marker 0x8F,
resistance_read_status=0), replacing the source defaults. Literals retain double
precision. Reference range 7 and diode coefficients were NaN and are not imported;
range 7 is excluded from resistance conversion and diode returns raw volts.
Matching coefficients is not proof of equal hardware readings. Analog switches, amplifier bias/leakage,
reference resistors and excitation also differ between hardware revisions.

Do not mix these polynomials with a 0.269-V denominator without recalibration.
Test known resistors in each range, repeated same-pair acquisitions, open and
short inputs, diode forward/reverse, and the same unpowered DUT on both devices.
Keep DUT initial charge state, polarity and scan order identical. Check the host
SPI transaction timeout permits the 2500-ms worst case. No hardware measurements
or deployment are performed by building this firmware.
