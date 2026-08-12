#include <SPI.h>
#include <SD.h>
#include <string.h>

#define SD_CS_PIN 10
#define SD_SPI_SPEED 8000000UL

#define DB_FILE "DB.RAW"

#define ROW_COUNT 10000UL
#define COL_COUNT 32
#define TARGET "abcdefgh"
#define READ_BUFFER_SIZE 512
#define RUN_COUNT 3

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

File db_file;
char read_buffer[READ_BUFFER_SIZE];


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

    Serial.print(F("Runs: "));
    Serial.println(RUN_COUNT);

    Serial.print(F("Total file size: "));
    Serial.print(DB_SIZE);
    Serial.println(F(" bytes"));

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
    struct db_row db_row;

    memset(db_row.col, '0', COL_COUNT - 1);
    db_row.col[COL_COUNT - 1] = '\n';

    if (SD.exists(DB_FILE)) {
        SD.remove(DB_FILE);
    }

    db_file = SD.open(DB_FILE, FILE_WRITE);

    if (!db_file) {
        fail(F("CREATE open"));
    }

    uint32_t start_time = micros();

    for (uint16_t row_cnt = 0; row_cnt < ROW_COUNT; row_cnt++) {
        if (row_cnt == ROW_COUNT - 1) {
            memcpy(db_row.col, TARGET, sizeof(TARGET) - 1);
        }

        db_file.write((uint8_t *)db_row.col, COL_COUNT);
    }

    db_file.flush();
    db_file.close();

    double result = elapsed_ms(start_time);

    db_file = SD.open(DB_FILE, FILE_READ);

    if (!db_file) {
        fail(F("CREATE verify open"));
    }

    uint32_t file_size = db_file.size();
    db_file.close();

    if (file_size != DB_SIZE) {
        fail(F("CREATE size"));
    }

    return result;
}

double read_database_only(void)
{
    db_file = SD.open(DB_FILE, FILE_READ);

    if (!db_file) {
        fail(F("READ open"));
    }

    uint32_t start_time = micros();
    int bytes_read;

    while ((bytes_read = db_file.read(read_buffer, READ_BUFFER_SIZE)) > 0) {
    }

    double result = elapsed_ms(start_time);
    uint32_t final_position = db_file.position();

    db_file.close();

    if (bytes_read < 0) {
        fail(F("READ error"));
    }

    if (final_position != DB_SIZE) {
        fail(F("READ incomplete"));
    }

    return result;
}

double search_database_target_at_start(void)
{
    uint16_t row_cnt = 0;
    uint32_t found_offset = UINT32_MAX;

    db_file = SD.open(DB_FILE, FILE_READ);

    if (!db_file) {
        fail(F("STARTS WITH open"));
    }

    uint32_t start_time = micros();
    int bytes_read;

    while ((bytes_read = db_file.read(read_buffer, READ_BUFFER_SIZE)) > 0) {
        uint8_t rows_in_buffer = bytes_read / COL_COUNT;

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
    double result = elapsed_ms(start_time);

    db_file.close();

    if (found_offset != TARGET_OFFSET) {
        fail(F("STARTS WITH target"));
    }

    return result;
}

double search_database_target_anywhere(void)
{
    uint16_t row_cnt = 0;
    uint32_t found_offset = UINT32_MAX;

    db_file = SD.open(DB_FILE, FILE_READ);

    if (!db_file) {
        fail(F("CONTAINS open"));
    }

    uint32_t start_time = micros();
    int bytes_read;

    while ((bytes_read = db_file.read(read_buffer, READ_BUFFER_SIZE)) > 0) {
        uint8_t rows_in_buffer = bytes_read / COL_COUNT;

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
    double result = elapsed_ms(start_time);

    db_file.close();

    if (found_offset != TARGET_OFFSET) {
        fail(F("CONTAINS target"));
    }

    return result;
}

double search_database_direct_target(void)
{
    db_file = SD.open(DB_FILE, FILE_READ);

    if (!db_file) {
        fail(F("DIRECT open"));
    }

    uint32_t start_time = micros();

    bool seek_result = db_file.seek(TARGET_OFFSET);
    int bytes_read = db_file.read(read_buffer, COL_COUNT);

    bool target_match = true;

    for (uint8_t target_cnt = 0; target_cnt < sizeof(TARGET) - 1; target_cnt++) {
        if (read_buffer[target_cnt] != TARGET[target_cnt]) {
            target_match = false;
            break;
        }
    }

    double result = elapsed_ms(start_time);

    db_file.close();

    if (!seek_result) {
        fail(F("DIRECT seek"));
    }

    if (bytes_read != COL_COUNT) {
        fail(F("DIRECT read"));
    }

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

    if (!SD.begin(SD_SPI_SPEED, SD_CS_PIN)) {
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

    Serial.println(F(""));
    Serial.print(F("Average results ("));
    Serial.print(RUN_COUNT);
    Serial.println(F(" runs):"));

    print_average_result(F("CREATE"), result.create_file_total_ms, DB_SIZE);
    print_average_result(F("READ"), result.read_file_total_ms, DB_SIZE);
    print_average_result(F("STARTS WITH"), result.starts_with_total_ms, DB_SIZE);
    print_average_result(F("CONTAINS"), result.contains_total_ms, DB_SIZE);
    print_average_result(F("DIRECT ACCESS"), result.direct_access_total_ms, COL_COUNT);
}

void loop(void)
{
}
