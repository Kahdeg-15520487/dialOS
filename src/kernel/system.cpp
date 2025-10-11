#include "kernel/system.h"
#include <stdarg.h>

namespace dialOS {

SystemServices::SystemServices()
    : minLogLevel(LogLevel::INFO)
    , sleeping(false)
    , watchdogEnabled(false)
    , watchdogTimeout(0)
    , lastWatchdogFeed(0) {
}

SystemServices::~SystemServices() {
}

bool SystemServices::init() {
    Serial.println("System services starting...");
    lastWatchdogFeed = millis();
    return true;
}

void SystemServices::log(LogLevel level, const char* message) {
    if (level < minLogLevel) {
        return;
    }
    
    printLog(level, message);
}

void SystemServices::logf(LogLevel level, const char* format, ...) {
    if (level < minLogLevel) {
        return;
    }
    
    char buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    printLog(level, buffer);
}

void SystemServices::panic(const char* reason) {
    Serial.println("\n========== KERNEL PANIC ==========");
    Serial.print("Reason: ");
    Serial.println(reason);
    Serial.println("System halted.");
    Serial.println("==================================\n");
    
    // Halt system
    while (true) {
        delay(1000);
    }
}

void SystemServices::handleError(const char* error) {
    log(LogLevel::ERROR, error);
    // Could implement error recovery logic here
}

void SystemServices::enterSleep() {
    log(LogLevel::INFO, "Entering sleep mode");
    sleeping = true;
    // TODO: Implement actual sleep mode
}

void SystemServices::exitSleep() {
    sleeping = false;
    log(LogLevel::INFO, "Exiting sleep mode");
}

bool SystemServices::setRTC(uint32_t timestamp) {
    // TODO: Implement RTC setting via BM8563
    logf(LogLevel::INFO, "RTC set to: %lu", timestamp);
    return true;
}

uint32_t SystemServices::getRTC() const {
    // TODO: Implement RTC reading via BM8563
    // For now, return system uptime
    return millis() / 1000;
}

void SystemServices::feedWatchdog() {
    if (!watchdogEnabled) {
        return;
    }
    
    lastWatchdogFeed = millis();
}

void SystemServices::enableWatchdog(uint32_t timeoutMs) {
    watchdogEnabled = true;
    watchdogTimeout = timeoutMs;
    lastWatchdogFeed = millis();
    logf(LogLevel::INFO, "Watchdog enabled: %lu ms timeout", timeoutMs);
}

void SystemServices::disableWatchdog() {
    watchdogEnabled = false;
    log(LogLevel::INFO, "Watchdog disabled");
}

void SystemServices::printLog(LogLevel level, const char* message) {
    // Format: [TIME][LEVEL] message
    Serial.print("[");
    Serial.print(millis());
    Serial.print("][");
    Serial.print(logLevelToString(level));
    Serial.print("] ");
    Serial.println(message);
}

const char* SystemServices::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARN";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        default:                 return "UNKNOWN";
    }
}

} // namespace dialOS
