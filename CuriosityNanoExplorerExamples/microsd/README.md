# Explorer microSD card demo

Reads the size of the card in the Curiosity Nano Explorer microSD socket, mounts its FAT file
system, appends one line to `LOG.TXT`, prints the file back and lists the root folder on the
serial console. `LOG.TXT` gains one line on every reset, so you can see that the writes reach the
card and survive a power cycle. You can also open the file on a PC.

## Supported boards
| Board | `-b` identifier | Zephyr source | Overlay |
|---|---|---|---|
| SAM D21 Curiosity Nano | `samd21_cnano` | [Vooks916/zephyr `samd21-cnano`](https://github.com/Vooks916/zephyr/tree/samd21-cnano) | `boards/samd21_cnano.overlay` |
| SAM E51 Curiosity Nano | `same51_cnano` | [Vooks916/zephyr `same51-cnano`](https://github.com/Vooks916/zephyr/tree/same51-cnano) | `boards/same51_cnano.overlay` |

`src/main.c`, `prj.conf` and `CMakeLists.txt` are the same for every board. Zephyr picks up the
right overlay automatically from `boards/<board>.overlay`.

## Explorer hardware (user guide DS50003716)
The socket is on the Explorer SPI bus, which it shares with the 25CSM04 SPI EEPROM and the
MCP4821 DAC. Only the SPI signals reach the Nano. Card detect and the D1/D2 data lines go only
to header J212, so the app cannot tell whether a card is inserted until it tries to talk to it.

| Signal | Explorer pin | SAM D21 | SAM E51 |
|---|---|---|---|
| MOSI (card CMD) | SPI MOSI | PA16 (SERCOM1 PAD0) | PA16 (SERCOM1 PAD0) |
| SCK (card CLK) | SPI SCK | PA17 (SERCOM1 PAD1) | PA17 (SERCOM1 PAD1) |
| MISO (card D0) | SPI MISO | PA19 (SERCOM1 PAD3) | PA19 (SERCOM1 PAD3) |
| Chip select (card D3) | IO 25 | PB23 | PB06 |

## How it works
Zephyr has a complete SD card stack, so no driver code is needed. It is built from three
layers, each set up in devicetree or `prj.conf`:

| Layer | Set up by | What it does |
|---|---|---|
| SPI bus | Board devicetree (`&sercom1`) | Moves bytes |
| `zephyr,sdhc-spi-slot` | Overlay, child of `&sercom1` | Speaks the SD card protocol over SPI (`CONFIG_SPI_SDHC`, enabled automatically) |
| `zephyr,sdmmc-disk` | Overlay, child of the slot | Turns the card into a disk named `SD` (`CONFIG_DISK_ACCESS`) |
| FatFs | `CONFIG_FAT_FILESYSTEM_ELM` | FAT file system on disk `SD`, mounted at `/SD:` |

The board devicetree already has one chip select on SERCOM1 (PA18, the SPI EEPROM). The overlay
replaces `cs-gpios` with a list of two, and the card uses the second entry (`reg = <1>`). PA18
stays in the list so the driver keeps the EEPROM deselected.

The card starts at 400 kHz, as the SD specification requires, and then switches to
`spi-max-frequency` (8 MHz).

`main.c` uses three APIs:
- `disk_access_ioctl()` (`zephyr/storage/disk_access.h`): starts the card and reads its
  sector count and sector size.
- `fs_mount()` / `fs_unmount()` (`zephyr/fs/fs.h`): attach the FAT file system to `/SD:`.
- `fs_open()`, `fs_write()`, `fs_read()`, `fs_opendir()`, `fs_readdir()`: normal file access.
  Paths start with the mount point, for example `/SD:/LOG.TXT`.

Long file names are off (`CONFIG_FS_FATFS_LFN`), so names are limited to 8.3 and shown in
upper case.

## Preparing the card
Format the card as **FAT32** (or FAT16) on a PC. Cards over 32 GB come formatted as exFAT,
which this sample does not enable. The app never formats the card: `CONFIG_FS_FATFS_MOUNT_MKFS`
is turned off and the mount uses `FS_MOUNT_FLAG_NO_FORMAT`, so a card with the wrong format is
reported, not wiped.

## Build and flash
Run from the Zephyr checkout on the branch that contains the board:

```
west build -p always -b samd21_cnano path/to/microsd
west build -p always -b same51_cnano path/to/microsd
west flash
```

| Board | Flash | RAM |
|---|---|---|
| SAM D21 | 33 KB of 128 KB | 6.5 KB of 16 KB |
| SAM E51 | 41 KB of 1 MB | 6.9 KB of 256 KB |

## Serial console
115200 8N1 on the Nano Debugger virtual COM port. Output after the second reset looks like this (sizes and times depend on the card):

```
Explorer microSD demo
Card: 15523840 sectors of 512 bytes, 7580 MB
Mounted at /SD:
--- /SD:/LOG.TXT ---
Boot at uptime 412 ms
Boot at uptime 410 ms
--- end ---
Files in /SD:
        44 LOG.TXT
Unmounted (0). Done.
```

## Troubleshooting
- **`No card found`:** the card did not answer. Reseat it and the Nano, and press reset; the app
  only checks once at startup. Some cards need a slower bus: try `spi-max-frequency = <4000000>;`.
- **`Mount failed (error -19)`:** the card answered but has no FAT file system it can read
  (`-ENODEV`). This is usually exFAT. Reformat it as FAT32.
- **Adding exFAT or long names:** add `CONFIG_FS_FATFS_EXFAT=y` (which also enables long file
  names) to `prj.conf`. This costs flash and RAM, which matters most on the D21.
