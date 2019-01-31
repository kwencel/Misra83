#ifndef INC_3PC_DEFINE_H
#define INC_3PC_DEFINE_H

#include <map>

#define ROUND_TIME 10000
#define MIN_SLEEP_TIME 6000
#define MAX_SLEEP_TIME 7000
#define MIN_SLEEP_TIME_COORDINATOR 4000
#define MAX_SLEEP_TIME_COORDINATOR 5000
#define COORDINATOR_ID 0
#define MPI_CRASH_TAG 100

enum State : unsigned char {
    Q, W, A, P ,C
};

const std::map<State, std::string>  stateString = {{State::Q, "Q"},
                                                   {State::W, "W"},
                                                   {State::A, "A"},
                                                   {State::P, "P"},
                                                   {State::C, "C"}};

inline std::ostream& operator<< (std::ostream& os, State messageType) {
    return os << stateString.at(messageType);
}

inline std::string& operator+ (std::string& str, State messageType) {
    return str.append(stateString.at(messageType));
}

namespace std {
    template<>
    struct hash<State> {
        inline int operator()(const State& state) const {
            return static_cast<std::underlying_type<State>::type>(state);
        }
    };
}

enum class MessageType : unsigned char {
    PING, PONG, CRASH
};

const std::map<MessageType, std::string>  messageTypeString = {{MessageType::PING, "PING"},
                                                               {MessageType::PONG, "PONG"},
                                                               {MessageType::CRASH, "CRASH"}};

inline std::ostream& operator<< (std::ostream& os, MessageType messageType) {
    return os << messageTypeString.at(messageType);
}

inline std::string& operator+ (std::string& str, MessageType messageType) {
    return str.append(messageTypeString.at(messageType));
}

namespace std {
    template<>
    struct hash<MessageType> {
        inline int operator()(const MessageType& messageType) const {
            return static_cast<std::underlying_type<MessageType>::type>(messageType);
        }
    };
}

#endif //INC_3PC_DEFINE_H
