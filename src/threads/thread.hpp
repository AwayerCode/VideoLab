#pragma once

#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

void testThread()
{
    boost::filesystem::path path("/home/user/test");
    if(boost::filesystem::exists(path))
    {
        std::cout << "File exists" << std::endl;
    }
    else
    {
        std::cout << "File does not exist" << std::endl;
    }
}