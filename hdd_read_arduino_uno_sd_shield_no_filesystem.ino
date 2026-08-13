#include <SPI.h>
#include <SD.h>
#include <string.h>

#define SD_CS_PIN 10
#define SD_SPI_SPEED 8000000UL

#define START_BLOCK 1UL

#define ROW_COUNT 10000UL
#define COL_COUNT 32
#define TARGET "abcdefgh"
#define READ_BUFFER_SIZE 512
#define RUN_COUNT 3

#define ROWS_PER_BLOCK (READ_BUFFER_SIZE / COL_COUNT)
#define BLOCK_COUNT (ROW_COUNT / ROWS_PER_BLOCK)
#define DB_SIZE (ROW_COUNT * COL_COUNT)
#define TARGET_OFFSET ((ROW_COUNT - 1) * COL_COUNT)

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

Sd2Card card;
uint8_t read_buffer[READ_BUFFER_SIZE];


// UTILS

double elapsed_ms(uint32_t start_time)
{
    return (double)(micros() - start_time) / 1000.0;
}

void fail(const __FlashStringHelper *message)
{
    Serial.print(F("Benchmark failed: "));
    Serial.println(message);

    while (true) {
    }
}

void prepare_block(void)
{
    memset(read_buffer, '0', READ_BUFFER_SIZE);

    for (uint8_t row_cnt = 0; row_cnt < ROWS_PER_BLOCK; row_cnt++) {
        read_buffer[(row_cnt + 1) * COL_COUNT - 1] = '\n';
    }
}

void print_configuration(void)
{
    Serial.println(F("Configuration:"));

    Serial.print(F("Rows: "));
    Serial.println(ROW_COUNT);

    Serial.print(F("Columns: "));
    Serial.println(COL_COUNT);

    Serial.print(F("Target: "));
    Serial.println(TARGET);

    Serial.print(F("Read buffer size: "));
    Serial.print(READ_BUFFER_SIZE);
    Serial.println(F(" bytes"));

    Serial.print(F("Rows per block: "));
    Serial.println(ROWS_PER_BLOCK);

    Serial.print(F("Blocks: "));
    Serial.println(BLOCK_COUNT);

    Serial.print(F("Runs: "));
    Serial.println(RUN_COUNT);

    Serial.print(F("Total database size: "));
    Serial.print(DB_SIZE);
    Serial.println(F(" bytes"));

    Serial.print(F("Start block: "));
    Serial.println(START_BLOCK);

    Serial.print(F("SD SPI speed: "));
    Serial.print(SD_SPI_SPEED);
    Serial.println(F(" Hz"));

    Serial.println();
}

void print_average_result(const __FlashStringHelper *name, double total_ms, uint32_t bytes)
{
    double average_ms = total_ms / RUN_COUNT;
    double throughput = ((double)bytes / 1000000.0) / (average_ms / 1000.0);

    Serial.print(name);
    Serial.print(F(": "));
    Serial.print(average_ms, 3);
    Serial.print(F(" ms, "));
    Serial.print(throughput, 3);
    Serial.println(F(" MB/s"));
}


// SERVICE

double create_database(void)
{
    prepare_block();

    uint32_t start_time = micros();

    for (uint16_t block_cnt = 0; block_cnt < BLOCK_COUNT; block_cnt++) {
        if (block_cnt == BLOCK_COUNT - 1) {
            memcpy(&read_buffer[(ROWS_PER_BLOCK - 1) * COL_COUNT], TARGET, sizeof(TARGET) - 1);
        }

        if (!card.writeBlock(START_BLOCK + block_cnt, read_buffer)) {
            fail(F("CREATE write"));
        }
    }

    return elapsed_ms(start_time);
}

double read_database_only(void)
{
    uint32_t start_time = micros();

    for (uint16_t block_cnt = 0; block_cnt < BLOCK_COUNT; block_cnt++) {
        if (!card.readBlock(START_BLOCK + block_cnt, read_buffer)) {
            fail(F("READ"));
        }
    }

    return elapsed_ms(start_time);
}

double search_database_target_at_start(void)
{
    uint16_t row_cnt = 0;
    uint32_t found_offset = UINT32_MAX;

    uint32_t start_time = micros();

    for (uint16_t block_cnt = 0; block_cnt < BLOCK_COUNT; block_cnt++) {
        if (!card.readBlock(START_BLOCK + block_cnt, read_buffer)) {
            fail(F("STARTS WITH read"));
        }

        for (uint8_t buffer_row_cnt = 0; buffer_row_cnt < ROWS_PER_BLOCK; buffer_row_cnt++) {
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
    double result = elapsed_ms(start_time);

    if (found_offset != TARGET_OFFSET) {
        fail(F("STARTS WITH target"));
    }

    return result;
}

double search_database_target_anywhere(void)
{
    uint16_t row_cnt = 0;
    uint32_t found_offset = UINT32_MAX;

    uint32_t start_time = micros();

    for (uint16_t block_cnt = 0; block_cnt < BLOCK_COUNT; block_cnt++) {
        if (!card.readBlock(START_BLOCK + block_cnt, read_buffer)) {
            fail(F("CONTAINS read"));
        }

        for (uint8_t buffer_row_cnt = 0; buffer_row_cnt < ROWS_PER_BLOCK; buffer_row_cnt++) {
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
    double result = elapsed_ms(start_time);

    if (found_offset != TARGET_OFFSET) {
        fail(F("CONTAINS target"));
    }

    return result;
}

double search_database_direct_target(void)
{
    uint16_t target_row = ROW_COUNT - 1;
    uint32_t target_block = START_BLOCK + target_row / ROWS_PER_BLOCK;
    uint16_t row_offset = (target_row % ROWS_PER_BLOCK) * COL_COUNT;

    uint32_t start_time = micros();

    if (!card.readBlock(target_block, read_buffer)) {
        fail(F("DIRECT read"));
    }

    bool target_match = true;

    for (uint8_t target_cnt = 0; target_cnt < sizeof(TARGET) - 1; target_cnt++) {
        if (read_buffer[row_offset + target_cnt] != TARGET[target_cnt]) {
            target_match = false;
            break;
        }
    }

    double result = elapsed_ms(start_time);

    if (!target_match) {
        fail(F("DIRECT target"));
    }

    return result;
}


// MAIN

void setup(void)
{
    Serial.begin(115200);

    delay(1000);

    print_configuration();

    if (ROW_COUNT % ROWS_PER_BLOCK != 0) {
        fail(F("ROW_COUNT alignment"));
    }

    if (!card.init(SPI_FULL_SPEED, SD_CS_PIN)) {
        fail(F("SD init"));
    }

    struct measurement_result result = {0};

    for (uint8_t run = 0; run < RUN_COUNT; run++) {
        result.create_file_total_ms += create_database();
        result.read_file_total_ms += read_database_only();
        result.starts_with_total_ms += search_database_target_at_start();
        result.contains_total_ms += search_database_target_anywhere();
        result.direct_access_total_ms += search_database_direct_target();

        Serial.print(F("."));
    }

    Serial.println();
    Serial.print(F("Average results ("));
    Serial.print(RUN_COUNT);
    Serial.println(F(" runs):"));

    print_average_result(F("CREATE"), result.create_file_total_ms, DB_SIZE);
    print_average_result(F("READ"), result.read_file_total_ms, DB_SIZE);
    print_average_result(F("STARTS WITH"), result.starts_with_total_ms, DB_SIZE);
    print_average_result(F("CONTAINS"), result.contains_total_ms, DB_SIZE);
    print_average_result(F("DIRECT ACCESS"), result.direct_access_total_ms, READ_BUFFER_SIZE);
}

void loop(void)
{
}
