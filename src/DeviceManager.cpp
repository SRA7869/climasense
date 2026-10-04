#include "DeviceManager.h"
#include <cstddef>
#include "VirtualDevices.h"

namespace {
std::size_t slot(DeviceId id) { return static_cast<std::size_t>(id); }
}

DeviceManager::DeviceManager() {
    devices_[slot(DeviceId::FAN)]     = std::make_unique<Fan>();
    devices_[slot(DeviceId::AC)]      = std::make_unique<AirConditioner>();
    devices_[slot(DeviceId::EXHAUST)] = std::make_unique<Exhaust>();
}

void DeviceManager::apply(const DeviceCommand& cmd) {
    devices_[slot(cmd.device)]->setOn(cmd.on);
}

const IDevice& DeviceManager::device(DeviceId id) const {
    return *devices_[slot(id)];
}

std::string DeviceManager::statusLine() const {
    std::string s;
    for (const auto& d : devices_) {
        if (!s.empty()) s += " ";
        s += d->name() + ":" + (d->isOn() ? "ON" : "OFF");
    }
    return s;
}

int DeviceManager::totalSwitches() const {
    int n = 0;
    for (const auto& d : devices_) n += d->switchCount();
    return n;
}
