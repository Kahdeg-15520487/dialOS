/**
 * dialScript Platform Interface Implementation
 *
 * Implements callback system for event-driven programming
 */

#include "platform.h"
#include "vm_value.h"
#include "vm_core.h"
#include <map>
#include <sstream>
#include <iomanip>

namespace dialos
{
    namespace vm
    {
        // Define the callback registry structure
        struct PlatformInterface::CallbackRegistry {
            std::map<std::string, Value> callbacks;
        };

        // Constructor
        PlatformInterface::PlatformInterface() = default;

        // Destructor (must be defined in .cpp where CallbackRegistry is complete)
        PlatformInterface::~PlatformInterface() = default;

        void PlatformInterface::registerCallback(const std::string& eventName, const Value& callback)
        {
            // Lazy initialization of callback registry
            if (!callbacks_) {
                callbacks_ = std::unique_ptr<CallbackRegistry>(new CallbackRegistry());
            }
            
            // Store the callback in the registry
            // Note: We're copying the Value, which is safe because ValuePool manages the underlying objects
            callbacks_->callbacks[eventName] = callback;
        }

        const Value* PlatformInterface::getCallback(const std::string& eventName) const
        {
            if (!callbacks_) {
                return nullptr;
            }
            
            auto it = callbacks_->callbacks.find(eventName);
            if (it != callbacks_->callbacks.end())
            {
                return &it->second;
            }
            return nullptr;
        }

        bool PlatformInterface::invokeCallback(const std::string& eventName, const std::vector<Value>& args)
        {
            // console_log("[DEBUG] Event occurred: " + eventName);
            
            // Check if VM is initialized and still running
            if (vm_ == nullptr || !vm_->isRunning())
            {
                // console_log("[DEBUG] VM not running for event: " + eventName);
                return false;
            }

            // Get the callback
            const Value* callback = getCallback(eventName);
            if (callback == nullptr || !callback->isFunction())
            {
                // console_log("[DEBUG] No callback registered for event: " + eventName);
                return false;
            }

            // console_log("[DEBUG] Invoking callback for event: " + eventName);
            
            // Use VM's invokeFunction method for immediate callback execution
            bool success = vm_->invokeFunction(*callback, args);
            
            if (!success) {
                console_log("[VM] Callback invocation failed for event: " + eventName);
                // Also log any VM error details immediately
                if (vm_->hasError()) {
                    console_log("[VM] VM Error during callback: " + vm_->getError());
                } else {
                    console_log("[VM] No VM error reported - callback failed for unknown reason");
                }
                // Check VM state
                console_log("[VM] VM running state: " + std::string(vm_->isRunning() ? "true" : "false"));
            } else {
                // console_log("[DEBUG] Callback succeeded for event: " + eventName);
            }
            
            return success;
        }

        void PlatformInterface::dumpVMState(const VMState &vm, size_t pc, const std::string &reason)
        {
            std::stringstream ss;
            ss << "[PLATFORM-VM-DUMP] Reason: " << reason << " PC=" << pc << "\n";
            ss << "[PLATFORM-VM-DUMP] Stack size=" << vm.getStackSize() << "\n";

            // Attempt to print globals
            ss << "[PLATFORM-VM-DUMP] Globals:\n";
            const auto &globals = vm.getGlobals();
            for (const auto &p : globals) {
                try { ss << "  " << p.first << " = " << p.second.toString() << "\n"; } catch (...) { ss << "  " << p.first << " = <unprintable>\n"; }
            }

            // Print call frames
            ss << "[PLATFORM-VM-DUMP] CallStack depth=" << vm.getCallStack().size() << "\n";
            const auto &callstack = vm.getCallStack();
            for (size_t i = 0; i < callstack.size(); ++i) {
                const auto &cf = callstack[i];
                ss << "  Frame[" << i << "] func=" << cf.functionName << " stackBase=" << cf.stackBase << " locals={";
                bool first = true;
                for (const auto &loc : cf.locals) {
                    if (!first) ss << ", "; first = false;
                    try { ss << (int)loc.first << ":" << loc.second.toString(); } catch (...) { ss << (int)loc.first << ":<unprintable>"; }
                }
                ss << "}\n";
            }

            console_log(ss.str());

            // If debug info exists, attempt to print source context by searching common script paths
            // Note: VMState does not expose module directly; we rely on debug info being reachable via pc mapping (best-effort)
            // For now we only print the minimal dump above; platform-specific implementations (SDLPlatform) can extend this.
        }

        void PlatformInterface::printRuntimeError(const VMState &vm, const std::string &bytecodePath, size_t pc, const std::string &errorMessage, uint32_t sourceLine)
        {
            // Default behavior: basic console output plus VM dump
            console_error("Runtime Error: " + errorMessage);
            console_log(std::string("PC: ") + std::to_string(pc) + ", Stack: " + std::to_string(vm.getStackSize()));
            // Platform-specific implementations (like SDLPlatform) may override to open files and print context
        }

    } // namespace vm
} // namespace dialos
