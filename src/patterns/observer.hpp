#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <mutex>

class ObserverBase {
public:
    virtual ~ObserverBase() = default;

    virtual void update(const std::string& message) = 0;
};

class Observer : public ObserverBase {
public:
    Observer(const std::string& name) : name(name) {}

    void update(const std::string& message) override {
        std::cout << name << " received message: " << message << std::endl;
    }

private:
    std::string name;
};


class Subject {
public:
    Subject() = default;
    virtual ~Subject() = default;

    void addObserver(std::shared_ptr<ObserverBase> observer)
    {
        if (observer) { 
            std::lock_guard<std::mutex> lock(mutex);
            observers.push_back(observer);
        }
    }

    void removeObserver(std::shared_ptr<ObserverBase> observer)
    {
        if (observer) {
            std::lock_guard<std::mutex> lock(mutex);
            auto it = std::find(observers.begin(), observers.end(), observer);
            if (it != observers.end()) {
                observers.erase(it);
            }
        }
    }

    void notifyObservers(const std::string& message)
    {
        for (auto& observer : observers) {
            observer->update(message);
        }
    }

private:
    std::vector<std::shared_ptr<ObserverBase>> observers;
    std::mutex mutex;
};


void testObserver()
{
    Subject subject;
    auto observer1 = std::make_shared<Observer>("Observer 1");
    auto observer2 = std::make_shared<Observer>("Observer 2");
    subject.addObserver(observer1);
    subject.addObserver(observer2);

    subject.notifyObservers("Hello, world!");
}