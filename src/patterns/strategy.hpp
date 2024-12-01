#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <list>
class SpliceBase {
public:
    virtual std::string execute(const std::list<std::string>& params) = 0;
};

class SpliceSpace : public SpliceBase {
public:
    std::string execute(const std::list<std::string>& params) override {
        std::cout << "SpliceSpace execute" << std::endl;
        std::string result = "";
        for (const auto& param : params) {
            result += param + " ";
        }

        return result;
    }
};

class SpliceUnderline : public SpliceBase {
public:
    std::string execute(const std::list<std::string>& params) override {
        std::cout << "SpliceUnderline execute" << std::endl;
        std::string result = "";
        for (const auto& param : params) {
            result += param + "_";
        }

        return result;
    }
};

class Context {
public:
    void addStrategy(std::shared_ptr<SpliceBase> strategy) {
        strategies.push_back(std::move(strategy));
    }

    void executeStrategy(const std::list<std::string>& params) {
        for (const auto& strategy : strategies) {
            auto result = strategy->execute(params);
            std::cout << result << std::endl;
        }
    }

private:
    std::vector<std::shared_ptr<SpliceBase>> strategies;
};

void testStrategy() {
    auto context = Context();
    context.addStrategy(std::make_shared<SpliceSpace>());
    context.addStrategy(std::make_shared<SpliceUnderline>());
    context.executeStrategy({"Hello", "World"});
}
