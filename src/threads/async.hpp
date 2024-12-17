#pragma once

#include <future>
#include <iostream>
#include <chrono>

void testAsync()
{
    std::cout << "开始异步操作..." << std::endl;
    
    auto result = std::async(std::launch::async, []() {
        std::cout << "异步线程: 等待输入..." << std::endl;
        std::string input;
        std::cin >> input;  
        std::cout << "异步线程: 收到输入!" << std::endl;
        return input;
    });

    std::cout << "主线程: 等待异步结果..." << std::endl;
    std::cout << "异步结果: " << result.get() << std::endl;
    std::cout << "完成!" << std::endl;
}