#pragma once

#include <iostream>

class StateBase {
public:
    virtual void doAction() = 0;
};

class StateA : public StateBase {
public:
    void doAction() override {
        std::cout << "StateA doAction" << std::endl;
    }
};

class StateB : public StateBase {
public:
    void doAction() override {
        std::cout << "StateB doAction" << std::endl;
    }
};

class StateC : public StateBase {
public:
    void doAction() override {
        std::cout << "StateC doAction" << std::endl;
    }
};

class StateContext {
public:
    StateContext() {
        states = std::make_shared<std::list<std::shared_ptr<StateBase>>>();
    }
    void addState(std::shared_ptr<StateBase> state) {
        states->push_back(state);
    }

    void doAction() {
        for (const auto& state : *states) {
            state->doAction();
        }
    }

private:
    std::shared_ptr<std::list<std::shared_ptr<StateBase>>> states;
};

void testState() {
    auto context = StateContext();
    context.addState(std::make_shared<StateA>());
    context.addState(std::make_shared<StateB>());
    context.addState(std::make_shared<StateC>());

    context.doAction();
}
