# Explorer Bop It! game

A "Bop It!" reaction game for the Curiosity Nano Explorer. The OLED shows an instruction in large
text, such as **TWIST / IT!**. You have to do that action before the time runs out. The time
bar on the OLED and the white segments on the RGB ring show how much time is left.

- **Correct action:** the ring flashes a rainbow and the next round starts. The time limit gets
  shorter as your score goes up.
- **Wrong action** (a different input fires) **or timeout:** the game over screen shows the
  reason, your score and your best score, and the ring flashes red. After 1.5 s, any input starts
  a new game.
- **On boot:** a start screen ("BOP IT! / Touch or press to start").

> **Status: written, NOT built, compiled or tested.** No build or hardware run was possible. Expect
> some compile fixes and threshold tuning on real hardware (see "Assumptions" and "Compile risks").

## Instructions and how they are detected

| OLED | Hardware | Detector (baseline captured at round start) |
|---|---|---|
| COVER IT! | VCNL4200 ambient light @0x51 | light **drops** by at least 50 % and at least 20 counts, 2 samples in a row |
| TOUCH IT! | capacitive pads SW1..SW3 (MCP23008 @0x24, GP5..GP7) | new press on any pad (pads already held at round start must be released first) |
| TWIRL IT! | joystick (MCP23008 GP0..GP3) | up, down, left **and** right each pressed at least once in the round |
| TWIST IT! | potentiometer, ADC AIN7 | change of at least 400 / 4095 in either direction |
| MOVE IT! | VCNL4200 proximity | proximity count **rises** by at least 30 counts or 50 % |
| WARM IT! | MCP9808 @0x1C | temperature **rises** by at least 0.375 °C (+3 s extra time) |
| SAY IT! | microphone, ADC AIN6 (branch `microphone`) | peak-to-peak level of a 64-sample window rises by at least 200 counts or 100 % |

All thresholds and timings are in `src/game_config.h`.

## Code layout

```
CMakeLists.txt  prj.conf  README.md
boards/samd21_cnano.overlay  boards/samd21_cnano.conf
boards/same51_cnano.overlay  boards/same51_cnano.conf
src/main.c          init + main loop (20 ms poll)
src/game.c/.h       state machine: START -> ROUND -> (SUCCESS -> ROUND) | GAME_OVER
src/game_config.h   all tunables
src/ui.c/.h         OLED screens (CFB, picks the largest font that fits)
src/ring.c/.h       WS2812 animations (rainbow flash, countdown, fail, idle)
src/audio.c/.h      audio extension point (no-op stubs)
src/hw/             one owner module per chip: ioexp.c (MCP23008), vcnl4200.c, analog.c (ADC)
src/inputs/         input.h (common interface), detector.c (baseline/threshold helper),
                    inputs.c (registry), input_<name>.c (one per instruction)
```

### Adding an input
1. Write `src/inputs/input_foo.c` that defines a `const struct bopit_input input_foo` with
   `verb`, `init`, `arm` (capture the baseline), `poll` (returns `INPUT_IDLE`, `INPUT_ACTIVITY` or
   `INPUT_COMPLETE`), plus optional `extra_time_ms`, `ignore_mask` and `can_fail`.
2. Add `INPUT_FOO` to `enum bopit_input_id` and the `extern` in `input.h`.
3. Add it to the registry in `inputs.c` and to `CMakeLists.txt`.

`INPUT_ACTIVITY` is for multi-step actions. For example, one joystick direction counts as "any
input" on the start screen and as a wrong action in other rounds, but it does not complete
"Twirl it!".

Inputs that fail `init()` (for example, a sensor that doesn't respond) are left out of the random
selection. The start screen then says "Some sensors off", and the console prints which ones.

## Supported boards and build

| Board | `-b` | PyOCD pack |
|---|---|---|
| SAM D21 Curiosity Nano | `samd21_cnano` | atsamd21g17d |
| SAM E51 Curiosity Nano | `same51_cnano` | atsame51j20a |

Use the repo's Zephyr fork (`capstone/demo`, see the top-level repo README). It contains the board
files and the SAM support for `ws2812-gpio`. Then:

```
west build -p always -b samd21_cnano path/to/bopit_game
west flash
```

## Merged overlays: per-board resources

| Function | SAM D21 | SAM E51 | Source example |
|---|---|---|---|
| Explorer I2C (OLED, MCP23008, VCNL4200, MCP9808) | SERCOM4 PA12/PA13 | SERCOM2 PA12/PA13 | ambient_light, joystick_oled, proximity_sensor, temperature_sensor, capacitive_touch |
| Potentiometer | `&adc` AIN7 (PA07), REF_INTERNAL | `&adc0` AIN7, REF_INTERNAL | potentiometer_reading |
| Microphone | `&adc` AIN6 (PA06), REF_VDD_1_2 | `&adc0` AIN6, REF_VDD_1 | microphone (branch) |
| RGB ring (8x WS2812B) | PA28, ws2812-gpio | PB04, ws2812-gpio | rgb_ring |
| Speaker DAC (commented out) | SERCOM1 SPI PA16/17/19, CS PA27, EN PB02 | SERCOM1 SPI, CS PB05, LDAC PA03, EN PB14 | speaker |
| Console | SERCOM5 | SERCOM5 | board default |

## Conflicts and how they were resolved

1. **MCP23008 @ 0x24 had two owners.** `capacitive_touch` binds Zephyr's `microchip,mcp23008`
   GPIO driver plus `gpio-keys`. `joystick_oled` and `snake_game` read the same chip raw over I2C.
   The overlay node `ioexp2` has **no compatible**, and `src/hw/ioexp.c` reads all 8 bits for
   both the joystick and the touch pads. `CONFIG_INPUT` and `CONFIG_INPUT_GPIO_KEYS` were dropped.
2. **VCNL4200 is shared** by "Cover" (ambient light) and "Move" (proximity). Covering the sensor
   also raises the proximity count. Each of the two instructions ignores the other
   (`ignore_mask`).
3. **One ADC, two references.** The pot uses `ADC_REF_INTERNAL` and the mic uses `VDD_1_2` /
   `VDD_1`. Zephyr's `adc_sam0` driver writes the global `REFCTRL`/`INPUTCTRL` in
   `adc_channel_setup()` and converts one channel per sequence. `analog.c` therefore re-runs the
   setup whenever it switches channel, so each input keeps its example's exact configuration.
   *Assumption:* the SAME51 board uses a driver with the same per-call setup behaviour. If it uses
   the Microchip g1 ADC driver, check this.
4. **OLED compatible.** The examples use both `solomon,ssd1306b` (majority) and `solomon,ssd1306`.
   The game uses `ssd1306b`. If your tree rejects it, switch to the other one.
5. **joystick_oled restated SERCOM4/SERCOM2** (pinctrl with `input-enable`, clocks). The game
   relies on the board defaults, like the other 6 examples. If I2C reads fail on SAME51, copy the
   `input-enable` pinctrl block from `joystick_oled`.
6. **Heap:** the examples used 2048 to 8192 bytes. The game uses 4096 (the 1 KiB CFB frame buffer
   plus margin) in `prj.conf` and 8192 on SAME51 via `same51_cnano.conf`.
7. **MCP9808 sign bit:** the `temperature_sensor` example tests bit 5. According to the
   datasheet, the sign is bit 4, which `input_warm.c` uses.
8. **Speaker vs. everything else:** no pin overlap. The SPI on SERCOM1 is free, and the DAC CS,
   LDAC and enable pins are separate from the I2C, ADC and ring pins.

## Assumptions (where the repo is ambiguous)

- **Touch pads:** the "capacitive touch" pads are SW1..SW3 on MCP23008 GP7..GP5, as in both the
  capacitive_touch and joystick_oled examples.
- **"Wrong action" policy** (all of it is in each input's descriptor):
  - **Move it!** (`can_fail = false`): hands reaching for the board pass the proximity sensor.
  - **Say it!** (`can_fail = false`): the mic picks up joystick clicks and board taps.
  - **Warm it!** ignores Cover and Move.
  - Change these if you want a stricter game.
- **Warm it!** only detects a rise, so a sensor that is cooling down after a previous round never
  triggers. A thumb heats it slowly, so it gets 3 s of extra time.
- **Cover it!** needs some room light. In the dark, the baseline is too low for a 20-count drop.
- **Round timing:** each round lasts 4.0 s minus 150 ms per point, with a minimum of 1.2 s, plus
  the extra time per input. There is a 250 ms settle time after each baseline.
- **No repeats:** the same instruction is not shown twice in a row (unless only one input works).

## Compile risks (not verified)

- **CFB fonts:** `cfb_get_numof_fonts` and `cfb_framebuffer_set_font` switch fonts between prints
  on one frame. This assumes the default 10x16, 15x24 and 20x32 fonts are present.
- **I2C nodes without a compatible** (`ioexp2`, `vcnl4200`): the same pattern as the
  `proximity_sensor` example, used with `I2C_DT_SPEC_GET`.
- **ADC names:** `ADC_DT_SPEC_GET_BY_NAME` needs `io-channel-names` (present in both overlays).

## Audio extension (not implemented)

`src/audio.h` defines `audio_init`, `audio_start_loop`, `audio_stop_loop`,
`audio_play_instruction(id)`, `audio_play_success`, `audio_play_start` and `audio_play_end`. The
game state machine already calls them, and `src/audio.c` contains no-ops. To implement:

1. Copy `dts/bindings/microchip,mcp4821.yaml` and `src/dac_mcp4821.c` from the `speaker` example.
   Add the driver to `CMakeLists.txt`, uncomment the speaker block in the board overlay, and set
   `CONFIG_SPI=y` and `CONFIG_DAC=y`.
2. Run playback in its own thread. A `k_timer` or TC interrupt at the sample rate pulls one mixed
   sample from a small ring buffer and writes it to the MCP4821. The game loop must never block on
   audio, and the API is non-blocking by contract.

### Feasibility study

**Hardware** (Explorer user guide DS50003716, schematics, and the repo's `speaker` example):

- There is **no buzzer**. Sound comes from an **MCP4821 12-bit SPI DAC**, then a **class-D mono
  amplifier**, then an **8 Ω / 0.2 W speaker**.
- Volume is set with a potentiometer next to the amplifier. That is a separate pot from the game's
  AIN7 pot (assumption, per the user guide). J202 can remap the amplifier input.
- The amplifier has an enable pin (IO 12: PB02 on SAMD21, PB14 on SAME51).
- The microphone is a MEMS mic with an MCP6022 op-amp: gain 1, 221 or 1001 via switch S200, on an
  ADC pin.

**Zephyr support:**

- There is no upstream MCP4821 driver; the repo has an out-of-tree one (`dac_mcp4821.c`).
- The upstream DAC API is `dac_write_value()`: one synchronous sample per call, no streaming, no
  DMA.
- The internal DACs (`atmel,sam0-dac` on SAMD21, 10-bit; `microchip,dac-g1` on SAME51, merged
  around v4.4) have the same single-value API. They would also need J202 rewiring, so the MCP4821
  path is the natural one.

**Is a percussion loop + per-instruction sounds + start/end jingles possible?** Yes, with
limits:

- Each sample is a 16-bit SPI frame. At 1 MHz SPI (the speaker example's setting) that is about
  16 µs of bus time per sample. With Zephyr's SPI overhead, an 8 kHz rate is realistic and 11 to
  16 kHz is possible with a higher SPI clock (MCP4821 max 20 MHz).
- Calling `spi_write` from a timer ISR is not allowed. Use either a high-priority thread woken
  every N samples to fill a buffer and push it, or a DMA + TC trigger driving SERCOM SPI directly
  (bare-metal, not through the Zephyr API).
- **SAMD21 (48 MHz M0+, no FPU):** an 8 kHz software mixer of 2 to 3 voices, with integer or
  wavetable synthesis, is feasible but costs noticeable CPU.
- **SAME51 (120 MHz M4F):** comfortable for 4 or more voices.

**Mixing and polyphony:**

- Software mixing is required, because there is one mono DAC channel.
- Plan for **one loop voice plus one or two effect voices** (instruction or jingle). Sum them with
  saturation or simple ducking of the loop.
- The WS2812 bit-bang locks interrupts for about 240 µs per ring update. At 8 kHz (125 µs per
  sample) this causes an audible glitch or jitter, so the ring update rate should drop during
  audio, or audio needs a buffered or DMA path.
- The blocking ~1 KiB OLED I2C transfers are fine if audio runs in a higher-priority thread.

**Microphone vs. speaker:**

- Electrically, they don't conflict (different pins: ADC vs. SPI).
- Acoustically they do: the speaker is close to the mic, so the backing loop would trigger
  "Say it!" or raise its baseline.
- Mitigations:
  - mute or duck the loop while "Say it!" is active (the `audio_play_instruction(INPUT_SAY)` hook
    can do this);
  - capture the mic baseline while the loop is playing, and raise the threshold;
  - mic and DAC timing also compete for CPU on the SAMD21.
- The game's ADC reads are polled, so audio must have the higher thread priority.

**Memory: samples vs. synthesized tones:**

- At 8 kHz with 8-bit PCM, audio costs 8 KB per second.
- **SAMD21G17D (128 KiB flash, 16 KiB RAM):**
  - the game itself probably takes about 30 to 50 KiB of flash (estimate);
  - that leaves room for roughly 6 to 8 s of 8-bit audio in total;
  - realistic budget: short drum hits (kick, snare, hat, ~0.1 to 0.2 s each, ~4 KB total)
    sequenced into a loop, and synthesized jingles (square or triangle tone sequences, a few bytes
    each);
  - 7 spoken voice lines (~0.6 s each ≈ 34 KB) would just fit at 8 kHz/8-bit or 4-bit ADPCM, but
    are tight.
- **SAME51J20A (1 MiB flash, 256 KiB RAM):** sampled voice lines, a sampled loop and jingles all
  fit easily, even at 16 kHz.
- **RAM:** only a few hundred bytes of mix buffer are needed (double buffer of 2 × 64 to
  256 samples).

**Recommendation:**

- Synthesize the percussion loop (noise burst + decaying sine/square) and the jingles on both
  boards.
- Use short PCM or ADPCM voice lines on SAME51, and tone motifs per instruction on SAMD21.
