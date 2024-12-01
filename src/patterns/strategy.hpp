#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

class StrategyBase {
public:
    virtual void execute() = 0;
};

class StrategyA : public StrategyBase {
public:
    void execute() override {
        std::cout << "StrategyA execute" << std::endl;
    }
};

class StrategyB : public StrategyBase {
public:
    void execute() override {
        std::cout << "StrategyB execute" << std::endl;
    }
};

class Context {
public:
    void addStrategy(std::shared_ptr<StrategyBase> strategy) {
        strategies.push_back(std::move(strategy));
    }

    void executeStrategy() {
        for (const auto& strategy : strategies) {
            strategy->execute();
        }
    }

private:
    std::vector<std::shared_ptr<StrategyBase>> strategies;
};

void testStrategy() {
    auto strategyA = std::make_shared<StrategyA>();
    auto strategyB = std::make_shared<StrategyB>();
    auto context = Context();
    context.addStrategy(strategyA);
    context.addStrategy(strategyB);
    
    context.executeStrategy();
}
