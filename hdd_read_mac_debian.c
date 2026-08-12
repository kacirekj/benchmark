#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DB_FILE "DB.RAW"

#define ROW_COUNT 10000U
#define COL_COUNT 32U
#define TARGET "abcdefgh"
#define READ_BUFFER_SIZE 512U
#define RUN_COUNT 5U

struct db_row {
    char col[COL_COUNT];
};

struct measurement_result {
    double create_file_total_ms;
    double read_file_total_ms;
    double starts_with_total_ms;
    double contains_total_ms;
    double direct_access_total_ms;
};

static char read_buffer[READ_BUFFER_SIZE];
static volatile uint32_t benchmark_sink;


// UTILS

static uint64_t get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static double elapsed_ms(uint64_t start_time)
{
    return (double)(get_time_ns() - start_time) / 1000000.0;
}

static void drop_caches(void)
{
#if defined(__APPLE__)
    system("purge");
#elif defined(__linux__)
    system("sync && echo 1 > /proc/sys/vm/drop_caches");
#else
#error Unsupported operating system
#endif
}

static void print_configuration(void)
{
    printf("Configuration:\n");
    printf("Rows: %u\n", ROW_COUNT);
    printf("Columns: %u\n", COL_COUNT);
    printf("Target: %s\n", TARGET);
    printf("Read buffer size: %u bytes\n", READ_BUFFER_SIZE);
    printf("Runs: %u\n", RUN_COUNT);
    printf("Total file size: %u bytes\n", ROW_COUNT * COL_COUNT);
    printf("\n");
}

static void print_average_result(const char *name, double total_ms, uint32_t bytes)
{
    double average_ms = total_ms / RUN_COUNT;
    double throughput = ((double)bytes / 1000000.0) / (average_ms / 1000.0);

    printf("%-14s %10.3f ms  %10.3f MB/s\n", name, average_ms, throughput);
}


// SERVICE

static double create_database(void)
{
    struct db_row db_row;

    memset(db_row.col, '0', COL_COUNT - 1);
    db_row.col[COL_COUNT - 1] = '\n';

    int fd = open(DB_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    uint64_t start_time = get_time_ns();

    for (uint16_t row_cnt = 0; row_cnt < ROW_COUNT; row_cnt++) {
        if (row_cnt == ROW_COUNT - 1) {
            memcpy(db_row.col, TARGET, sizeof(TARGET) - 1);
        }

        write(fd, db_row.col, COL_COUNT);
    }

    fsync(fd);
    close(fd);

    return elapsed_ms(start_time);
}

static double read_database_only(void)
{
    int fd = open(DB_FILE, O_RDONLY);
    uint64_t start_time = get_time_ns();

    while (read(fd, read_buffer, READ_BUFFER_SIZE) > 0) {
    }

    double result = elapsed_ms(start_time);

    close(fd);

    return result;
}

static double search_database_target_at_start(void)
{
    uint16_t row_cnt = 0;
    uint32_t found_offset = UINT32_MAX;

    int fd = open(DB_FILE, O_RDONLY);
    uint64_t start_time = get_time_ns();

    ssize_t bytes_read;

    while ((bytes_read = read(fd, read_buffer, READ_BUFFER_SIZE)) > 0) {
        uint8_t rows_in_buffer = (uint8_t)(bytes_read / COL_COUNT);

        for (uint8_t buffer_row_cnt = 0; buffer_row_cnt < rows_in_buffer; buffer_row_cnt++) {
            uint16_t row_offset = (uint16_t)buffer_row_cnt * COL_COUNT;
            bool target_match = true;

            for (uint8_t target_cnt = 0; target_cnt < sizeof(TARGET) - 1; target_cnt++) {
                if (read_buffer[row_offset + target_cnt] != TARGET[target_cnt]) {
                    target_match = false;
                    break;
                }
            }

            if (target_match) {
                found_offset = (uint32_t)row_cnt * COL_COUNT;
                goto done;
            }

            row_cnt++;
        }
    }

done:
    ;

    double result = elapsed_ms(start_time);

    close(fd);

    benchmark_sink ^= found_offset;

    return result;
}

static double search_database_target_anywhere(void)
{
    uint16_t row_cnt = 0;
    uint32_t found_offset = UINT32_MAX;

    int fd = open(DB_FILE, O_RDONLY);
    uint64_t start_time = get_time_ns();

    ssize_t bytes_read;

    while ((bytes_read = read(fd, read_buffer, READ_BUFFER_SIZE)) > 0) {
        uint8_t rows_in_buffer = (uint8_t)(bytes_read / COL_COUNT);

        for (uint8_t buffer_row_cnt = 0; buffer_row_cnt < rows_in_buffer; buffer_row_cnt++) {
            uint16_t row_offset = (uint16_t)buffer_row_cnt * COL_COUNT;

            for (uint8_t column_cnt = 0; column_cnt <= COL_COUNT - (sizeof(TARGET) - 1); column_cnt++) {
                bool target_match = true;

                for (uint8_t target_cnt = 0; target_cnt < sizeof(TARGET) - 1; target_cnt++) {
                    if (read_buffer[row_offset + column_cnt + target_cnt] != TARGET[target_cnt]) {
                        target_match = false;
                        break;
                    }
                }

                if (target_match) {
                    found_offset = (uint32_t)row_cnt * COL_COUNT;
                    goto done;
                }
            }

            row_cnt++;
        }
    }

done:
    ;

    double result = elapsed_ms(start_time);

    close(fd);

    benchmark_sink ^= found_offset;

    return result;
}

static double search_database_direct_target(void)
{
    uint32_t struct_offset = (ROW_COUNT - 1U) * COL_COUNT;

    int fd = open(DB_FILE, O_RDONLY);
    uint64_t start_time = get_time_ns();

    lseek(fd, (off_t)struct_offset, SEEK_SET);
    read(fd, read_buffer, COL_COUNT);

    bool target_match = true;

    for (uint8_t target_cnt = 0; target_cnt < sizeof(TARGET) - 1; target_cnt++) {
        if (read_buffer[target_cnt] != TARGET[target_cnt]) {
            target_match = false;
            break;
        }
    }

    double result = elapsed_ms(start_time);

    close(fd);

    benchmark_sink ^= target_match ? struct_offset : UINT32_MAX;

    return result;
}


// MAIN

int main(void)
{
    struct measurement_result result = {0};

    print_configuration();

    for (uint8_t run = 0; run < RUN_COUNT; run++) {
        result.create_file_total_ms += create_database();

        drop_caches();
        result.read_file_total_ms += read_database_only();

        drop_caches();
        result.starts_with_total_ms += search_database_target_at_start();

        drop_caches();
        result.contains_total_ms += search_database_target_anywhere();

        drop_caches();
        result.direct_access_total_ms += search_database_direct_target();
    }

    printf("Average results (%u runs):\n", RUN_COUNT);

    print_average_result("CREATE", result.create_file_total_ms, ROW_COUNT * COL_COUNT);
    print_average_result("READ", result.read_file_total_ms, ROW_COUNT * COL_COUNT);
    print_average_result("STARTS WITH", result.starts_with_total_ms, ROW_COUNT * COL_COUNT);
    print_average_result("CONTAINS", result.contains_total_ms, ROW_COUNT * COL_COUNT);
    print_average_result("DIRECT ACCESS", result.direct_access_total_ms, COL_COUNT);

    return EXIT_SUCCESS;
}
