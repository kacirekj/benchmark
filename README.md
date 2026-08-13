# Benchmarks

This is C language benchmark for disk read/write operations. It contains also the results for various platforms.

## Hdd read

### Arduino Uno

#### FAT Filesystem

```
Configuration:
Rows: 10000
Columns: 32
Target: abcdefgh
Read buffer size: 512 bytes
Runs: 3
Total file size: 320000 bytes
SD SPI speed: 8000000 Hz

...
Average results (3 runs):
CREATE: 2034.175 ms, 0.157 MB/s
READ: 1016.065 ms, 0.315 MB/s
STARTS WITH: 1030.581 ms, 0.311 MB/s
CONTAINS: 1325.560 ms, 0.241 MB/s
DIRECT ACCESS: 3.365 ms, 0.010 MB/s
```

#### No Filesystem

```
Configuration:
Rows: 10000
Columns: 32
Target: abcdefgh
Read buffer size: 512 bytes
Rows per block: 16
Blocks: 625
Runs: 3
Total database size: 320000 bytes
Start block: 1
SD SPI speed: 8000000 Hz

...
Average results (3 runs):
CREATE: 1397.492 ms, 0.229 MB/s
READ: 995.747 ms, 0.321 MB/s
STARTS WITH: 1016.712 ms, 0.315 MB/s
CONTAINS: 1386.895 ms, 0.231 MB/s
DIRECT ACCESS: 1.569 ms, 0.326 MB/s
```

### Dell Latitude E5470 @ Debian 13

```
Configuration:
Rows: 10000
Columns: 32
Target: abcdefgh
Read buffer size: 512 bytes
Runs: 5
Total file size: 320000 bytes

Average results (5 runs):
CREATE             40.082 ms       7.984 MB/s
READ                4.300 ms      74.410 MB/s
STARTS WITH         6.045 ms      52.935 MB/s
CONTAINS            8.500 ms      37.648 MB/s
DIRECT ACCESS       0.565 ms       0.057 MB/s
```

### Mac Air M1 2020

```
Configuration:
Rows: 10000
Columns: 32
Target: abcdefgh
Read buffer size: 512 bytes
Runs: 5
Total file size: 320000 bytes

Average results (5 runs):
CREATE             24.244 ms      13.199 MB/s
READ                1.209 ms     264.769 MB/s
STARTS WITH         1.423 ms     224.814 MB/s
CONTAINS            3.346 ms      95.642 MB/s
DIRECT ACCESS       0.249 ms       0.129 MB/s
```

## Coremark

| Device | Score |
|---|---:|
| Macbook Air M1 2020 | 31600 |
| Teensy 4.0 | 2313.57 |
| Adafruit Metro M4 (200MHz overclock, 'dragons' optimization) | 536.35 |
| Adafruit Metro M4 (180MHz overclock, faster optimizations) | 458.19 |
| Teensy 3.6 | 440.72 |
| Sparkfun ESP32 Thing | 351.33 |
| Adafruit HUZZAH 32 | 351.35 |
| Teensy 3.5 | 265.50 |
| Teensy 3.2 (96MHz overclock, faster optimizations) | 218.26 |
| Adafruit Metro M4 (120MHz, smaller code) | 214.85 |
| Teensy 3.2 (72MHz) | 168.62 |
| Teensy 3.2 (72MHz, smaller code) | 126.76 |
| Arduino Due | 94.95 |
| Arduino Zero | 56.86 |
| Arduino Nano Every | 8.20 |
| Arduino Mega | 7.03 |
