#pragma once

#include <iostream>
#include <memory>
#include <string>

// 目标接口（我们期望的接口）
class Target {
public:
    virtual ~Target() = default;
    virtual void request() = 0;
};

// 被适配的类（现有的接口）
class Adaptee {
public:
    void specificRequest() {
        std::cout << "Adaptee: handling specific request" << std::endl;
    }
};

// 对象适配器（通过组合方式）
class Adapter : public Target {
public:
    explicit Adapter(std::shared_ptr<Adaptee> adaptee) 
        : adaptee_(std::move(adaptee)) {}

    void request() override {
        std::cout << "Adapter: translating request" << std::endl;
        adaptee_->specificRequest();
    }

private:
    std::shared_ptr<Adaptee> adaptee_;
};

// 测试函数
void testAdapter() {
    // 创建被适配的对象
    auto adaptee = std::make_shared<Adaptee>();
    
    // 创建适配器
    auto adapter = std::make_shared<Adapter>(adaptee);
    
    // 通过适配器调用接口
    adapter->request();
}

