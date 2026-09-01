#include "IotConnectionState.h"

bool IotConnectionSnapshot::known() const {
    return status != IotConnectionStatus::Unknown;
}

bool IotConnectionSnapshot::connected() const {
    return status == IotConnectionStatus::Connected;
}

bool IotConnectionSnapshot::needsRecovery() const {
    return status == IotConnectionStatus::Disconnected;
}

int IotConnectionSnapshot::value() const {
    return static_cast<int>(status);
}

std::string IotConnectionSnapshot::replayDescription() const {
    if (!description.empty()) return description;
    return connected() ? "Connected" : "Disconnected";
}

std::optional<IotConnectionSnapshot> IotConnectionStateStore::updateFromSignal(
        const std::string& signal,
        const std::string& state,
        const std::string& description) {
    if (signal == "iot_connect_state") {
        return update(state == "1"
                      ? IotConnectionStatus::Connected
                      : IotConnectionStatus::Disconnected,
                      description);
    }
    if (signal == "iot_connect") {
        return update(IotConnectionStatus::Connected, description);
    }
    if (signal == "iot_disconnect") {
        return update(IotConnectionStatus::Disconnected, description);
    }
    return std::nullopt;
}

IotConnectionSnapshot IotConnectionStateStore::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex);
    return current;
}

void IotConnectionStateStore::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    current = {};
}

void IotConnectionStateStore::markDisconnected(const std::string& description) {
    update(IotConnectionStatus::Disconnected, description);
}

IotConnectionSnapshot IotConnectionStateStore::update(
        IotConnectionStatus status,
        const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex);
    current.status = status;
    current.description = description;
    return current;
}
