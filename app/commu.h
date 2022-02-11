#pragma once

#include <Lunaris/Socket/socket.h>

#include <vector>

const size_t max_data_len = 1 << 8;
const uint32_t comm_version = 1;

enum pack_type : uint32_t {LPACK_PING, LPACK_MESSAGE, LPACK_VERSIONCHECK};

struct _pack_ping_type {
    unsigned long long millisec;
};

struct _pack_version {
    uint32_t current_version;
};

struct each_pack {
    union {
        _pack_ping_type ping;
        _pack_version version;
        char data[max_data_len];
    } raw;

    uint32_t len;
    uint32_t type; // pack_type
    bool has_more;
};

unsigned long long get_time_now_ms();

std::vector<each_pack> generate_of_message(const std::string&);
each_pack generate_ping();
each_pack generate_version();

// check if type match size and size is less than max size
bool is_package_good(const each_pack&);

unsigned long long calculate_ms_delay(const each_pack&);
void cat_message(std::string&, const each_pack&);