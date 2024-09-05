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
const int DELAY = 2000;

// Instantiate TMC51X0
TMC51X0 tmc5130;

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);

  pinMode(ENABLE_VCC_PIN, OUTPUT);
  digitalWrite(ENABLE_VCC_PIN, ENABLE_VCC_POLARITY);

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
}

void loop()
{
  tmc5130.printer.readAndPrintGconf();
  // tmc5130.printer.readAndPrintRampStat();
  // tmc5130.printer.readAndPrintChopconf();

  delay(DELAY);
  Serial.println("--------------------------");
}
