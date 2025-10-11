#include "Encoder.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "kernel/ramfs.h"
#include "kernel/system.h"
#include "kernel/task.h"
#include <Arduino.h>
#include <M5Dial.h>

using namespace dialOS;

// Global variables for encoder interaction
int encoderValue = 0;
bool kernelEnabled = false;

void memoryTestTask(void *param) {
  static bool initialized = false;
  static void *allocations[10] = {nullptr};
  static int allocationSizes[10] = {512, 768,  1024, 256, 2048,
                                    512, 1024, 384,  640, 896};
  static int currentStep = 0;
  static unsigned long lastActionTime = 0;
  static bool allocating = true;

  unsigned long now = millis();

  // Initialize on first run
  if (!initialized) {
    SystemServices *sys = Kernel::instance().getSystemServices();
    sys->log(LogLevel::INFO,
             "Memory Test: Starting allocation/deallocation cycle");
    initialized = true;
    lastActionTime = now;
    return;
  }

  // Perform action every 500ms
  if (now - lastActionTime < 500) {
    return;
  }
  lastActionTime = now;

  MemoryManager *mem = Kernel::instance().getMemoryManager();
  SystemServices *sys = Kernel::instance().getSystemServices();

  if (allocating) {
    // Allocation phase
    if (currentStep < 10) {
      allocations[currentStep] =
          mem->allocate(allocationSizes[currentStep], 5); // Task ID 5
      if (allocations[currentStep]) {
        sys->logf(LogLevel::INFO, "Allocated %d bytes at step %d",
                  allocationSizes[currentStep], currentStep);
      } else {
        sys->logf(LogLevel::WARNING, "Failed to allocate %d bytes at step %d",
                  allocationSizes[currentStep], currentStep);
      }
      currentStep++;
    } else {
      // Switch to deallocation phase
      allocating = false;
      currentStep = 0;
      sys->log(LogLevel::INFO, "Memory Test: Switching to deallocation phase");
    }
  } else {
    // Deallocation phase
    if (currentStep < 10) {
      if (allocations[currentStep]) {
        mem->free(allocations[currentStep], 5); // Task ID 5
        sys->logf(LogLevel::INFO, "Freed %d bytes at step %d",
                  allocationSizes[currentStep], currentStep);
        allocations[currentStep] = nullptr;
      }
      currentStep++;
    } else {
      // Reset for next cycle
      allocating = true;
      currentStep = 0;
      sys->log(LogLevel::INFO, "Memory Test: Cycle complete, restarting");
    }
  }
}

void memoryGaugeTask(void *param) {
  // Get memory stats
  MemoryManager::MemoryStats stats =
      Kernel::instance().getMemoryManager()->getStats();

  // Calculate memory usage percentage
  float usagePercent = (float)stats.usedHeap / (float)stats.totalHeap * 100.0f;

  // Screen dimensions
  const int centerX = 120;
  const int centerY = 120;
  const int radius = 119;
  const int arcWidth = 9;

  // Arc angles for bottom third (240 degrees to 300 degrees, covering 60
  // degrees) In M5GFX, angles start at 3 o'clock and go clockwise
  const int startAngle = 30;
  const int endAngle = 150;

  // Calculate fill angle based on memory usage (120 degrees total)
  int fillAngle = (int)(120.0f * (usagePercent / 100.0f));

  // Draw background arc (dark gray)
  M5Dial.Display.fillArc(centerX, centerY, radius, radius - arcWidth,
                         startAngle, endAngle, TFT_BLACK);
  M5Dial.Display.drawArc(centerX, centerY, radius, radius - arcWidth,
                         startAngle, endAngle, TFT_DARKGREY);

  // Choose color based on usage
  uint16_t fillColor;
  if (usagePercent < 50.0f) {
    fillColor = TFT_GREEN;
  } else if (usagePercent < 75.0f) {
    fillColor = TFT_YELLOW;
  } else {
    fillColor = TFT_RED;
  }

  // Draw filled arc showing usage
  if (fillAngle < endAngle) {
    M5Dial.Display.fillArc(centerX, centerY, radius, radius - arcWidth,
                           endAngle - fillAngle, endAngle, fillColor);
  }

  // Display memory stats text at bottom
  M5Dial.Display.setTextSize(1);
  M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Dial.Display.setCursor(60, 200);
  M5Dial.Display.printf("RAM: %d%% %d/%d", (int)usagePercent, stats.usedHeap,
                        stats.totalHeap);
}

void displayTask(void *param) {
  static int counter = 0;
  M5Dial.Display.setTextSize(2);
  M5Dial.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5Dial.Display.setCursor(40, 80);
  M5Dial.Display.printf("dialOS\n\tTask: %d", counter++);
}

void encoderTask(void *param) {
  int newValue = get_encoder();
  if (newValue != encoderValue) {
    encoderValue = newValue;
    Serial.print("Encoder: ");
    Serial.println(encoderValue);
  }
}

void ramfsTestTask(void *param) {
  static bool testRun = false;
  if (testRun)
    return; // Only run once

  RamFS *ramfs = Kernel::instance().getRamFS();
  SystemServices *sys = Kernel::instance().getSystemServices();

  sys->log(LogLevel::INFO, "=== RamFS Test Starting ===");

  // Test 1: Create and write a file
  const char *testFile = "test.txt";
  const char *testData = "Hello from dialOS RamFS!";
  int fd = ramfs->open(testFile, FileMode::WRITE, 0); // Task ID 0 for test

  if (fd >= 0) {
    size_t written = ramfs->write(fd, testData, strlen(testData));
    sys->logf(LogLevel::INFO, "Wrote %d bytes to %s", written, testFile);
    ramfs->close(fd);
  } else {
    sys->log(LogLevel::ERROR, "Failed to create file");
  }

  // Test 2: Read the file back
  fd = ramfs->open(testFile, FileMode::READ, 0);
  if (fd >= 0) {
    char buffer[64] = {0};
    size_t bytesRead = ramfs->read(fd, buffer, sizeof(buffer) - 1);
    sys->logf(LogLevel::INFO, "Read %d bytes: %s", bytesRead, buffer);
    ramfs->close(fd);
  } else {
    sys->log(LogLevel::ERROR, "Failed to open file for reading");
  }

  // Test 3: Get file stats
  uint32_t fileSize = ramfs->getSize(testFile);
  sys->logf(LogLevel::INFO, "File size: %d bytes", fileSize);

  // Test 4: Get filesystem stats
  RamFS::Stats stats = ramfs->getStats();
  sys->logf(LogLevel::INFO, "RamFS: %d/%d bytes used, %d/%d files",
            stats.totalSize, stats.totalSize + stats.freeSpace,
            stats.totalFiles, stats.maxFiles);

  sys->log(LogLevel::INFO, "=== RamFS Test Complete ===");
  testRun = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========== dialOS Boot ==========");

  // Initialize M5Dial hardware
  M5Dial.begin(true, true);
  M5Dial.Display.setTextSize(2);
  M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Dial.Display.fillScreen(TFT_BLACK);
  M5Dial.Display.setCursor(20, 100);
  M5Dial.Display.println("Booting...");

  // Initialize custom encoder driver
  init_encoder();

  Serial.println("M5Dial hardware initialized");

  // Initialize kernel
  if (!Kernel::instance().init()) {
    Serial.println("FATAL: Kernel initialization failed!");
    M5Dial.Display.fillScreen(TFT_RED);
    M5Dial.Display.setCursor(20, 100);
    M5Dial.Display.println("KERNEL\nFAIL!");
    while (true)
      delay(1000);
  }

  Serial.println("Kernel initialized successfully");
  kernelEnabled = true;

  // Create kernel tasks
  TaskScheduler *scheduler = Kernel::instance().getScheduler();

  Task *task1 = scheduler->createTask("Display", displayTask, nullptr, 2048,
                                      TaskPriority::NORMAL);
  Task *task2 = scheduler->createTask("Encoder", encoderTask, nullptr, 2048,
                                      TaskPriority::HIGH_PRIORITY);
  Task *task3 = scheduler->createTask("RamFS_Test", ramfsTestTask, nullptr,
                                      4096, TaskPriority::NORMAL);
  Task *task4 = scheduler->createTask("MemoryGauge", memoryGaugeTask, nullptr,
                                      2048, TaskPriority::LOW_PRIORITY);
  Task *task5 = scheduler->createTask("MemoryTest", memoryTestTask, nullptr,
                                      2048, TaskPriority::NORMAL);

  if (task1 && task2 && task3) {
    Serial.println("Kernel tasks created");
  }

  // Print memory stats
  MemoryManager::MemoryStats stats =
      Kernel::instance().getMemoryManager()->getStats();
  Serial.print("Memory: ");
  Serial.print(stats.usedHeap);
  Serial.print(" / ");
  Serial.print(stats.totalHeap);
  Serial.println(" bytes used");

  // Initialize encoder value

  encoderValue = get_encoder();

  M5Dial.Display.fillScreen(TFT_BLACK);
  M5Dial.Display.setCursor(20, 100);
  M5Dial.Display.println("dialOS\nReady!");
  delay(1000);
  M5Dial.Display.fillScreen(TFT_BLACK);

  Serial.println("=================================\n");
}

void loop() {
  // Update M5Dial state
  M5Dial.update();

  if (kernelEnabled) {
    // Run kernel scheduler
    Kernel::instance().getScheduler()->schedule();
  } else {
    // Fallback: simple display update
    int newEncoderValue = get_encoder();
    if (newEncoderValue != encoderValue) {
      encoderValue = newEncoderValue;
      M5Dial.Display.fillScreen(TFT_BLACK);
      M5Dial.Display.setCursor(20, 80);
      M5Dial.Display.printf("dialOS\nEncoder: %d", encoderValue);
    }
  }

  delay(10);
}