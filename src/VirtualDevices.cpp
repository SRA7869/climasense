#include "VirtualDevices.h"
#include <utility>

VirtualDevice::VirtualDevice(std::string name, double watts)
    : name_(std::move(name)), watts_(watts) {}

void VirtualDevice::setOn(bool on) {
    if (on == on_) return;
    const auto now = std::chrono::steady_clock::now();
    if (on) {
        onSince_ = now;
    } else {
        accumulated_ += std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - onSince_);
    }
    on_ = on;
    ++switches_;
}

std::chrono::milliseconds VirtualDevice::onTime() const {
    if (!on_) return accumulated_;
    return accumulated_ + std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - onSince_);
}
