#pragma once

#include <iostream>
#include <memory>
#include <string>

// 前向声明
class Order;

// 状态枚举
enum class OrderState {
    UnPaid,
    Paid,
    Delivered,
    Completed
};

// 状态基类
class StateBase {
public:
    virtual ~StateBase() = default;
    virtual bool doAction(Order& order) = 0;
    virtual OrderState getState() const = 0;
    virtual std::string getName() const = 0;
};

// 具体状态类
class StateUnPaid : public StateBase {
public:
    bool doAction(Order& order) override;
    OrderState getState() const override { return OrderState::UnPaid; }
    std::string getName() const override { return "UnPaid"; }
};

class StatePaid : public StateBase {
public:
    bool doAction(Order& order) override;
    OrderState getState() const override { return OrderState::Paid; }
    std::string getName() const override { return "Paid"; }
};

class StateDelivered : public StateBase {
public:
    bool doAction(Order& order) override;
    OrderState getState() const override { return OrderState::Delivered; }
    std::string getName() const override { return "Delivered"; }
};

class StateCompleted : public StateBase {
public:
    bool doAction(Order& order) override;
    OrderState getState() const override { return OrderState::Completed; }
    std::string getName() const override { return "Completed"; }
};

// 订单类
class Order {
public:
    Order() : state(std::make_shared<StateUnPaid>()) {}

    bool doAction() {
        if (!state) return false;
        return state->doAction(*this);
    }

    void setState(std::shared_ptr<StateBase> newState) {
        if (newState) {
            std::cout << "State changed from " << state->getName() 
                     << " to " << newState->getName() << std::endl;
            state = std::move(newState);
        }
    }

    OrderState getCurrentState() const {
        return state ? state->getState() : OrderState::UnPaid;
    }

    std::string getCurrentStateName() const {
        return state ? state->getName() : "Unknown";
    }

private:
    std::shared_ptr<StateBase> state;
};

// 具体状态类的实现
bool StateUnPaid::doAction(Order& order) {
    std::cout << "Processing payment..." << std::endl;
    // 模拟支付处理
    order.setState(std::make_shared<StatePaid>());
    return true;
}

bool StatePaid::doAction(Order& order) {
    std::cout << "Starting delivery..." << std::endl;
    // 模拟发货处理
    order.setState(std::make_shared<StateDelivered>());
    return true;
}

bool StateDelivered::doAction(Order& order) {
    std::cout << "Confirming delivery..." << std::endl;
    // 模拟确认收货
    order.setState(std::make_shared<StateCompleted>());
    return true;
}

bool StateCompleted::doAction(Order& order) {
    std::cout << "Order already completed!" << std::endl;
    return false;
}

// 测试函数
void testState() {
    Order order;
    
    // 显示初始状态
    std::cout << "Initial state: " << order.getCurrentStateName() << std::endl;
    
    // 执行状态转换
    order.doAction();  // UnPaid -> Paid
    order.doAction();  // Paid -> Delivered
    order.doAction();  // Delivered -> Completed
    order.doAction();  // Completed (无法继续转换)
}
