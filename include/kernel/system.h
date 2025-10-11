#ifndef DIALOS_SYSTEM_H
#define DIALOS_SYSTEM_H

#include <Arduino.h>

namespace dialOS {

/**
 * @brief Log levels for system logging
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

/**
 * @brief System services - logging, error handling, power management
 */
class SystemServices {
public:
    SystemServices();
    ~SystemServices();
    
    bool init();
    
    // Logging
    void log(LogLevel level, const char* message);
    void logf(LogLevel level, const char* format, ...);
    void setLogLevel(LogLevel level) { minLogLevel = level; }
    
    // Error handling
    void panic(const char* reason);  // Unrecoverable error
    void handleError(const char* error);
    
    // System state
    void enterSleep();
    void exitSleep();
    bool isSleeping() const { return sleeping; }
    
    // RTC management
    bool setRTC(uint32_t timestamp);
    uint32_t getRTC() const;
    
    // Watchdog
    void feedWatchdog();
    void enableWatchdog(uint32_t timeoutMs);
    void disableWatchdog();
    
private:
    static const size_t LOG_BUFFER_SIZE = 256;
    
    LogLevel minLogLevel;
    bool sleeping;
    bool watchdogEnabled;
    uint32_t watchdogTimeout;
    uint32_t lastWatchdogFeed;
    
    void printLog(LogLevel level, const char* message);
    const char* logLevelToString(LogLevel level);
};

} // namespace dialOS

#endif // DIALOS_SYSTEM_H
