#include <utility>

#ifndef MISRA_PROCESS_H
#define MISRA_PROCESS_H

#include <mutex>
#include <condition_variable>
#include <communication/ITaggedCommunicator.h>
#include <communication/ICommunicator.h>
#include <communication/CommunicationManager.h>
#include <util/Random.h>

using TokenVal = int;

struct Token {
    TokenVal value;
    bool isPresent;

    [[nodiscard]] std::string toString() const noexcept {
        return util::concat("[", value, ", ", isPresent ? "yes" : "no ", "]");
    }
};

class Process {
public:

    explicit Process(std::shared_ptr<CommunicationManager> monitor)
        :  monitor(std::move(monitor)) {

        Logger::setStateCollector([&] {
            std::stringstream ss;
            ss << std::setw(2) << std::setfill(' ') << m;
            std::string m = ss.str();
            auto processId = this->monitor->getProcessId();
            return util::concat("(ping: ", ping.toString(), ", pong: ", pong.toString(),", m: ", m, ")[P", processId, "] ");
        });

        this->monitor->subscribe([](const Packet& p) { return p.messageType == MessageType::CRASH; }, [&](const Packet& p) {
            if (p.message == "PING") {
                omitNextPing = true;
            } else if (p.message == "PONG") {
                omitNextPong = true;
            } else {
                Logger::log("Unexpected packet ");
            }
        });

        if (this->monitor->getProcessId() == 0) {
            ping.isPresent = true;
            pong.isPresent = true;
            std::thread([&]{
                while (true) {
                    Logger::registerThread("Input");
                    ProcessId process;
                    char token;
                    std::cin >> token >> process;
                    if (process >= 0 and process < this->monitor->getNumberOfProcesses()) {
                        if (token == 'q') {
                            Logger::log(util::concat("P", process, " will omit the next PING"));
                            this->monitor->send(MessageType::CRASH, "PING", process);
                            continue;
                        } else if (token == 'w') {
                            Logger::log(util::concat("P", process, " will omit the next PONG"));
                            this->monitor->send(MessageType::CRASH, "PONG", process);
                            continue;
                        }
                    }
                    Logger::log(util::concat("Unexpected input '", process, " ", token,"'", " - ignoring"));
                }
            }).detach();
        }

        this->monitor->subscribe([](const Packet& p) { return p.messageType == MessageType::PING; }, [&](const Packet& p) {
            if (omitNextPing) {
                Logger::log(util::concat("Omitted PING from P", p.source));
                omitNextPing = false;
                return;
            }
            if (std::abs(std::stoi(p.message)) < std::abs(m)) {
                Logger::log("An old ping has arrived - ignoring it", rang::fg::blue);
                return;
            }
            std::unique_lock<std::mutex> lock(csMutex);
            ping = { .value = std::stoi(p.message), .isPresent = true };
            bool pongRegenerated = false;
            if (m == ping.value) {
                // PONG got lost
                regenerate(ping.value);
                pongRegenerated = true;
            }
            if (ping.isPresent and pong.isPresent) {
                // Both PING and PONG have met in the same process (possibly due to the regeneration)
                incarnate(ping.value);
            }
            // Allow the main thread to enter critical section
            csCond.notify_one();
            lock.unlock();

            if (pongRegenerated) {
                /* If pong was just regenerated, we also want to send it. We make sure it is never sent before PING
                   if both tokens exists in the process */
                sendPong();
            }
        });

        this->monitor->subscribe([](const Packet& p) { return p.messageType == MessageType::PONG; }, [&](const Packet& p) {
            if (omitNextPong) {
                Logger::log(util::concat("Omitted PONG from P", p.source), rang::fg::red);
                omitNextPong = false;
                return;
            }
            if (std::abs(std::stoi(p.message)) < std::abs(m)) {
                Logger::log("An old pong has arrived - ignoring it", rang::fg::blue);
                return;
            }
            pong = { .value = std::stoi(p.message), .isPresent = true };
            if (m == pong.value) {
                // PING got lost
                regenerate(pong.value);
            }
            if (ping.isPresent and pong.isPresent) {
                // Both PING and PONG have met in the same process (possibly due to the regeneration)
                incarnate(ping.value);
                csCond.notify_one();
            }

            /* We just received pong so we can surely send it. We make sure this operation is delayed until the PING
               is sent if the process has it */
            sendPong();
        });
    }

    void sendPong() {
        std::unique_lock<std::mutex> lock(tokensMutex);
//        Logger::log("Waiting for ping to clear to send the pong");
        pongCond.wait(lock, [&]() { return not ping.isPresent; });
//        Logger::log("Ping was sent, so I send pong");
        send(MessageType::PONG, pong);
    }

    void run() {
        // Wait for entering critical section
        while (true) {
            std::unique_lock<std::mutex> csLock(csMutex);
            csCond.wait(csLock, [&]() { return ping.isPresent; });

            // Enter critical section
            Logger::log("Entered CS", rang::fg::green);
            sleep<int>();
            Logger::log("Left CS", rang::fg::green);

            // Send token(s) to the next process
            std::lock_guard<std::mutex> guard(tokensMutex);
            sleep(1000, 2000);
            send(MessageType::PING, ping);

            // Only needed at the start to bootstrap the tokens in the ring
            if (bootstrap && monitor->getProcessId() == 0) {
                send(MessageType::PONG, pong);
                bootstrap = false;
            }
            pongCond.notify_one();
        }
    }

    void regenerate(TokenVal value) {
        std::lock_guard<std::mutex> guard(tokensMutex);
        Logger::log("REGENERATE", rang::fg::gray);
        ping = { .value = std::abs(value), .isPresent = true };
        pong = { .value = -ping.value, .isPresent = true };
    }

    void incarnate(TokenVal value) {
        std::lock_guard<std::mutex> guard(tokensMutex);
        Logger::log("INCARNATE", rang::fg::gray);
//        ping.value = (std::abs(value) + 1) % (monitor->getNumberOfProcesses() + 1);
        ping.value = (std::abs(value) + 1);
        pong.value = -ping.value;
    }

    void send(MessageType messageType, Token& token) {
        if (not token.isPresent) {
            throw std::runtime_error("Tried to send a token that the process does not possess");
        }
        ProcessId nextProcess = (monitor->getProcessId() + 1) % monitor->getNumberOfProcesses();
        monitor->send(messageType, std::to_string(token.value), nextProcess);
        token.isPresent = false;
        m = token.value;
    }

protected:
    template <typename T>
    void sleep(T min = MIN_SLEEP_TIME, T max = MAX_SLEEP_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(random.randomBetween(min, max)));
    }

    std::shared_ptr<CommunicationManager> monitor;


private:
    /** Misra algorithm variables **/
    Token ping { .value = 1, .isPresent = false };
    Token pong { .value = -1, .isPresent = false };
    TokenVal m = 0; // last sent token value
    bool omitNextPing = false;
    bool omitNextPong = false;
    bool bootstrap = true;

    /** Internal synchronization variables **/
    std::mutex csMutex;
    std::condition_variable csCond;
    std::mutex tokensMutex;
    std::condition_variable pongCond;

    Random random;
};


#endif //MISRA_PROCESS_H
