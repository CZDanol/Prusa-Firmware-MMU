/// @file hw_sanity.h
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "config/axis.h"

namespace logic {

/// @brief Performs a sanity check of the hardware at reset/boot. Checks the following:
/// - TMC drivers using their IOIN registers (DIR/STEP/DRV_EN)
/// - ...
/// - Profit!

class HWSanity : public CommandBase {
public:
    inline HWSanity()
        : CommandBase() {}

    /// Restart the automaton
    bool Reset(uint8_t param) override;

    /// @returns true if the state machine finished its job, false otherwise.
    /// LED indicators during the test execution:
    /// Slots 1-3: Pin states for STEP, DIR, and ENA
    /// Slot 4: Axis under test - G: Idler, R: Selector, RG: Pully.
    /// Slot 5: G: Blinking to indicate test progression. R: Solid to indicate completed test w/ fault.
    /// Indicators at test end (fault condition):
    /// Slots 1-3 now indicate pin
    /// - Off: No faults detected.
    /// - G:   STEP fault
    /// - R:   DIR fault
    /// - RG:  EN fault.
    /// - Blinking R/G: Multiple fault, e.g both an EN fault together with STEP and/or DIR.
    /// Slot 4: Reserved
    /// Slot 5: R: Solid
    bool StepInner() override;

private:
    static uint8_t test_step;
    static config::Axis axis;
    static uint8_t fault_masks[3];
    static ProgressCode next_state;
    static uint16_t wait_start;
};

/// The one and only instance of hwSanity state machine in the FW
extern HWSanity hwSanity;

} // namespace logic
