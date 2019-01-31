#ifndef INC_3PC_COMMUNICATIONMANAGER_H
#define INC_3PC_COMMUNICATIONMANAGER_H

#include <memory>
#include <thread>
#include <mutex>
#include <logging/Logger.h>
#include <util/StringConcat.h>
#include <util/Utils.h>
#include <unordered_map>
#include <functional>
#include "ICommunicator.h"

using SubscriptionId = std::size_t;
using SubscriptionPredicate = std::function<bool(Packet)>;
using SubscriptionCallback = std::function<void(Packet)>;

class CommunicationManager {
public:

    explicit CommunicationManager(std::shared_ptr<ICommunicator> communicator) : communicator(
            std::move(communicator)) {};

    virtual ~CommunicationManager() {
        terminate = true;
        if (receivingThread and receivingThread->joinable()) {
            receivingThread->join();
        }
    }

    void listen() {
        if (not receivingThread) {
            receivingThread = std::make_unique<std::thread>(threadFunction);
        }
    }

    SubscriptionId subscribe(SubscriptionPredicate predicate, SubscriptionCallback callback) {
        std::lock_guard<std::mutex> lock(subscriptionMutex);
        subscriptions[subscriptionSeqNo] = {predicate, callback};
        return subscriptionSeqNo++;
    }

    void unsubscribe(SubscriptionId id) {
        std::lock_guard<std::mutex> lock(subscriptionMutex);
        subscriptions.erase(id);
    }

    Packet send(MessageType messageType, const std::string& message, ProcessId recipient) {
        Logger::log(util::concat("Sending to process ", recipient, " ", printPacket(messageType, message)));
        return communicator->send(messageType, message, recipient);
    }

    Packet send(MessageType messageType, const std::string& message, const std::unordered_set<ProcessId>& recipients) {
        Logger::log(util::concat("Sending to processes ", printContainer(recipients), " ",
                                 printPacket(messageType, message)));
        return communicator->send(messageType, message, recipients);
    }

    Packet sendOthers(MessageType messageType, const std::string& message) {
        Logger::log("Sending to other processes " + printPacket(messageType, message));
        return communicator->sendOthers(messageType, message);
    }

    ProcessId getProcessId() {
        return communicator->getProcessId();
    }

    ProcessId getNumberOfProcesses() {
        return communicator->getNumberOfProcesses();
    }

    LamportTime getCurrentLamportTime() {
        return communicator->getCurrentLamportTime();
    }

protected:

    std::function<void()> threadFunction = [&]() {
        Logger::registerThread("Recv", rang::fg::yellow);
        while (not terminate.load()) {

            Packet packet = communicator->receive();
            Logger::log(util::concat("Received packet from process ", packet.source, " ",
                                     printPacket(packet.messageType, packet.message)));
            std::lock_guard<std::mutex> lock(subscriptionMutex);
            bool anyCallbackInvoked = false;
            for (const auto& subscription : subscriptions) {
                const auto&[predicate, callback] = subscription.second;
                if (predicate(packet)) {
                    callback(packet);
                    anyCallbackInvoked = true;
                }
            }
            if (not anyCallbackInvoked) {
                auto error = "WARNING! No callback invoked for packet with TS " + std::to_string(packet.lamportTime) +
                             " " + printPacket(packet.messageType, packet.message);
                Logger::log(error);
                throw std::runtime_error(error);
            }
        }
    };

    static const std::string printPacket(MessageType messageType, const std::string& message) {
        return util::concat("[messageType: ", messageType, ", message: ", message, ']');
    }

    std::unordered_map<SubscriptionId, std::pair<SubscriptionPredicate, SubscriptionCallback>> subscriptions;
    SubscriptionId subscriptionSeqNo = 0;
    std::shared_ptr<ICommunicator> communicator;
    std::unique_ptr<std::thread> receivingThread;
    std::atomic<bool> terminate = false;
    std::mutex subscriptionMutex;
};


#endif //INC_3PC_COMMUNICATIONMANAGER_H
