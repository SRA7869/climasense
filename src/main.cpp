#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include "Config.h"
#include "Controller.h"
#include "Dashboard.h"
#include "FaultySensor.h"
#include "HumiditySensor.h"
#include "TemperatureSensor.h"

using namespace std::chrono_literals;

static std::string describe(const SystemConfig& c) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1)
       << "temp_high=" << c.comfort.tempHigh
       << " temp_very_high=" << c.comfort.tempVeryHigh
       << " humidity_high=" << c.comfort.humidityHigh
       << " hysteresis=" << c.comfort.hysteresis
       << " fan_runs_with_ac=" << (c.automation.fanRunsWithAc ? "true" : "false");
    return os.str();
}

static void printDeviceSummary(const DeviceManager& devices) {
    std::cout << "\nDevice summary\n";
    std::cout << std::left << std::setw(10) << "Device" << std::setw(7) << "State"
              << std::setw(10) << "Switches" << std::setw(12) << "On-time (s)"
              << "Energy (Wh)\n";
    double totalWh = 0.0;
    for (DeviceId id : {DeviceId::FAN, DeviceId::AC, DeviceId::EXHAUST}) {
        const IDevice& d = devices.device(id);
        const double sec = static_cast<double>(d.onTime().count()) / 1000.0;
        const double wh = d.ratedWatts() * sec / 3600.0;
        totalWh += wh;
        std::cout << std::setw(10) << d.name()
                  << std::setw(7) << (d.isOn() ? "ON" : "OFF")
                  << std::setw(10) << d.switchCount()
                  << std::setw(12) << sec << wh << "\n";
    }
    std::cout << std::right << "Total energy: " << totalWh << " Wh\n";
}

int main() {
    std::cout << std::fixed << std::setprecision(2);

    SystemConfig config;
    const ConfigReport report = loadConfig("climasense.conf", config);

    Controller controller(config, "climasense.log");
    Dashboard dashboard(controller);

    std::cout << "using config: " << describe(config) << "\n";
    for (const std::string& p : report.problems) std::cout << "config problem: " << p << "\n";
    if (!controller.logging()) std::cout << "warning: could not open climasense.log\n";
    if (!dashboard.live()) std::cout << "(output is not a terminal: final frame only)\n";
    controller.note("config: " + describe(config));
    if (!report.fileFound) controller.note("config file not found, using defaults");
    for (const std::string& p : report.problems) controller.note("config problem: " + p);

    auto tempInner = std::make_unique<TemperatureSensor>(25.0);
    auto humInner  = std::make_unique<HumiditySensor>(50.0);
    TemperatureSensor* room = tempInner.get();
    HumiditySensor*    air  = humInner.get();
    auto tempFaulty = std::make_unique<FaultySensor>(std::move(tempInner));
    FaultySensor* tempFault = tempFaulty.get();
    controller.addSensor(std::move(tempFaulty));
    controller.addSensor(std::make_unique<FaultySensor>(std::move(humInner)));

    room->setTarget(36.0);
    air->setTarget(90.0);

    controller.start();
    dashboard.start();

    std::this_thread::sleep_for(4s);
    controller.mark(">>> injecting fault: temperature OUT_OF_RANGE");
    tempFault->setFault(Fault::OUT_OF_RANGE);
    std::this_thread::sleep_for(2s);
    controller.mark(">>> clearing fault");
    tempFault->setFault(Fault::NONE);
    std::this_thread::sleep_for(1s);
    controller.mark(">>> room conditions returning to normal");
    room->setTarget(24.0);
    air->setTarget(45.0);
    std::this_thread::sleep_for(5s);

    controller.stop();
    dashboard.stop();
    dashboard.showFinal();

    std::cout << "Samples processed: " << controller.processed() << "\n";
    std::cout << "Total transitions: " << controller.machine().transitionCount() << "\n";
    if (controller.logging()) std::cout << "Log written to climasense.log\n";
    printDeviceSummary(controller.devices());
    return 0;
}
