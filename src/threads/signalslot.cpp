#include "signalslot.hpp"

#include <memory>

void testSignalSlot()
{
    std::unique_ptr<SignalManager> manager = std::make_unique<SignalManager>();
    std::unique_ptr<SignalWorker> worker = std::make_unique<SignalWorker>();

    QObject::connect(manager.get(), &SignalManager::valueChanged, 
                    worker.get(), &SignalWorker::onValueChanged, Qt::QueuedConnection);

    manager->emitSignal();
} 