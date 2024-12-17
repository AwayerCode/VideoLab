#pragma once

#include <folly/futures/Future.h>
#include <folly/executors/ThreadedExecutor.h>
#include <iostream>
#include <thread>
#include <chrono>

// 模拟一个异步操作，返回一个数字
folly::Future<int> asyncOperation() {
    // 创建一个 Promise
    auto promise = std::make_shared<folly::Promise<int>>();
    auto future = promise->getFuture();

    // 在新线程中执行异步操作
    std::thread([promise]() {
        // 模拟耗时操作
        std::this_thread::sleep_for(std::chrono::seconds(2));
        // 设置结果
        promise->setValue(42);
    }).detach();

    return future;
}

void testThread() {
    // 创建线程池执行器
    folly::ThreadedExecutor executor;

    std::cout << "开始异步操作..." << std::endl;

    // 启动异步操作并添加回调
    asyncOperation()
        .via(&executor)
        .thenValue([](int result) {
            std::cout << "异步操作完成，结果为: " << result << std::endl;
        })
        .thenError([](const folly::exception_wrapper& e) {
            std::cout << "发生错误: " << e.what() << std::endl;
        });

    // 等待一段时间以便看到结果
    std::this_thread::sleep_for(std::chrono::seconds(3));
}