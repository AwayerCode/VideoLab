#pragma once

#include <QtCore/QObject>
#include <iostream>

class SignalManager : public QObject
{
    Q_OBJECT

public:
    SignalManager(QObject* parent = nullptr) : QObject(parent) {}

signals:
    void valueChanged();

public:
    void emitSignal() {
        emit valueChanged();
    }
};

class SignalWorker : public QObject
{
    Q_OBJECT

public:
    SignalWorker(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void onValueChanged()
    {
        std::cout << "Worker received signal" << std::endl;
    }
};

// 只声明函数，实现移到 cpp 文件中
void testSignalSlot();