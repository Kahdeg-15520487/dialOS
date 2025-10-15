/**
 * dialScript Platform Interface Implementation
 *
 * Implements callback system for event-driven programming
 */

#include "platform.h"
#include "vm_value.h"
#include "vm_core.h"
#include <map>

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
                callbacks_ = std::make_unique<CallbackRegistry>();
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
            // Check if VM is initialized
            if (vm_ == nullptr)
            {
                return false;
            }

            // Get the callback
            const Value* callback = getCallback(eventName);
            if (callback == nullptr || !callback->isFunction())
            {
                return false;
            }

            // Use VM's invokeFunction method for immediate callback execution
            return vm_->invokeFunction(*callback, args);
        }

    } // namespace vm
} // namespace dialos
