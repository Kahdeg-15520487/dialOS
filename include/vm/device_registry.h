#ifndef DIALOS_DEVICE_REGISTRY_H
#define DIALOS_DEVICE_REGISTRY_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace dialos {
namespace vm {

/**
 * Device bus types.
 */
enum class BusType {
    I2C,
    SPI,
    VIRTUAL   // simulated devices (emulators)
};

/**
 * Static descriptor for a device driver.
 * A driver declares which bus(es)/addresses it can probe and how to
 * identify hardware. Native drivers register a ProbeFn; userspace
 * (dialScript) drivers register descriptors only and drive the bus
 * themselves via os.i2c.* / os.spi.*.
 */
struct DeviceDescriptor {
    std::string name;          // Unique driver name, e.g. "bmp280"
    BusType bus;
    uint8_t address;           // I2C 7-bit address, or SPI CS "address", 0 for VIRTUAL
    std::string capabilities;  // JSON object, e.g. {"type":"sensor","provides":["tempC","pa"]}
    bool hotplug;              // true if device may appear/disappear at runtime
};

/**
 * A probed (present) device instance discovered on a bus.
 */
struct DeviceInstance {
    DeviceDescriptor desc;
    bool present;
};

/**
 * An open device handle with task ownership (mirrors RamFS handle pattern).
 */
struct DeviceHandle {
    std::string name;
    BusType bus;
    uint8_t address;
    uint32_t taskId;      // Owner task; 0 = system
    bool isOpen;
};

/**
 * Probe function: return true if the hardware responding at `address`
 * matches this driver (e.g. check a WHO_AM_I / signature register).
 * Implemented natively per-platform; receives a caller-defined context.
 */
using ProbeFn = bool (*)(void *busContext, uint8_t address);

/**
 * DeviceRegistry - central device/driver table.
 *
 * Platforms embed one registry, register native drivers at init time,
 * and call scan() to populate the discovered-device list. The registry
 * itself performs NO bus I/O; probing is delegated to registered ProbeFn
 * callbacks so the same code works on SDL (simulated) and ESP32 (Wire).
 *
 * Threading: NOT thread-safe. Platforms must serialize access (the VM
 * currently executes cooperatively; ESP32 should guard with a mutex).
 */
class DeviceRegistry {
  public:
    DeviceRegistry() = default;
    ~DeviceRegistry() = default;

    // ===== Driver registration (init time) =====

    // Register a driver descriptor with an optional hardware probe.
    // probe may be nullptr for userspace-only drivers (open-by-name still works).
    bool registerDriver(const DeviceDescriptor &desc, ProbeFn probe);

    // ===== Discovery =====

    // Run all registered probes against busContext (platform-owned, e.g. TwoWire*).
    // Returns number of devices found. Updates the discovered list.
    size_t scan(void *busContext);

    // Manual: mark a descriptor present/absent (used by simulators & hotplug).
    void setPresent(const std::string &name, bool present);

    // JSON array: [{"name":..,"bus":"i2c","address":118,"capabilities":{..}}, ...]
    std::string listDiscoveredJson() const;

    // ===== Handles =====

    // Open by driver name ("bmp280") or by address ("i2c:0x76" / "spi:5").
    // Returns handle id (>=0) or -1. Ownership recorded for taskId.
    int open(const std::string &nameOrAddress, uint32_t taskId);

    // Close a handle; verifies ownership. Returns false if not owner/not open.
    bool close(int handle, uint32_t taskId);

    // JSON object descriptor for an open handle, or "{}".
    std::string getInfoJson(int handle) const;

    // Resolve a handle to its descriptor. Returns nullptr if invalid.
    const DeviceInstance *resolve(int handle) const;

    // Release all handles owned by a task (call on task exit, like RamFS).
    void releaseAllForTask(uint32_t taskId);

    size_t openHandleCount() const { return openCount_; }

  private:
    static const size_t MAX_HANDLES = 16;

    int allocateHandle();

    std::vector<DeviceInstance> discovered_;
    std::map<std::string, ProbeFn> probes_;     // driver name -> probe fn
    std::map<std::string, DeviceDescriptor> drivers_;
    DeviceHandle handles_[MAX_HANDLES];
    size_t openCount_ = 0;
};

// BusType -> string ("i2c" | "spi" | "virtual")
const char *busTypeToString(BusType t);
// String -> BusType ("i2c" | "spi" | "virtual"), defaults to VIRTUAL
BusType busTypeFromString(const std::string &s);

} // namespace vm
} // namespace dialos

#endif // DIALOS_DEVICE_REGISTRY_H
