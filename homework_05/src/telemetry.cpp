#include "telemetry.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>

// Debugging exercise notes:
// this file intentionally contains four runtime defects.
// The defects are related to malformed input shape, invalid numeric values,
// unsafe time deltas, and empty logs. Exact locations are not marked on purpose.

const int EXPECTED_FIELD_COUNT = 7;
const int MAX_LINE_LENGTH = 256;

int split_line(char line[], char* fields[], int max_fields) {
    int count = 0;
    char* cursor = line;

    while (*cursor != '\0' && count < max_fields) {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r') {
            *cursor = '\0';
            ++cursor;
        }

        if (*cursor == '\0') {
            break;
        }

        fields[count] = cursor;
        ++count;

        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t' && *cursor != '\n' &&
               *cursor != '\r') {
            ++cursor;
        }
    }

    return count;
}


bool is_valid_conversion(const char* text, char* end) {
    if (end == text) {
        std::cerr << "Error: No digits found in field: " << text << "\n";
        return false;
    }
    if (*end != '\0') {
        std::cerr << "Error: Field contains non-numeric characters: " << text << "\n";
        return false;
    }
    return true;
}

long parse_long(const char* text) {
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (!is_valid_conversion(text, end)) return -1;
    return value;
}

int parse_int(const char* text) {
    return static_cast<int>(parse_long(text));
}

double parse_double(const char* text) {
    char* end = nullptr;
    const double value = std::strtod(text, &end);
    if (!is_valid_conversion(text, end)) return -1;
    return value;
}

Frame parse_frame(char line[]) {
    char* fields[EXPECTED_FIELD_COUNT] = {};
    const int field_count = split_line(line, fields, EXPECTED_FIELD_COUNT);

    Frame frame{};

    if (field_count < EXPECTED_FIELD_COUNT) {
        std::cerr << "Error: Invalid telemetry frame. Expected at least 7 fields, but got " << field_count << "\n";
        frame.timestamp_ms = INVALID_VALUE;
        return frame;
    }
   
    frame.timestamp_ms = parse_long(fields[0]);
    frame.seq = parse_int(fields[1]);
    frame.voltage_v = parse_double(fields[2]);
    frame.current_a = parse_double(fields[3]);
    frame.temperature_c = parse_double(fields[4]);
    frame.gps_fix = parse_int(fields[5]);
    frame.satellites = parse_int(fields[6]);
    return frame;
}

double compute_frame_rate_hz(const Frame frames[], int frame_count) {
    const long elapsed_ms = frames[frame_count - 1].timestamp_ms - frames[0].timestamp_ms;

    return static_cast<double>((frame_count - 1) * 1000 / elapsed_ms);
}

int read_frames(const char* path, Frame frames[], int max_frames) {
    std::ifstream input{path};
    if (!input) {
        std::cerr << "error: failed to open input file: " << path << '\n';
        return 0;
    }

    int frame_count = 0;
    char line[MAX_LINE_LENGTH];

    int cur_seq = 0;
    int cur_timestamp = -1;

    while (input.getline(line, MAX_LINE_LENGTH)) {
        if (line[0] == '\0') {
            continue;
        }

        if (frame_count < max_frames) {
            Frame f = parse_frame(line);

            if (!f) 
            {
                return -1; 
            }

            if (f.voltage_v <= 0) {
                std::cerr << "error: voltage_v is non-positive, value: " << f.voltage_v << '\n';
                return -1;
            }

            if (f.temperature_c < -40 || f.temperature_c > 120) {
                std::cerr << "error: temperature_c should be in [-40, 120], value: " << f.temperature_c << '\n';
                return -1;
            }

            if (f.gps_fix != 0 && f.gps_fix != 1) {
                std::cerr << "error: gps_fix should be 0 or 1, value: " << f.gps_fix << '\n';
                return -1;
            }

            if (f.satellites < 0) {
                std::cerr << "error: satellites is less than zero, value: " << f.satellites << '\n';
                return -1;
            }

            if(cur_seq != 0 && f.seq - cur_seq != 1) {
                 std::cerr << "error: seq must be exactly one greater than the preceding value, value: " << f.seq << '\n';
                return -1;
            }

            if(cur_timestamp != -1 && f.timestamp_ms - cur_timestamp <= 0) {
                 std::cerr << "error: timestamp_ms must be greater than the preceding value, value: " << f.timestamp_ms << '\n';
                return -1;
            }

            frames[frame_count] = f;
            cur_seq = f.seq;
            cur_timestamp = f.timestamp_ms;
            ++frame_count;
        }
    }

    return frame_count;
}

Summary summarize(const Frame frames[], int frame_count) {
    Summary summary{};
    summary.frames_total = frame_count;
    summary.frames_valid = frame_count;
    summary.voltage_min = frames[0].voltage_v;
    summary.voltage_max = frames[0].voltage_v;
    summary.low_voltage_frames = 0;

    double temperature_sum = 0.0;

    for (int i = 0; i < frame_count; ++i) {
        if (frames[i].voltage_v < summary.voltage_min) {
            summary.voltage_min = frames[i].voltage_v;
        }

        if (frames[i].voltage_v > summary.voltage_max) {
            summary.voltage_max = frames[i].voltage_v;
        }

        temperature_sum += frames[i].temperature_c;

        if (frames[i].voltage_v < 22.0) {
            ++summary.low_voltage_frames;
        }
    }

    const int temperature_tenths = static_cast<int>(temperature_sum * 10.0) / frame_count;
    summary.temperature_avg = static_cast<double>(temperature_tenths) / 10.0;
    summary.frame_rate_hz = compute_frame_rate_hz(frames, frame_count);
    return summary;
}

void print_summary(const Summary& summary) {
    std::cout << "frames_total " << summary.frames_total << '\n';
    std::cout << "frames_valid " << summary.frames_valid << '\n';
    std::cout << "voltage_min " << summary.voltage_min << '\n';
    std::cout << "voltage_max " << summary.voltage_max << '\n';
    std::cout << "temperature_avg " << summary.temperature_avg << '\n';
    std::cout << "low_voltage_frames " << summary.low_voltage_frames << '\n';
    std::cout << "frame_rate_hz " << summary.frame_rate_hz << '\n';
}
