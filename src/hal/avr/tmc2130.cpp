#include "../tmc2130.h"
#include "../../config/config.h"

namespace hal {
namespace tmc2130 {

TMC2130::TMC2130(const MotorParams &params, const MotorCurrents &currents, MotorMode mode)
    : mode(mode)
    , currents(currents)
    , sg_counter(0) {
    Init(params);
}

bool TMC2130::Init(const MotorParams &params) {
    gpio::Init(params.csPin, gpio::GPIO_InitTypeDef(gpio::Mode::output, gpio::Level::high));
    gpio::Init(params.sgPin, gpio::GPIO_InitTypeDef(gpio::Mode::input, gpio::Pull::up));
    gpio::Init(params.stepPin, gpio::GPIO_InitTypeDef(gpio::Mode::output, gpio::Level::low));

    ///check for compatible tmc driver (IOIN version field)
    uint32_t IOIN = ReadRegister(params, Registers::IOIN);

    // if the version is incorrect or an always 1st bit is 0
    // (the supposed SD_MODE pin that doesn't exist on this driver variant)
    if (((IOIN >> 24U) != 0x11) | !(IOIN & (1U << 6U)))
        return true; // @todo return some kind of failure

    ///clear reset_flag as we are (re)initializing
    errorFlags.reset_flag = false;

    ///apply chopper parameters
    const uint32_t chopconf = (uint32_t)(3U & 0x0FU) << 0U //toff
        | (uint32_t)(5U & 0x07U) << 4U //hstrt
        | (uint32_t)(1U & 0x0FU) << 7U //hend
        | (uint32_t)(2U & 0x03U) << 15U //tbl
        | (uint32_t)(currents.vSense & 0x01U) << 17U //vsense
        | (uint32_t)(params.uSteps & 0x0FU) << 24U //mres
        | (uint32_t)((bool)params.uSteps) << 28U //intpol
        | (uint32_t)(1U & 0x01) << 29U; //dedge
    WriteRegister(params, Registers::CHOPCONF, chopconf);

    ///apply currents
    SetCurrents(params, currents);

    ///instant powerdown ramp
    WriteRegister(params, Registers::TPOWERDOWN, 0);

    ///Stallguard parameters
    WriteRegister(params, Registers::COOLCONF, config::tmc2130_coolConf);
    WriteRegister(params, Registers::TCOOLTHRS, config::tmc2130_coolStepThreshold);

    ///Write stealth mode config and setup diag0 output
    constexpr uint32_t gconf = (uint32_t)(1U & 0x01U) << 2U //en_pwm_mode - always enabled since we can control it's effect with TPWMTHRS (0=only stealthchop, 0xFFFFF=only spreadcycle)
        | (uint32_t)(1U & 0x01U) << 7U; //diag0_stall - diag0 is open collector => active low with external pullups
    WriteRegister(params, Registers::GCONF, gconf);

    ///stealthChop parameters
    constexpr uint32_t pwmconf = config::tmc2130_PWM_AMPL | config::tmc2130_PWM_GRAD | config::tmc2130_PWM_FREQ | config::tmc2130_PWM_AUTOSCALE;
    WriteRegister(params, Registers::PWMCONF, pwmconf);

    /// TPWMTHRS: switching velocity between stealthChop and spreadCycle.
    /// Stallguard is also disabled if the velocity falls below this.
    /// Should be set as high as possible when homing.
    SetMode(params, mode);
    return false;
}

void TMC2130::SetMode(const MotorParams &params, MotorMode mode) {
    this->mode = mode;

    ///0xFFF00 is used as a "Normal" mode threshold since stealthchop will be used at standstill.
    WriteRegister(params, Registers::TPWMTHRS, (mode == Stealth) ? 70 : 0xFFF00); // @todo should be configurable
}

void TMC2130::SetCurrents(const MotorParams &params, const MotorCurrents &currents) {
    this->currents = currents;

    uint32_t ihold_irun = (uint32_t)(currents.iHold & 0x1F) << 0 //ihold
        | (uint32_t)(currents.iRun & 0x1F) << 8 //irun
        | (uint32_t)(15 & 0x0F) << 16; //IHOLDDELAY
    WriteRegister(params, Registers::IHOLD_IRUN, ihold_irun);
}

void TMC2130::SetEnabled(const MotorParams &params, bool enabled) {
    hal::shr16::shr16.SetTMCDir(params.idx, enabled);
    if (this->enabled != enabled)
        ClearStallguard(params);
    this->enabled = enabled;
}

void TMC2130::ClearStallguard(const MotorParams &params) {
    // @todo: maximum resolution right now is x256/4 (uint8_t / 4)
    sg_counter = 4 * (1 << (8 - params.uSteps)) - 1; /// one electrical full step (4 steps when fullstepping)
}

bool TMC2130::CheckForErrors(const MotorParams &params) {
    uint32_t GSTAT = ReadRegister(params, Registers::GSTAT);
    uint32_t DRV_STATUS = ReadRegister(params, Registers::DRV_STATUS);
    errorFlags.reset_flag |= GSTAT & (1U << 0U);
    errorFlags.uv_cp = GSTAT & (1U << 2U);
    errorFlags.s2g = DRV_STATUS & (3UL << 27U);
    errorFlags.otpw = DRV_STATUS & (1UL << 26U);
    errorFlags.ot = DRV_STATUS & (1UL << 25U);

    return GSTAT || errorFlags.reset_flag; //any bit in gstat is an error
}

uint32_t TMC2130::ReadRegister(const MotorParams &params, Registers reg) {
    uint8_t pData[5] = { (uint8_t)reg };
    _spi_tx_rx(params, pData);
    pData[0] = 0;
    _spi_tx_rx(params, pData);
    _handle_spi_status(params, pData[0]);
    return ((uint32_t)pData[1] << 24 | (uint32_t)pData[2] << 16 | (uint32_t)pData[3] << 8 | (uint32_t)pData[4]);
}

void TMC2130::WriteRegister(const MotorParams &params, Registers reg, uint32_t data) {
    uint8_t pData[5] = { (uint8_t)((uint8_t)(reg) | 0x80), (uint8_t)(data >> 24), (uint8_t)(data >> 16), (uint8_t)(data >> 8), (uint8_t)data };
    _spi_tx_rx(params, pData);
    _handle_spi_status(params, pData[0]);
}

void TMC2130::Isr(const MotorParams &params) {
    if (sg_counter) {
        if (SampleDiag(params))
            sg_counter--;
        else if (sg_counter < (4 * (1 << (8 - params.uSteps)) - 1))
            sg_counter++;
    }
}

void TMC2130::_spi_tx_rx(const MotorParams &params, uint8_t (&pData)[5]) {
    hal::gpio::WritePin(params.csPin, hal::gpio::Level::low);
    for (uint8_t i = 0; i < sizeof(pData); i++) {
        // @@TODO horrible hack to persuate the compiler, that the expression is const in terms of memory layout and meaning,
        // but we need to write into those registers
        pData[i] = hal::spi::TxRx(const_cast<hal::spi::SPI_TypeDef *>(params.spi), pData[i]);
    }
    hal::gpio::WritePin(params.csPin, hal::gpio::Level::high);
}

void TMC2130::_handle_spi_status(const MotorParams &params, uint8_t status) {
    // errorFlags.reset_flag |= status & (1 << 0);
    // errorFlags.driver_error |= status & (1 << 1);
}

} // namespace tmc2130
} // namespace hal