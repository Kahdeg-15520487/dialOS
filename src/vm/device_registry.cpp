#include "vm/device_registry.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace dialos {
namespace vm {

const char *busTypeToString(BusType t) {
    switch (t) {
    case BusType::I2C: return "i2c";
    case BusType::SPI: return "spi";
    case BusType::VIRTUAL: return "virtual";
    }
    return "virtual";
}

BusType busTypeFromString(const std::string &s) {
    if (s == "i2c") return BusType::I2C;
    if (s == "spi") return BusType::SPI;
    return BusType::VIRTUAL;
}

bool DeviceRegistry::registerDriver(const DeviceDescriptor &desc, ProbeFn probe) {
    if (desc.name.empty()) return false;
    drivers_[desc.name] = desc;
    probes_[desc.name] = probe;

    // Pre-seed the discovered list (absent until probed or setPresent).
    for (auto &d : discovered_) {
        if (d.desc.name == desc.name) return true; // already known
    }
    DeviceInstance inst;
    inst.desc = desc;
    inst.present = false;
    discovered_.push_back(inst);
    return true;
}

size_t DeviceRegistry::scan(void *busContext) {
    size_t found = 0;
    for (auto &inst : discovered_) {
        auto it = probes_.find(inst.desc.name);
        bool present = false;
        if (it != probes_.end() && it->second != nullptr) {
            present = it->second(busContext, inst.desc.address);
        }
        inst.present = present;
        if (present) found++;
    }
    return found;
}

void DeviceRegistry::setPresent(const std::string &name, bool present) {
    for (auto &inst : discovered_) {
        if (inst.desc.name == name) {
            inst.present = present;
            return;
        }
    }
}

static std::string addrToString(uint8_t addr, BusType bus) {
    char buf[16];
    if (bus == BusType::I2C)
        std::snprintf(buf, sizeof(buf), "0x%02X", addr);
    else
        std::snprintf(buf, sizeof(buf), "%u", (unsigned)addr);
    return buf;
}

std::string DeviceRegistry::listDiscoveredJson() const {
    std::string out = "[";
    bool first = true;
    for (const auto &inst : discovered_) {
        if (!inst.present) continue;
        if (!first) out += ",";
        first = false;
        out += "{\"name\":\"" + inst.desc.name + "\"";
        out += ",\"bus\":\"" + std::string(busTypeToString(inst.desc.bus)) + "\"";
        out += ",\"address\":\"" + addrToString(inst.desc.address, inst.desc.bus) + "\"";
        if (!inst.desc.capabilities.empty())
            out += ",\"capabilities\":" + inst.desc.capabilities;
        out += "}";
    }
    out += "]";
    return out;
}

int DeviceRegistry::allocateHandle() {
    for (size_t i = 0; i < MAX_HANDLES; i++) {
        if (!handles_[i].isOpen) return (int)i;
    }
    return -1;
}

int DeviceRegistry::open(const std::string &nameOrAddress, uint32_t taskId) {
    std::string driverName;

    if (nameOrAddress.rfind("i2c:", 0) == 0 || nameOrAddress.rfind("spi:", 0) == 0) {
        // Address form: "i2c:0x76" / "spi:5"
        BusType bus = busTypeFromString(nameOrAddress.substr(0, nameOrAddress.find(':')));
        std::string addrStr = nameOrAddress.substr(nameOrAddress.find(':') + 1);
        uint8_t addr = 0;
        if (addrStr.rfind("0x", 0) == 0 || addrStr.rfind("0X", 0) == 0) {
            addr = (uint8_t)std::strtoul(addrStr.c_str(), nullptr, 16);
        } else {
            addr = (uint8_t)std::strtoul(addrStr.c_str(), nullptr, 10);
        }
        for (const auto &inst : discovered_) {
            if (inst.present && inst.desc.bus == bus && inst.desc.address == addr) {
                driverName = inst.desc.name;
                break;
            }
        }
        if (driverName.empty()) return -1;
    } else {
        // Name form: must be a discovered (present) driver.
        for (const auto &inst : discovered_) {
            if (inst.present && inst.desc.name == nameOrAddress) {
                driverName = nameOrAddress;
                break;
            }
        }
        if (driverName.empty()) return -1;
    }

    // One open handle per device at a time.
    for (size_t i = 0; i < MAX_HANDLES; i++) {
        if (handles_[i].isOpen && handles_[i].name == driverName) return -1;
    }

    int h = allocateHandle();
    if (h < 0) return -1;

    const DeviceDescriptor &d = drivers_[driverName];
    handles_[h].name = driverName;
    handles_[h].bus = d.bus;
    handles_[h].address = d.address;
    handles_[h].taskId = taskId;
    handles_[h].isOpen = true;
    openCount_++;
    return h;
}

bool DeviceRegistry::close(int handle, uint32_t taskId) {
    if (handle < 0 || handle >= (int)MAX_HANDLES) return false;
    DeviceHandle &h = handles_[handle];
    if (!h.isOpen) return false;
    if (h.taskId != taskId) return false; // ownership check (mirrors RamFS)
    h.isOpen = false;
    h.name.clear();
    openCount_--;
    return true;
}

std::string DeviceRegistry::getInfoJson(int handle) const {
    if (handle < 0 || handle >= (int)MAX_HANDLES) return "{}";
    const DeviceHandle &h = handles_[handle];
    if (!h.isOpen) return "{}";

    auto it = drivers_.find(h.name);
    if (it == drivers_.end()) return "{}";

    const DeviceDescriptor &d = it->second;
    std::string out = "{\"name\":\"" + d.name + "\"";
    out += ",\"bus\":\"" + std::string(busTypeToString(d.bus)) + "\"";
    out += ",\"address\":\"" + addrToString(d.address, d.bus) + "\"";
    if (!d.capabilities.empty())
        out += ",\"capabilities\":" + d.capabilities;
    out += "}";
    return out;
}

const DeviceInstance *DeviceRegistry::resolve(int handle) const {
    if (handle < 0 || handle >= (int)MAX_HANDLES) return nullptr;
    if (!handles_[handle].isOpen) return nullptr;
    auto it = drivers_.find(handles_[handle].name);
    if (it == drivers_.end()) return nullptr;
    for (const auto &inst : discovered_) {
        if (inst.desc.name == handles_[handle].name) return &inst;
    }
    return nullptr;
}

void DeviceRegistry::releaseAllForTask(uint32_t taskId) {
    for (size_t i = 0; i < MAX_HANDLES; i++) {
        if (handles_[i].isOpen && handles_[i].taskId == taskId) {
            handles_[i].isOpen = false;
            handles_[i].name.clear();
            openCount_--;
        }
    }
}

} // namespace vm
} // namespace dialos
