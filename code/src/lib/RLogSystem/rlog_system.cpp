//
// Created by RestRegular on 2025/7/14.
//

#include "../../../include/lib/RLogSystem/rlog_system.h"
#include "../../../include/lib/newrcc.h"

namespace rlog {
    std::string getLogRoleString(const LogRole &role) {
        switch (role) {
            case UNKNOWN: return "UNKNOWN";
            case SYSTEM: return "SYSTEM";
            default: return "UNKNOWN";
        }
    }

    std::unordered_set<LogMode> LogManager::logMode = {ALL};

    void LogManager::setLogMode(const std::unordered_set<LogMode> &mode) {
        logMode = mode;
    }

    void LogManager::rlog(const std::string &message, const LogRole &role) {
        if (logMode.contains(NONE) || (!logMode.contains(ALL) && !logMode.contains(MESSAGE))) return;
        std::cout << cc::colorfulText("[RLog(" + getLogRoleString(role) + ")]: ",
                                      cc::TextColor::LIGHT_GREEN) << message << std::endl;
    }

    void LogManager::rlog(const std::string &message, const std::string &role) {
        if (logMode.contains(NONE) || (!logMode.contains(ALL) && !logMode.contains(MESSAGE))) return;
        std::cout << cc::colorfulText("[RLog(" + role + ")]: ", cc::TextColor::LIGHT_GREEN)
        << message << std::endl;
    }

    void LogManager::rwarn(const std::string &message, const LogRole &role) {
        if (logMode.contains(NONE) || (!logMode.contains(ALL) && !logMode.contains(WARNING))) return;
        std::cout << cc::colorfulText("[RLog(" + getLogRoleString(role) + ")]: ",
            cc::TextColor(255, 165, 0)) << message << std::endl;
    }

    void LogManager::rwarn(const std::string &message, const std::string &role) {
        if (logMode.contains(NONE) || (!logMode.contains(ALL) && !logMode.contains(WARNING))) return;
        std::cout << cc::colorfulText("[RLog(" + role + ")]: ",
            cc::TextColor(255, 165, 0)) << message << std::endl;
    }

    void LogManager::rerror(const std::string &message, const LogRole &role) {
        if (logMode.contains(NONE) || (!logMode.contains(ALL) && !logMode.contains(ERROR_LM))) return;
        std::cerr << cc::colorfulText("[RLog(" + getLogRoleString(role) + ")]: ",
            cc::TextColor(255, 0, 0)) << message << std::endl;
    }

    void LogManager::rerror(const std::string &message, const std::string &role) {
        if (logMode.contains(NONE) || (!logMode.contains(ALL) && !logMode.contains(ERROR_LM))) return;
        std::cerr << cc::colorfulText("[RLog(" + role + ")]: ",
            cc::TextColor(255, 0, 0)) << message << std::endl;
    }
}
