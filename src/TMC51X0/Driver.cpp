// ----------------------------------------------------------------------------
// Driver.cpp
//
// Authors:
// Peter Polidoro peter@polidoro.io
// ----------------------------------------------------------------------------
#include "Driver.hpp"


using namespace tmc51x0;

Driver::Driver()
{
  hardware_enable_pin_ = -1;
}

void Driver::setHardwareEnablePin(size_t hardware_enable_pin)
{
  hardware_enable_pin_ = hardware_enable_pin;
  pinMode(hardware_enable_pin_, OUTPUT);
  hardwareDisable();
}

void Driver::enable()
{
  hardwareEnable();
  softwareEnable();
}

void Driver::disable()
{
  hardwareDisable();
  softwareDisable();
}

void Driver::setGlobalCurrentScaler(uint8_t percent)
{
  uint8_t scaler = percentToGlobalCurrentScaler(percent);
  Serial.print("percent = ");
  Serial.print(percent);
  Serial.print(", scaler = ");
  Serial.println(scaler);
  // registers_ptr_->write(Registers::GLOBAL_SCALER, scaler);
}

void Driver::setRunCurrent(uint8_t percent)
{
  Registers::IholdIrun ihold_irun;
  ihold_irun.bytes = registers_ptr_->getStored(Registers::IHOLD_IRUN);
  ihold_irun.irun = percentToCurrentSetting(percent);
  registers_ptr_->write(Registers::IHOLD_IRUN, ihold_irun.bytes);
}

void Driver::setHoldCurrent(uint8_t percent)
{
  Registers::IholdIrun ihold_irun;
  ihold_irun.bytes = registers_ptr_->getStored(Registers::IHOLD_IRUN);
  ihold_irun.ihold = percentToCurrentSetting(percent);
  registers_ptr_->write(Registers::IHOLD_IRUN, ihold_irun.bytes);
}

void Driver::setHoldDelay(uint8_t percent)
{
  Registers::IholdIrun ihold_irun;
  ihold_irun.bytes = registers_ptr_->getStored(Registers::IHOLD_IRUN);
  ihold_irun.iholddelay = percentToHoldDelaySetting(percent);
  registers_ptr_->write(Registers::IHOLD_IRUN, ihold_irun.bytes);
}

void Driver::setAllCurrentValues(uint8_t run_current_percent,
  uint8_t hold_current_percent,
  uint8_t hold_delay_percent)
{
  Registers::IholdIrun ihold_irun;
  ihold_irun.irun = percentToCurrentSetting(run_current_percent);
  ihold_irun.ihold = percentToCurrentSetting(hold_current_percent);
  ihold_irun.iholddelay = percentToHoldDelaySetting(hold_delay_percent);
  registers_ptr_->write(Registers::IHOLD_IRUN, ihold_irun.bytes);
}

void Driver::enableAutomaticCurrentControl()
{
  Registers::Pwmconf pwmconf;
  pwmconf.bytes = registers_ptr_->getStored(Registers::PWMCONF);
  pwmconf.pwm_autoscale = 1;
  pwmconf.pwm_autograd = 1;
  registers_ptr_->write(Registers::PWMCONF, pwmconf.bytes);
}

void Driver::disableAutomaticCurrentControl()
{
  Registers::Pwmconf pwmconf;
  pwmconf.bytes = registers_ptr_->getStored(Registers::PWMCONF);
  pwmconf.pwm_autoscale = 0;
  pwmconf.pwm_autograd = 0;
  registers_ptr_->write(Registers::PWMCONF, pwmconf.bytes);
}

// private

void Driver::setup(Registers & registers)
{
  registers_ptr_ = &registers;
  toff_ = TOFF_ENABLE_DEFAULT;

  disable();
  minimizeMotorCurrent();
  disableAutomaticCurrentControl();
}

void Driver::hardwareEnable()
{
  if (hardware_enable_pin_ >= 0)
  {
    digitalWrite(hardware_enable_pin_, LOW);
  }
}

void Driver::hardwareDisable()
{
  if (hardware_enable_pin_ >= 0)
  {
    digitalWrite(hardware_enable_pin_, HIGH);
  }
}

void Driver::softwareEnable()
{
  Registers::Chopconf chopconf;
  chopconf.bytes = registers_ptr_->getStored(Registers::CHOPCONF);
  chopconf.toff = toff_;
  registers_ptr_->write(Registers::CHOPCONF, chopconf.bytes);
}

void Driver::softwareDisable()
{
  Registers::Chopconf chopconf;
  chopconf.bytes = registers_ptr_->getStored(Registers::CHOPCONF);
  chopconf.toff = DISABLE_TOFF;
  registers_ptr_->write(Registers::CHOPCONF, chopconf.bytes);
}

uint8_t Driver::percentToGlobalCurrentScaler(uint8_t percent)
{
  uint8_t constrained_percent = constrain_(percent,
    PERCENT_MIN,
    PERCENT_MAX);
  uint16_t scaler = map(constrained_percent,
    PERCENT_MIN,
    PERCENT_MAX,
    GLOBAL_SCALER_MIN,
    GLOBAL_SCALER_MAX);
  if (scaler < GLOBAL_SCALER_THRESHOLD)
  {
    scaler = GLOBAL_SCALER_THRESHOLD;
  }
  else if (scaler >= GLOBAL_SCALER_MAX)
  {
    scaler = 0;
  }
  return scaler;
}

uint8_t Driver::percentToCurrentSetting(uint8_t percent)
{
  uint8_t constrained_percent = constrain_(percent,
    PERCENT_MIN,
    PERCENT_MAX);
  uint8_t current_setting = map(constrained_percent,
    PERCENT_MIN,
    PERCENT_MAX,
    CURRENT_SETTING_MIN,
    CURRENT_SETTING_MAX);
  return current_setting;
}

uint8_t Driver::currentSettingToPercent(uint8_t current_setting)
{
  uint8_t percent = map(current_setting,
    CURRENT_SETTING_MIN,
    CURRENT_SETTING_MAX,
    PERCENT_MIN,
    PERCENT_MAX);
  return percent;
}

uint8_t Driver::percentToHoldDelaySetting(uint8_t percent)
{
  uint8_t constrained_percent = constrain_(percent,
    PERCENT_MIN,
    PERCENT_MAX);
  uint8_t hold_delay_setting = map(constrained_percent,
    PERCENT_MIN,
    PERCENT_MAX,
    HOLD_DELAY_MIN,
    HOLD_DELAY_MAX);
  return hold_delay_setting;
}

uint8_t Driver::holdDelaySettingToPercent(uint8_t hold_delay_setting)
{
  uint8_t percent = map(hold_delay_setting,
    HOLD_DELAY_MIN,
    HOLD_DELAY_MAX,
    PERCENT_MIN,
    PERCENT_MAX);
  return percent;
}

void Driver::minimizeMotorCurrent()
{
  uint32_t global_scaler = GLOBAL_SCALER_MIN;
  registers_ptr_->write(Registers::GLOBAL_SCALER, global_scaler);

  Registers::IholdIrun ihold_irun;
  ihold_irun.ihold = CURRENT_SETTING_MIN;
  ihold_irun.irun = CURRENT_SETTING_MIN;
  registers_ptr_->write(Registers::IHOLD_IRUN, ihold_irun.bytes);
}

uint32_t Driver::constrain_(uint32_t value, uint32_t low, uint32_t high)
{
  return ((value)<(low)?(low):((value)>(high)?(high):(value)));
}
