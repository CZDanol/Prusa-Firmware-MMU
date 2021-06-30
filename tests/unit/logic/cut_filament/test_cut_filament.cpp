#include "catch2/catch.hpp"

#include "../../../../src/modules/buttons.h"
#include "../../../../src/modules/finda.h"
#include "../../../../src/modules/fsensor.h"
#include "../../../../src/modules/globals.h"
#include "../../../../src/modules/idler.h"
#include "../../../../src/modules/leds.h"
#include "../../../../src/modules/motion.h"
#include "../../../../src/modules/permanent_storage.h"
#include "../../../../src/modules/selector.h"

#include "../../../../src/logic/cut_filament.h"

#include "../../modules/stubs/stub_adc.h"

#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

using Catch::Matchers::Equals;

namespace mm = modules::motion;
namespace mf = modules::finda;
namespace mi = modules::idler;
namespace ml = modules::leds;
namespace mb = modules::buttons;
namespace mg = modules::globals;
namespace ms = modules::selector;

#include "../helpers/helpers.ipp"

void CutSlot(uint8_t cutSlot) {

    ForceReinitAllAutomata();

    logic::CutFilament cf;
    REQUIRE(VerifyState(cf, false, mi::Idler::IdleSlotIndex(), 0, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));

    EnsureActiveSlotIndex(cutSlot);

    // restart the automaton
    cf.Reset(cutSlot);

    // check initial conditions
    REQUIRE(VerifyState(cf, false, mi::Idler::IdleSlotIndex(), cutSlot, false, ml::blink0, ml::off, ErrorCode::OK, ProgressCode::SelectingFilamentSlot));

    // now cycle at most some number of cycles (to be determined yet) and then verify, that the idler and selector reached their target positions
    REQUIRE(WhileTopState(cf, ProgressCode::SelectingFilamentSlot, 5000));

    // idler and selector reached their target positions and the CF automaton will start feeding to FINDA as the next step
    REQUIRE(VerifyState(cf, false, cutSlot, cutSlot, false, ml::blink0, ml::off, ErrorCode::OK, ProgressCode::FeedingToFinda));

    // prepare for simulated finda trigger
    REQUIRE(WhileCondition(
        cf,
        [&](int step) -> bool {
        if( step == 100 ){ // simulate FINDA trigger - will get pressed in 100 steps (due to debouncing)
            hal::adc::SetADC(config::findaADCIndex, 900);
        }
        return cf.TopLevelState() == ProgressCode::FeedingToFinda; }, 5000));

    // filament fed to FINDA
    //@@TODO filament loaded flag - decide whether the filament loaded flag means really loaded into the printer or just a piece of filament
    // stuck out of the pulley to prevent movement of the selector
    REQUIRE(VerifyState(cf, /*true*/ false, cutSlot, cutSlot, true, ml::blink0, ml::off, ErrorCode::OK, ProgressCode::UnloadingToPulley));

    // pull it back to the pulley + simulate FINDA depress
    REQUIRE(WhileCondition(
        cf,
        [&](int step) -> bool {
        if( step == 100 ){ // simulate FINDA trigger - will get depressed in 100 steps
            hal::adc::SetADC(config::findaADCIndex, 0);
        }
        return cf.TopLevelState() == ProgressCode::UnloadingToPulley; }, 5000));

    REQUIRE(VerifyState(cf, /*true*/ false, cutSlot, cutSlot, false, ml::blink0, ml::off, ErrorCode::OK, ProgressCode::PreparingBlade));

    // now move the selector aside, prepare for cutting
    REQUIRE(WhileTopState(cf, ProgressCode::PreparingBlade, 5000));
    REQUIRE(VerifyState2(cf, /*true*/ false, cutSlot, cutSlot + 1, false, cutSlot, ml::blink0, ml::off, ErrorCode::OK, ProgressCode::PushingFilament));

    // pushing filament a bit for a cut
    REQUIRE(WhileTopState(cf, ProgressCode::PushingFilament, 5000));
    REQUIRE(VerifyState2(cf, /*true*/ false, cutSlot, cutSlot + 1, false, cutSlot, ml::blink0, ml::off, ErrorCode::OK, ProgressCode::PerformingCut));

    // cutting
    REQUIRE(WhileTopState(cf, ProgressCode::PerformingCut, 10000));
    REQUIRE(VerifyState2(cf, /*true*/ false, cutSlot, 0, false, cutSlot, ml::blink0, ml::off, ErrorCode::OK, ProgressCode::ReturningSelector));

    // moving selector to the other end of its axis
    REQUIRE(WhileTopState(cf, ProgressCode::ReturningSelector, 5000));
    REQUIRE(VerifyState2(cf, /*true*/ false, cutSlot, ms::Selector::IdleSlotIndex(), false, cutSlot, ml::on, ml::off, ErrorCode::OK, ProgressCode::OK));
}

TEST_CASE("cut_filament::cut0", "[cut_filament]") {
    for (uint8_t cutSlot = 0; cutSlot < config::toolCount; ++cutSlot) {
        CutSlot(cutSlot);
    }
}