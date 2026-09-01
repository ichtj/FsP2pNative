#ifndef IOT_CONNECTION_STATE_H
#define IOT_CONNECTION_STATE_H

#include <mutex>
#include <optional>
#include <string>

enum class IotConnectionStatus : int {
    Disconnected = -1,
    Unknown = 0,
    Connected = 1,
};

struct IotConnectionSnapshot {
    IotConnectionStatus status = IotConnectionStatus::Unknown;
    std::string description;

    bool known() const;
    bool connected() const;
    bool needsRecovery() const;
    int value() const;
    std::string replayDescription() const;
};

class IotConnectionStateStore {
public:
    std::optional<IotConnectionSnapshot> updateFromSignal(
            const std::string& signal,
            const std::string& state,
            const std::string& description);

    IotConnectionSnapshot snapshot() const;
    void reset();
    void markDisconnected(const std::string& description = std::string());

private:
    IotConnectionSnapshot update(
            IotConnectionStatus status,
            const std::string& description);

    mutable std::mutex mutex;
    IotConnectionSnapshot current;
};

#endif // IOT_CONNECTION_STATE_H
