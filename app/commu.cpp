#include "commu.h"

unsigned long long get_time_now_ms()
{
    return std::chrono::duration_cast<std::chrono::duration<unsigned long long, std::milli>>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::vector<each_pack> generate_of_message(const std::string& s)
{
    std::vector<each_pack> vec;

    for(size_t p = 0; p < s.size();)
    {
        std::string subs = s.substr(p, max_data_len);
        p += subs.size();

        each_pack pkg;
        pkg.has_more = true;
        pkg.len = static_cast<uint32_t>(subs.size());
        pkg.type = pack_type::LPACK_MESSAGE;
        for(size_t a = 0; a < pkg.len && a < max_data_len; ++a) pkg.raw.data[a] = subs[a];

        vec.push_back(std::move(pkg));
    }
    if (vec.size()) vec.back().has_more = false;

    return vec;
}

each_pack generate_ping()
{
    each_pack pkg;
    pkg.has_more = false;
    pkg.len = sizeof(_pack_ping_type);
    pkg.type = pack_type::LPACK_PING;
    pkg.raw.ping.millisec = get_time_now_ms();

    return pkg;
}

each_pack generate_version()
{
    each_pack pkg;
    pkg.has_more = false;
    pkg.len = sizeof(_pack_version);
    pkg.type = pack_type::LPACK_VERSIONCHECK;
    pkg.raw.version.current_version = comm_version;

    return pkg;
}
// check if type match size and size is less than max size
bool is_package_good(const each_pack& pkg)
{
    switch(pkg.type){
    case pack_type::LPACK_PING:
        return pkg.len == sizeof(_pack_ping_type) && pkg.has_more == false;
    case pack_type::LPACK_VERSIONCHECK:
        return pkg.len == sizeof(_pack_version) && pkg.has_more == false && pkg.raw.version.current_version == comm_version;
    default:
        return pkg.len < max_data_len;
    }
    return false;
}

unsigned long long calculate_ms_delay(const each_pack& pkg)
{
    if (pkg.type != pack_type::LPACK_PING || pkg.len != sizeof(_pack_ping_type)) {
        return static_cast<unsigned long long>(-1); 
    }

    return get_time_now_ms() - pkg.raw.ping.millisec;
}

void cat_message(std::string& s, const each_pack& pkg)
{
    if (pkg.type != pack_type::LPACK_MESSAGE || pkg.len == 0) return;

    s.append(pkg.raw.data, pkg.len);
}