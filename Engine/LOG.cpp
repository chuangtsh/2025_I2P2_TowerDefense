#include "LOG.hpp"
#include <fstream>
#include <iostream>

namespace Engine {
    // bool LOG::Enabled = false;
    bool LOG::Enabled = true;
    // bool LOG::LogVerbose = false;
    bool LOG::LogVerbose = true;
    const char *LOG::FilePath = "log.txt";

    LOG::LOG(LogType type) {
        enabled = Enabled;
        this->type = type;
        if (canLog()) {
            ofs.open(FilePath, std::ofstream::app);
            std::cout << '[' << getLabel(type) << "] ";
            ofs << '[' << getLabel(type) << "] ";
        }
    }

    LOG::~LOG() {
        try {
            if (canLog()) {
                std::cout << std::endl;
                ofs << std::endl;
                ofs.close();
            }
        } catch (...) {}
    }

    bool LOG::canLog() const {
        return enabled && (type != VERBOSE || LogVerbose);
    }

    const char *LOG::getLabel(LogType type) {
        switch (type) {
            case VERBOSE: return "VERBOSE";
            case DEBUGGING: return "DEBUGGING";
            case INFO: return "INFO";
            case WARN: return "WARN";
            case ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }
    void LOG::SetConfig(bool enabled, bool logVerbose, const char *filePath) {
        Enabled = enabled;
        LogVerbose = logVerbose;
        FilePath = filePath;
        // Clear log file content.
        std::ofstream ofs(FilePath, std::ofstream::out);
    }
}
