#pragma once

#include <iostream>
#include <thread>

#include <boost/asio.hpp>

namespace asio = boost::asio;

void testAsio()
{
    asio::io_context io;
    asio::post(io, []() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "Test Asio!" << std::endl;
    });
    io.run();
}