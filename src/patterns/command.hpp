#pragma once

#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// 使用函数对象作为命令
using TaskCommand = std::function<void(const std::string& chefName)>;

// 命令基类 - 使用函数对象封装命令
class TaskBase {
public:
    explicit TaskBase(TaskCommand cmd) : command(std::move(cmd)) {}
    virtual ~TaskBase() = default;

    void execute(const std::string& chefName) {
        if (command) {
            command(chefName);
        }
    }

    virtual void setDocument(const std::string& doc) {
        document = doc;
    }

    virtual std::string getDocument() const {
        return document;
    }

protected:
    std::string document;
    TaskCommand command;
};

// 具体命令A - 使用lambda表达式
class TaskA : public TaskBase {
public:
    TaskA() : TaskBase([this](const std::string& chefName) {
        std::cout << chefName << " TaskA execute: " << document << std::endl;
    }) {}
};

// 具体命令B - 使用lambda表达式
class TaskB : public TaskBase {
public:
    TaskB() : TaskBase([this](const std::string& chefName) {
        std::cout << chefName << " TaskB execute: " << document << std::endl;
    }) {}
};

// 命令执行者
class Chef {
public:
    enum ChefType {
        UiDev = 0,
        BaseDev = 1,
    };

    Chef(const std::string& name = "", ChefType type = UiDev) : 
        name(name), type(type) {}

    // 也可以直接接受lambda表达式作为任务
    void addLambdaTask(TaskCommand cmd) {
        task = std::make_shared<TaskBase>(std::move(cmd));
    }

    void addTask(std::shared_ptr<TaskBase> newTask) {
        task = std::move(newTask);
    }

    void execute() {
        if (task) {
            task->execute(name);
        }
    }

private:
    std::string name;
    ChefType type;
    std::shared_ptr<TaskBase> task;
};

// 命令调用者
class Manager {
public:
    void addChef(std::shared_ptr<Chef> chef) {
        chefs.push_back(std::move(chef));
    }

    // 分发普通任务
    void dispatch(std::shared_ptr<TaskBase> task) {
        for (const auto& chef : chefs) {
            chef->addTask(task);
            chef->execute();
        }
    }

    // 分发lambda任务
    void dispatchLambda(TaskCommand cmd) {
        for (const auto& chef : chefs) {
            chef->addLambdaTask(cmd);
            chef->execute();
        }
    }

private:
    std::vector<std::shared_ptr<Chef>> chefs;
};

// 测试函数
void testCommand() {
    Manager manager;
    manager.addChef(std::make_shared<Chef>("ChefA", Chef::UiDev));
    manager.addChef(std::make_shared<Chef>("ChefB", Chef::BaseDev));

    // 使用传统方式
    auto taskUiDev = std::make_shared<TaskA>();
    taskUiDev->setDocument("TaskUiDev");
    manager.dispatch(taskUiDev);

    // 使用lambda方式
    std::string customDoc = "CustomTask";
    manager.dispatchLambda([customDoc](const std::string& chefName) {
        std::cout << chefName << " Custom lambda task: " << customDoc << std::endl;
    });

    // 使用传统方式
    auto taskBaseDev = std::make_shared<TaskB>();
    taskBaseDev->setDocument("TaskBaseDev");
    manager.dispatch(taskBaseDev);
}
