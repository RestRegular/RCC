//
// Created by RestRegular on 2025/7/14.
//

#ifndef RLOG_SYSTEM_H
#define RLOG_SYSTEM_H

#include <iostream>
#include <unordered_set>

namespace rlog {

    enum LogMode: int;
    enum LogRole: int;

    std::string getLogRoleString(const LogRole &role);

    class LogManager {
        static std::unordered_set<LogMode> logMode;
    public:
        static void setLogMode(const std::unordered_set<LogMode> &mode);
        static void rlog(const std::string &message, const LogRole &role=static_cast<LogRole>(0));
        static void rlog(const std::string &message, const std::string &role);
        static void rwarn(const std::string &message, const LogRole &role=static_cast<LogRole>(0));
        static void rwarn(const std::string &message, const std::string &role);
        static void rerror(const std::string &message, const LogRole &role=static_cast<LogRole>(0));
        static void rerror(const std::string &message, const std::string &role);
    };


    enum LogMode: int {
        NONE = 0,
        MESSAGE,
        WARNING,
        ERROR,
        ALL
    };

    enum LogRole: int {
        SYSTEM = 0,
        USER,
        UNKNOWN,
    };

}

#endif //RLOG_SYSTEM_H
