#include <communication/MpiOptimizedCommunicator.h>
#include <communication/CommunicationManager.h>
#include <processes/Process.h>

int main(int argc, char** argv) {
    auto communicator = std::make_shared<MpiOptimizedCommunicator>(argc, argv);
    Logger::init(communicator);
    Logger::registerThread("Main", rang::fg::cyan);
    Logger::setColorsEnabled(true);

    auto communicationManager= std::make_shared<CommunicationManager>(communicator);

    Process process(communicationManager);
    communicationManager->listen();

    process.run();
}
