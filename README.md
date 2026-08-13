# Benchmarks

This is C language benchmark for disk read/write operations. It contains also the results for various platforms.

## My hdd read benchmark

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

## Coremark single core

I confirm that Apple M1 speed is same on my Mac

| Board | CoreMark | Mark/MHz |
|---|---:|---:|
| Arduino MEGA 2560 | 7 | 0.44 |
| STM32F103C8T6 128k | 81 | 1.13 |
| STM32F401CCU6 256k | 150 | 1.79 |
| STM32F411CEU6 512k | 172 | 1.72 |
| T-Koala ESP32 | 351 | 2.19 |
| Raspberry Pi Model B v2 | 1574 | 2.25 |
| Qualcomm Atheros QCA956X | 2053 | 2.65 |
| Raspberry Pi 3 Model B | 3800 | 3.17 |
| Amlogic S905W tanix tx3 | 3913 | 3.26 |
| Raspberry Pi 4 v1.1 4GB | 8257 | 5.50 |
| Xeon X5550 | 13643 | 4.46 |
| i5-3320M | 21245 | 6.44 |
| i7-4960HQ | 21326 | 5.61 |
| i7-6820HQ | 23779 | 6.61 |
| i3-10100 | 30532 | 7.23 |
| i7-13700T | 39082 | 8.49 |
| Apple M1 | 31718 | 9.91 |
| Exynos 2400 in Galaxy S24 | 33129 | 10.32 |
