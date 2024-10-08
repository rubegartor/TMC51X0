#include <TMC51X0.hpp>


#if !defined(ARDUINO_ARCH_RP2040)
HardwareSerial & uart = Serial2;
#else
SerialUART & uart = Serial2;
pin_size_t TX_PIN = 4;
pin_size_t RX_PIN = 5;
#endif

const uint8_t ENABLE_VCC_PIN = 22;
const uint8_t ENABLE_VCC_POLARITY = HIGH;

// ENABLE_TX_PIN and ENABLE_RX_PIN may be the same pin
const uint32_t UART_BAUD_RATE = 115200;
const uint8_t NODE_ADDRESS = 1;
const uint8_t ENABLE_TX_POLARITY = HIGH;
const uint8_t ENABLE_RX_POLARITY = LOW;

const uint8_t ENABLE_TX_PIN = 15;
const uint8_t ENABLE_RX_PIN = 14;

const long SERIAL_BAUD_RATE = 115200;
const int DELAY = 4000;
const int RESET_DELAY = 5000;

// converter constants
// external clock is ~16MHz
const uint8_t CLOCK_FREQUENCY_MHZ = 16;
// 200 fullsteps per revolution for many steppers * 256 microsteps per fullstep
// one "real unit" in this example is one fullstep of the motor shaft
constexpr uint32_t MICROSTEPS_PER_REAL_UNIT = 256;

// driver constants
const uint8_t GLOBAL_CURRENT_SCALAR = 100; // percent
const uint8_t RUN_CURRENT = 20; // percent
const uint8_t PWM_OFFSET = 20; // percent
const uint8_t PWM_GRADIENT = 5; // percent
const tmc51x0::Driver::MotorDirection MOTOR_DIRECTION = tmc51x0::Driver::FORWARD;
const uint8_t STEALTH_CHOP_THRESHOLD = 50; // fullsteps/s
const uint8_t COOL_STEP_THRESHOLD = 200; // fullsteps/s
const uint8_t MIN_COOL_STEP = 1;
const uint8_t MAX_COOL_STEP = 0;
const uint8_t HIGH_VELOCITY_THRESHOLD = 600; // fullsteps/s
// const int8_t STALL_GUARD_THRESHOLD = -20;

// controller constants
const int32_t MIN_TARGET_VELOCITY = 50;  // fullsteps/s
const int32_t MAX_TARGET_VELOCITY = 500; // fullsteps/s
const int32_t TARGET_VELOCITY_INC = 50;  // fullsteps/s
const uint32_t MAX_ACCELERATION = 100;  // fullsteps/(s^2)
const int32_t INITIAL_POSITION = 0;

// Instantiate TMC51X0
TMC51X0 tmc5130;
uint32_t target_velocity;
tmc51x0::Controller::RampMode ramp_mode = tmc51x0::Controller::VELOCITY_POSITIVE;

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);

  pinMode(ENABLE_VCC_PIN, OUTPUT);
  digitalWrite(ENABLE_VCC_PIN, !ENABLE_VCC_POLARITY);

  delay(RESET_DELAY);

  pinMode(ENABLE_VCC_PIN, OUTPUT);
  digitalWrite(ENABLE_VCC_PIN, ENABLE_VCC_POLARITY);

  delay(RESET_DELAY);

#if defined(ARDUINO_ARCH_RP2040)
  uart.setTX(TX_PIN);
  uart.setRX(RX_PIN);
#endif
  uart.begin(UART_BAUD_RATE);

  tmc51x0::UartParameters uart_parameters(uart,
    NODE_ADDRESS,
    ENABLE_TX_PIN,
    ENABLE_RX_PIN,
    ENABLE_TX_POLARITY,
    ENABLE_RX_POLARITY);
  tmc5130.setupUart(uart_parameters);
  tmc51x0::Converter::Settings converter_settings =
    {
      CLOCK_FREQUENCY_MHZ,
      MICROSTEPS_PER_REAL_UNIT
    };
  tmc5130.converter.setup(converter_settings);
  tmc5130.driver.writeGlobalCurrentScaler(tmc5130.converter.percentToGlobalCurrentScaler(GLOBAL_CURRENT_SCALAR));
  tmc5130.driver.writeRunCurrent(tmc5130.converter.percentToCurrentSetting(RUN_CURRENT));
  tmc5130.driver.writePwmOffset(tmc5130.converter.percentToPwmSetting(PWM_OFFSET));
  tmc5130.driver.writePwmGradient(tmc5130.converter.percentToPwmSetting(PWM_GRADIENT));
  tmc5130.driver.writeMotorDirection(MOTOR_DIRECTION);

  tmc5130.driver.writeStealthChopThreshold(tmc5130.converter.velocityRealToTstep(STEALTH_CHOP_THRESHOLD));

  // tmc5130.driver.writeCoolStepThreshold(tmc5130.converter.velocityRealToTstep(COOL_STEP_THRESHOLD));
  // tmc5130.driver.enableCoolStep(MIN_COOL_STEP, MAX_COOL_STEP);

  // tmc5130.driver.writeHighVelocityThreshold(tmc5130.converter.velocityRealToTstep(HIGH_VELOCITY_THRESHOLD));
  // tmc5130.driver.writeStallGuardThreshold(STALL_GUARD_THRESHOLD);

  tmc5130.controller.writeMaxAcceleration(tmc5130.converter.accelerationRealToChip(MAX_ACCELERATION));
  tmc5130.controller.writeRampMode(ramp_mode);
  tmc5130.controller.writeActualPosition(tmc5130.converter.positionRealToChip(INITIAL_POSITION));

  tmc5130.driver.enable();

  tmc5130.controller.rampToZeroVelocity();
  while (!tmc5130.controller.zeroVelocity())
  {
    Serial.println("Waiting for zero velocity.");
    delay(DELAY);
  }

  target_velocity = MIN_TARGET_VELOCITY;
  tmc5130.controller.writeMaxVelocity(tmc5130.converter.velocityRealToChip(target_velocity));

  delay(DELAY);
}

void loop()
{
  tmc5130.printer.readAndPrintGconf();
  tmc5130.printer.readClearAndPrintGstat();
  tmc5130.printer.readAndPrintRampStat();
  tmc5130.printer.readAndPrintDrvStatus();
  tmc5130.printer.readAndPrintPwmScale();

  // Serial.print("acceleration (fullsteps per second per second): ");
  // Serial.println(MAX_ACCELERATION);
  // Serial.print("acceleration (chip units): ");
  // Serial.println(tmc5130.converter.accelerationRealToChip(MAX_ACCELERATION));
  // Serial.println("--------------------------");

    if (ramp_mode == tmc51x0::Controller::VELOCITY_POSITIVE)
    {
      Serial.println("velocity: positive");
    }
    else
    {
      Serial.println("velocity: negative");
    }
  Serial.print("target_velocity (fullsteps per second): ");
  Serial.println(target_velocity);
  int32_t actual_velocity_chip = tmc5130.controller.readActualVelocity();
  // Serial.print("actual_velocity (chip units): ");
  // Serial.println(actual_velocity_chip);
  int32_t actual_velocity_real = tmc5130.converter.velocityChipToReal(actual_velocity_chip);
  Serial.print("actual_velocity (fullsteps per second): ");
  Serial.println(actual_velocity_real);
  // uint32_t tstep = tmc5130.controller.readTstep();
  // Serial.print("tstep (chip units): ");
  // Serial.println(tstep);
  // uint32_t velocity_real = tmc5130.converter.tstepToVelocityReal(tstep);
  // Serial.print("tstepToVelocityReal (fullsteps per second): ");
  // Serial.println(velocity_real);
  // tstep = tmc5130.converter.velocityRealToTstep(velocity_real);
  // Serial.print("velocityRealToTstep (chip_units): ");
  // Serial.println(tstep);
  // Serial.print("STEALTH_CHOP_THRESHOLD (fullsteps per second): ");
  // Serial.println(STEALTH_CHOP_THRESHOLD);
  // Serial.print("STEALTH_CHOP_THRESHOLD (chip units): ");
  // Serial.println(tmc5130.converter.velocityRealToTstep(STEALTH_CHOP_THRESHOLD));
  Serial.println("--------------------------");

  // int32_t actual_position_chip = tmc5130.controller.readActualPosition();
  // Serial.print("actual position (chip units): ");
  // Serial.println(actual_position_chip);
  // int32_t actual_position_real = tmc5130.converter.positionChipToReal(actual_position_chip);
  // Serial.print("actual position (fullsteps): ");
  // Serial.println(actual_position_real);
  // Serial.println("--------------------------");

  Serial.println("--------------------------");

  delay(DELAY);

  target_velocity += TARGET_VELOCITY_INC;
  if (target_velocity > MAX_TARGET_VELOCITY)
  {
    target_velocity = MIN_TARGET_VELOCITY;
    if (ramp_mode == tmc51x0::Controller::VELOCITY_POSITIVE)
    {
      ramp_mode = tmc51x0::Controller::VELOCITY_NEGATIVE;
    }
    else
    {
      ramp_mode = tmc51x0::Controller::VELOCITY_POSITIVE;
    }
    tmc5130.controller.writeRampMode(ramp_mode);
  }
  tmc5130.controller.writeMaxVelocity(tmc5130.converter.velocityRealToChip(target_velocity));

  delay(DELAY);
}
