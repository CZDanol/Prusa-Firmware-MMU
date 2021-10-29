/// @file config.h
#pragma once
#include <stdint.h>
#include "axis.h"

/// Define Debug mode to add additional serial output

//#define DEBUG_FINDA
/// Enable DEBUG_LOGIC to compile debugging and error messages (beware of code base size ;) ) for the logic layer
//#define DEBUG_LOGIC

/// Enable DEBUG_MODULES to compile debugging and error messages (beware of code base size ;) ) for the modules layer
//#define DEBUG_MODULES

/// Enable DEBUG_HAL to compile debugging and error messages (beware of code base size ;) ) for the HAL layer
//#define DEBUG_HAL

/// Wrangler for assorted compile-time configuration and constants.
namespace config {

static constexpr const uint8_t toolCount = 5U; ///< Max number of extruders/tools/slots
static_assert(toolCount < 15, "Up to 14 valid slots (+1 parking) is supported in EEPROM storage");

// Printer's filament sensor setup
static constexpr const uint16_t fsensorDebounceMs = 10;

// LEDS
/// The complete period of LED's blinking (i.e. ON and OFF together)
/// Beware - in order to keep the blink periods "handle" millis overflow seamlessly
/// keep the period a power of 2 (i.e. 256, 512, 1024).
/// If you don't, one of the LED unit tests will fail.
static constexpr uint16_t ledBlinkPeriodMs = 1024U;
static_assert(ledBlinkPeriodMs == 256 || ledBlinkPeriodMs == 512 || ledBlinkPeriodMs == 1024 || ledBlinkPeriodMs == 2048, "LED blink period should be a power of 2");

// FINDA setup
static constexpr const uint16_t findaDebounceMs = 100;

// Buttons setup
static constexpr const uint8_t buttonCount = 3; ///< number of buttons currently supported
static constexpr const uint16_t buttonsDebounceMs = 100;
static constexpr const uint16_t buttonADCLimits[buttonCount][2] = { { 0, 50 }, { 80, 100 }, { 160, 180 } };
static constexpr const uint8_t buttonsADCIndex = 5; ///< ADC index of buttons input

// Motion and planning

/// Do not plan moves equal or shorter than the requested steps
static constexpr uint8_t dropSegments = 0;

/// Max step frequency 40KHz
static constexpr uint16_t maxStepFrequency = 40000;

/// Minimum stepping rate 120Hz
static constexpr uint16_t minStepRate = 120;

/// Size for the motion planner block buffer size
/// Beware of too low setting (esp. because of Motion::PlanLongMove)
static constexpr uint8_t blockBufferSize = 4;

/// Step timer frequency divider (F = F_CPU / divider)
static constexpr uint8_t stepTimerFrequencyDivider = 8;

/// Smallest stepping ISR scheduling slice (T = 1 / (F_CPU / divider) * quantum)
/// 25us is the max frequency interval per maxStepFrequency attainable for a single axis
/// while accelerating: with 3 axes this yields a required minimum of 75us
static constexpr uint16_t stepTimerQuantum = 256; // 256 = 128us

/// Max retries of FeedToBondtech used in LoadFilament
static constexpr uint8_t feedToBondtechMaxRetries = 2;

/// Distances
static constexpr U_mm pulleyToCuttingEdge = 33.0_mm; /// 33.0_mm /// Pulley to cutting edge.
/// Case 1: FINDA working: This should be the max retraction after FINDA un-triggers.
/// Case 2: FINDA not working: calculate retraction from printer to this point.
static constexpr U_mm filamentMinLoadedToMMU = 20.0_mm; /// 20.0_mm ??? /// Limit of retraction. @TODO find correct distance.
static constexpr U_mm ejectFromCuttingEdge = 40.0_mm; /// Eject should ignore FilamentMinLoadedToMMU and retract
static constexpr U_mm cuttingEdgeRetract = 5.0_mm; /// 3.0_mm /// Cutting retraction distance (filament should be flush with outlet) @TODO find correct distance.
static constexpr U_mm cuttingEdgeToFinda = 18.5_mm; /// 18.5_mm -1.0_mm /// Cutting edge to FINDA MMU2 side -1mm tolerance should be ~18.5. FINDA shouldn't trigger here.
static constexpr U_mm findaTriggerDistance = 4.5_mm; /// 9.0_mm /// FINDA trigger distance +1.0_mm tolerance.
static constexpr U_mm cuttingEdgeToFindaMidpoint = 22.85_mm; /// Cutting edge to Midpoint of FINDA should be 22.85_mm.
static constexpr U_mm findaToCoupler = 12.0_mm; /// 12.0_mm /// FINDA Coupler side to coupler screw.
static constexpr U_mm couplerToBowden = 3.5_mm; /// 3.5_mm /// FINDA Coupler screw to bowden mmu2s side (in coupling).

// @@TODO this is very tricky - the same MMU, same PTFE,
// just another piece of PLA (probably having more resistance in the tubes)
// and we are at least 40mm off! It looks like this really depends on the exact position
// We'll probably need to check for stallguard while pushing the filament to avoid ginding the filament
static constexpr U_mm defaultBowdenLength = 427.0_mm; /// ~427.0_mm /// Default Bowden length. @TODO Should be stored in EEPROM. 392 a 784
static constexpr U_mm minimumBowdenLength = 341.0_mm; /// ~341.0_mm /// Minimum bowden length. @TODO Should be stored in EEPROM.
static constexpr U_mm maximumBowdenLength = 792.0_mm; /// ~792.0_mm /// Maximum bowden length. @TODO Should be stored in EEPROM.
static constexpr U_mm feedToFinda = cuttingEdgeToFindaMidpoint + filamentMinLoadedToMMU;
static constexpr U_mm cutLength = 8.0_mm;
static constexpr U_mm fsensorToNozzle = 20.0_mm; /// ~20mm from MK4's filament sensor through extruder gears into nozzle
static constexpr U_mm fsensorToNozzleAvoidGrind = 5.0_mm;

/// Begin: Pulley axis configuration
static constexpr AxisConfig pulley = {
    .dirOn = false,
    .mRes = MRes_8,
    .vSense = true,
    .iRun = 20, /// 348mA
    .iHold = 0, /// 17mA in SpreadCycle, freewheel in StealthChop
    .stealth = false,
    .stepsPerUnit = (200 * 8 / 19.147274),
    .sg_thrs = 8,
};

/// Pulley motion limits
static constexpr PulleyLimits pulleyLimits = {
    .lenght = 1000.0_mm, // TODO
    .jerk = 4.0_mm_s,
    .accel = 800.0_mm_s2,
};
static constexpr U_mm_s pulleyFeedrate = 40._mm_s;
static constexpr U_mm_s pulleySlowFeedrate = 20._mm_s;
/// End: Pulley axis configuration

/// Begin: Selector configuration
static constexpr AxisConfig selector = {
    .dirOn = true,
    .mRes = MRes_8,
    .vSense = true,
    .iRun = 31, /// 530mA
    .iHold = 5, /// 99mA
    .stealth = false,
    .stepsPerUnit = (200 * 8 / 8.),
    .sg_thrs = 3,
};

/// Selector motion limits
static constexpr SelectorLimits selectorLimits = {
    .lenght = 75.0_mm, // @@TODO how does this relate to SelectorOffsetFromMin?
    .jerk = 1.0_mm_s,
    .accel = 200.0_mm_s2,
};

static constexpr U_mm SelectorSlotDistance = 14.0_mm; /// Selector distance between two slots
static constexpr U_mm SelectorOffsetFromMax = 1.0_mm; /// Selector offset from home max to slot 0
static constexpr U_mm SelectorOffsetFromMin = 75.5_mm; /// Selector offset from home min to slot 0

/// slots 0-4 are the real ones, the 5th is the farthest parking positions
/// selector.dirOn = true = Home at max: selector hits left side of the MMU2S body
/// selector.dirOn = false = Home at min: selector POM nut hit the selector motor
static constexpr U_mm selectorSlotPositions[toolCount + 1] = {

    ///selector max positions
    SelectorOffsetFromMax + 0 * SelectorSlotDistance, ///1.0_mm + 0 * 14.0_mm = 1.0_mm
    SelectorOffsetFromMax + 1 * SelectorSlotDistance, ///1.0_mm + 1 * 14.0_mm = 15.0_mm
    SelectorOffsetFromMax + 2 * SelectorSlotDistance, ///1.0_mm + 2 * 14.0_mm = 29.0_mm
    SelectorOffsetFromMax + 3 * SelectorSlotDistance, ///1.0_mm + 3 * 14.0_mm = 43.0_mm
    SelectorOffsetFromMax + 4 * SelectorSlotDistance, ///1.0_mm + 4 * 14.0_mm = 57.0_mm
    SelectorOffsetFromMax + 5 * SelectorSlotDistance ///1.0_mm + 5 * 14.0_mm = 71.0_mm

    ///selector min positions
    //    SelectorOffsetFromMin - 1.0_mm - 0 * SelectorSlotDistance, ///75.5_mm - 1.0_mm - 0 * 14.0_mm = 74.5_mm
    //    SelectorOffsetFromMin - 1.0_mm - 1 * SelectorSlotDistance, ///75.5_mm - 1.0_mm - 1 * 14.0_mm = 60.5_mm
    //    SelectorOffsetFromMin - 1.0_mm - 2 * SelectorSlotDistance, ///75.5_mm - 1.0_mm - 2 * 14.0_mm = 46.5_mm
    //    SelectorOffsetFromMin - 1.0_mm - 3 * SelectorSlotDistance, ///75.5_mm - 1.0_mm - 3 * 14.0_mm = 32.5_mm
    //    SelectorOffsetFromMin - 1.0_mm - 4 * SelectorSlotDistance, ///75.5_mm - 1.0_mm - 4 * 14.0_mm = 18.5_mm
    //    SelectorOffsetFromMin - 1.0_mm - 5 * SelectorSlotDistance ///75.5_mm - 1.0_mm - 5 * 14.0_mm = 4.5_mm
};

static constexpr U_mm_s selectorFeedrate = 30._mm_s;
/// End: Selector configuration

/// Begin: Idler configuration
static constexpr AxisConfig idler = {
    .dirOn = true,
    .mRes = MRes_16,
    .vSense = true,
    .iRun = 31, /// 530mA
    .iHold = 23, /// 398mA
    .stealth = false,
    .stepsPerUnit = (200 * 16 / 360.),
    .sg_thrs = 8,
};

/// Idler motion limits
static constexpr IdlerLimits idlerLimits = {
    .lenght = 270.0_deg,
    .jerk = 0.1_deg_s,
    .accel = 500.0_deg_s2,
};

static constexpr U_deg IdlerSlotDistance = 40.0_deg; /// Idler distance between two slots
static constexpr U_deg IdlerOffsetFromHome = 18.0_deg; /// Idler offset from home to slots

/// Absolute positions for Idler's slots: 0-4 are the real ones, the 5th index is the idle position
/// Home ccw with 5th idler bearing facing selector
static constexpr U_deg idlerSlotPositions[toolCount + 1] = {
    IdlerOffsetFromHome + 5 * IdlerSlotDistance, /// Slot 0 at 218 degree after homing ///18.0_deg + 5 * 40.0_deg = 218.0_deg
    IdlerOffsetFromHome + 4 * IdlerSlotDistance, /// Slot 1 at 178 degree after homing ///18.0_deg + 4 * 40.0_deg = 178.0_deg
    IdlerOffsetFromHome + 3 * IdlerSlotDistance, /// Slot 2 at 138 degree after homing ///18.0_deg + 3 * 40.0_deg = 138.0_deg
    IdlerOffsetFromHome + 2 * IdlerSlotDistance, /// Slot 3 at 98 degree after homing ///18.0_deg + 2 * 40.0_deg = 98.0_deg
    IdlerOffsetFromHome + 1 * IdlerSlotDistance, /// Slot 4 at 58 degree after homing ///18.0_deg + 1 * 40.0_deg = 58.0_deg
    IdlerOffsetFromHome ///18.0_deg Fully disengaged all slots
};

static constexpr U_deg idlerParkPositionDelta = -IdlerSlotDistance + 5.0_deg / 2; ///@TODO verify

static constexpr U_deg_s idlerFeedrate = 200._deg_s;
/// End: Idler configuration

// TMC2130 setup

// static constexpr int8_t tmc2130_sg_thrs = 3;
// static_assert(tmc2130_sg_thrs >= -64 && tmc2130_sg_thrs <= 63, "tmc2130_sg_thrs out of range");

static constexpr uint32_t tmc2130_coolStepThreshold = 5000; ///< step-based 20bit uint
static_assert(tmc2130_coolStepThreshold <= 0xfffff, "tmc2130_coolStepThreshold out of range");

static constexpr uint32_t tmc2130_PWM_AMPL = 240;
static_assert(tmc2130_PWM_AMPL <= 255, "tmc2130_PWM_AMPL out of range");

static constexpr uint32_t tmc2130_PWM_GRAD = 4;
static_assert(tmc2130_PWM_GRAD <= 255, "tmc2130_PWM_GRAD out of range");

static constexpr uint32_t tmc2130_PWM_FREQ = 2;
static_assert(tmc2130_PWM_FREQ <= 3, "tmc2130_PWM_GRAD out of range");

static constexpr uint32_t tmc2130_PWM_AUTOSCALE = 1;
static_assert(tmc2130_PWM_AUTOSCALE <= 1, "tmc2130_PWM_AUTOSCALE out of range");

/// Freewheel options for standstill:
/// 0: Normal operation (IHOLD is supplied to the motor at standstill)
/// 1: Freewheeling (as if the driver was disabled, no braking except for detent torque)
/// 2: Coil shorted using LS drivers (stronger passive braking)
/// 3: Coil shorted using HS drivers (weaker passive braking)
static constexpr uint32_t tmc2130_freewheel = 1;
static_assert(tmc2130_PWM_AUTOSCALE <= 3, "tmc2130_freewheel out of range");

} // namespace config
