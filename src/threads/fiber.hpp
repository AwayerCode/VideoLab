#pragma once

#include <iostream>

#include <folly/fibers/FiberManager.h>
#include <folly/fibers/FiberManagerMap.h>
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>

void testFiber()
{
    // 创建 EventBase
    folly::EventBase evb;
    
    // 获取 FiberManager
    auto& manager = folly::fibers::getFiberManager(evb);
    
    // 添加任务
    manager.addTask([]() {
        std::cout << "在 fiber 中执行" << std::endl;
    });

    // 运行事件循环
    evb.loop();
}