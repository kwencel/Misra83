#ifndef INC_3PC_LOGGER_H
#define INC_3PC_LOGGER_H

#include <communication/ICommunicator.h>
#include <thread>
#include <map>
#include <mutex>
#include <functional>
#include "ConsoleColor.h"

#define LOGGER_NUMBER_DIGITS 4

class Logger {
public:

    static void init(std::shared_ptr<ICommunicator> communicator);

    static void log(const std::string& message, rang::fg color = rang::fg::reset, rang::style style = rang::style::reset,
                    rang::bg backgroundColor = rang::bg::reset);

    static void registerThread(std::string threadFriendlyName, rang::fg consoleColor = rang::fg::reset);

    static void setStateCollector(std::function<std::string()> stateCollector);

    static void setColorsEnabled(bool enabled);


private:

    static std::string getFormattedNumber(unsigned long number);
    static std::string getCurrentTime();

    static std::mutex mutex;
    static std::map<std::thread::id, std::pair<std::string, rang::fg>> threads;
    static unsigned logMessageCounter;
    static std::function<std::string()> stateQueryingFunction;
    static std::shared_ptr<ICommunicator> communicator;
    static bool colorsEnabled;
};


#endif //INC_3PC_LOGGER_H
