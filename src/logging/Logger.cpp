#include <mutex>
#include <util/Define.h>
#include <iomanip>
#include "Logger.h"

std::mutex Logger::mutex;
std::map<std::thread::id, std::pair<std::string, rang::fg>> Logger::threads;
unsigned Logger::logMessageCounter = 0;
std::function<std::string()> Logger::stateQueryingFunction;
std::shared_ptr<ICommunicator> Logger::communicator;
bool Logger::colorsEnabled = true;


void Logger::init(std::shared_ptr<ICommunicator> communicator) {
    Logger::communicator = std::move(communicator);
}

void Logger::registerThread(std::string threadFriendlyName, rang::fg consoleColor) {
    std::lock_guard<std::mutex> guard(mutex);
    threads[std::this_thread::get_id()] = {std::move(threadFriendlyName), consoleColor};
}

void Logger::log(const std::string& message, rang::fg color, rang::style style, rang::bg backgroundColor) {
    std::lock_guard<std::mutex> guard(mutex);
    auto [threadId, threadColor] = threads[std::this_thread::get_id()];
    ProcessId myProcessId = communicator->getProcessId();
    std::string formattedLamportTime = getFormattedNumber(communicator->getCurrentLamportTime());
    std::string state = stateQueryingFunction ? stateQueryingFunction() : "";

    if (not colorsEnabled) {
        color = rang::fg::reset;
        style = rang::style::reset;
        backgroundColor = rang::bg::reset;
        threadColor = rang::fg::reset;
    }
    std::cout << "[TS " << formattedLamportTime << ":" << getFormattedNumber(logMessageCounter++) << " " << getCurrentTime()
              << " Process " <<  myProcessId << threadColor << " Thread " << threadId << rang::fg::reset << "]: " << state
              << color << style << backgroundColor << message << rang::style::reset << rang::fg::reset << rang::bg::reset << std::endl;
}

std::string Logger::getFormattedNumber(unsigned long number) {
    std::string numberAsString = std::to_string(number);
    if (numberAsString.length() < LOGGER_NUMBER_DIGITS) {
        unsigned long zerosToInsert = LOGGER_NUMBER_DIGITS - numberAsString.length();
        numberAsString.insert(0, zerosToInsert, '0');
    }
    return numberAsString;
}

std::string Logger::getCurrentTime() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S");
    return ss.str();
}

void Logger::setStateCollector(std::function<std::string()> stateCollector) {
    stateQueryingFunction = std::move(stateCollector);
}

void Logger::setColorsEnabled(bool enabled) {
    colorsEnabled = enabled;
}
