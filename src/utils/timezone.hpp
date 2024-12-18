#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

using namespace boost::local_time;
using namespace boost::posix_time;

void testTimezone()
{
    try {
        tz_database tz_db;
        tz_db.load_from_file("/usr/share/zoneinfo/posix/Asia/Shanghai");

        time_zone_ptr tz = tz_db.time_zone_from_region("Asia/Shanghai");
        std::cout << "时区名称: " << tz->std_zone_name() << std::endl;
        std::cout << "夏令时缩写: " << tz->dst_zone_abbrev() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "时区错误: " << e.what() << std::endl;
    }
}