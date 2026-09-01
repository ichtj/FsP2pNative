#include "IotConnectionState.h"

#include <iostream>
#include <string>

namespace {

bool expect(bool condition, const char* message) {
    if (condition) return true;
    std::cerr << message << '\n';
    return false;
}

bool failedStateIsAvailableForReplay() {
    IotConnectionStateStore state;
    const auto update = state.updateFromSignal(
            "iot_connect_state", "0", "Connection refused");
    const IotConnectionSnapshot snapshot = state.snapshot();

    return expect(update.has_value(), "iot_connect_state was not recognized") &&
           expect(snapshot.status == IotConnectionStatus::Disconnected,
                  "failed IOT state was not stored") &&
           expect(!snapshot.connected(), "failed IOT state reported connected") &&
           expect(snapshot.needsRecovery(),
                  "failed IOT state did not request recovery") &&
           expect(snapshot.known(), "failed IOT state reported unknown") &&
           expect(snapshot.description == "Connection refused",
                  "failed IOT description was not retained") &&
           expect(snapshot.replayDescription() == "Connection refused",
                  "failed IOT description was not available for replay");
}

bool broadcastConnectRecoversFailedState() {
    IotConnectionStateStore state;
    state.updateFromSignal("iot_connect_state", "0", "Connection refused");

    const auto update = state.updateFromSignal("iot_connect", "", "");
    const IotConnectionSnapshot snapshot = state.snapshot();

    return expect(update.has_value(), "iot_connect was not recognized") &&
           expect(snapshot.status == IotConnectionStatus::Connected,
                  "iot_connect did not recover the IOT state") &&
           expect(snapshot.connected(), "recovered IOT state reported disconnected") &&
           expect(!snapshot.needsRecovery(),
                  "connected IOT state still requested recovery") &&
           expect(snapshot.description.empty(),
                  "iot_connect retained the previous failure description") &&
           expect(snapshot.replayDescription() == "Connected",
                  "connected state did not provide a replay description");
}

bool explicitConnectedStateIsRecognized() {
    IotConnectionStateStore state;
    const auto update = state.updateFromSignal(
            "iot_connect_state", "1", "cloud connected");
    const IotConnectionSnapshot snapshot = state.snapshot();

    return expect(update.has_value(), "connected IOT state was not recognized") &&
           expect(snapshot.connected(), "state=1 did not report connected") &&
           expect(snapshot.description == "cloud connected",
                  "connected IOT description was not retained");
}

bool disconnectAndUnknownSignalsAreHandledSafely() {
    IotConnectionStateStore state;
    state.updateFromSignal("iot_connect", "", "");
    const auto disconnect = state.updateFromSignal(
            "iot_disconnect", "", "transport stopped");
    const IotConnectionSnapshot disconnected = state.snapshot();
    const auto ignored = state.updateFromSignal("Heartbeat", "1", "ignored");
    const IotConnectionSnapshot afterIgnored = state.snapshot();

    return expect(disconnect.has_value(), "iot_disconnect was not recognized") &&
           expect(!disconnected.connected(), "iot_disconnect reported connected") &&
           expect(disconnected.description == "transport stopped",
                  "iot_disconnect description was not retained") &&
           expect(!ignored.has_value(), "unrelated signal changed IOT state") &&
           expect(afterIgnored.status == disconnected.status &&
                          afterIgnored.description == disconnected.description,
                  "unrelated signal mutated the IOT snapshot");
}

bool resetReturnsStateToUnknown() {
    IotConnectionStateStore state;
    state.updateFromSignal("iot_connect", "", "");
    state.reset();
    const IotConnectionSnapshot snapshot = state.snapshot();

    return expect(!snapshot.known(), "reset state was still known") &&
           expect(!snapshot.needsRecovery(), "unknown IOT state requested recovery") &&
           expect(snapshot.value() == 0, "reset state did not use the unknown value") &&
           expect(snapshot.description.empty(), "reset state retained a description");
}

} // namespace

int main() {
    if (!failedStateIsAvailableForReplay()) return 1;
    if (!broadcastConnectRecoversFailedState()) return 2;
    if (!explicitConnectedStateIsRecognized()) return 3;
    if (!disconnectAndUnknownSignalsAreHandledSafely()) return 4;
    if (!resetReturnsStateToUnknown()) return 5;

    std::cout << "IOT connection state tests passed\n";
    return 0;
}
