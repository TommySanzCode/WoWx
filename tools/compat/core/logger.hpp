#pragma once
#include <iostream>
namespace wx { template<class... T> void log(T&&... args) { (std::cerr << ... << args) << '\n'; } }
namespace wowee::core {
class Logger {
public:
    static Logger& getInstance(){static Logger l;return l;}
    template<class... T> void debug(T&&...) {}
    template<class... T> void info(T&&...) {}
    template<class... T> void warning(T&&... a){wx::log(a...);}
    template<class... T> void error(T&&... a){wx::log(a...);}
    template<class... T> void fatal(T&&... a){wx::log(a...);}
};
}
#define LOG_DEBUG(...) do {} while(0)
#define LOG_INFO(...) do {} while(0)
#define LOG_WARNING(...) wx::log(__VA_ARGS__)
#define LOG_ERROR(...) wx::log(__VA_ARGS__)
#define LOG_FATAL(...) wx::log(__VA_ARGS__)
