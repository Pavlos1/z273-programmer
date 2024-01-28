#include <SoftwareSerial.h>
#include "data.h"

// Enable Z273 mode. If disabled, the 2716 chip should be taken off the board before being programmed.
#define Z273

// For AT28C16-based designs
//#define LOW_VOLTAGE

#define pinA(x) ((x) + 40) // For Z273 mode: connect A2 .. A7 to pinA(0) .. pinA(5)
#define pinO(x) ((x) + 22)

#define addr_pins_total 11

#ifdef Z273
#define addr_pins 6
#else
#define addr_pins addr_pins_total
#endif

#define data_pins 8

#define pin_OE 2 // low during reads
#define pin_CE 3 // pulsed during programming

#define pin_CLK 4 // only for Z273 mode

#define pin_SW 11 // for silent writes
#define pin_LED 13

#define pin_VPP 8 // supplies +5V during reads

//SoftwareSerial ser(10, 11);
#define ser Serial

void common_setup(void) {
  digitalWrite(pin_OE, 0);
  digitalWrite(pin_CE, 0);
  digitalWrite(pin_CLK, 0);
  pinMode(pin_OE, OUTPUT);
  pinMode(pin_CE, OUTPUT);
  pinMode(pin_CLK, OUTPUT);

  digitalWrite(pin_SW, 1);
  pinMode(pin_SW, INPUT_PULLUP);
  digitalWrite(pin_VPP, 1);
  pinMode(pin_VPP, OUTPUT);
  digitalWrite(pin_LED, 0);
  pinMode(pin_LED, OUTPUT);

  for (int i = 0; i < addr_pins; i++) {
    pinMode(pinA(i), OUTPUT);
  }
}

void blink_and_wait(uint16_t interval) {
  uint16_t guard_time = 100;
  uint16_t left = interval;

  digitalWrite(pin_LED, 0);
  while (1) {
    delay(1);
    if (guard_time) guard_time--;
    if (left) left --;

    if (!guard_time && !digitalRead(pin_SW)) {
      digitalWrite(pin_LED, 0);
      return;
    }

    if (!left) {
      digitalWrite(pin_LED, !digitalRead(pin_LED));
      left = interval;
    }
  }
}

#ifdef Z273
void set_addr(uint16_t addr) {
  const uint8_t weird_addr_pin_count = 5;
  const uint8_t weird_addr_pins[weird_addr_pin_count] = {0, 1, 8, 9, 10};

  // Write first 6 addr lines
  for (int i = 2; i <= 7; i++) {
    digitalWrite(pinA(i - 2), (addr & (1 << i)) >> i);
  }

  // Latch addr lines into flip-flop
  delay(1);
  digitalWrite(pin_CLK, 1);
  delay(1);
  digitalWrite(pin_CLK, 0);
  delay(1);

  // Write remaining addr lines
  for (uint16_t i=0; i<weird_addr_pin_count; i++) {
     digitalWrite(pinA(i), (addr & (1 << weird_addr_pins[i])) >> weird_addr_pins[i]);
  }
}
#else
void set_addr(uint16_t addr) {
  for (uint16_t i = 0; i < addr_pins; i++) {
    digitalWrite(pinA(i), (addr & (1 << i)) >> i);
  }
}
#endif

void read_setup(void) {
  common_setup();

  digitalWrite(pin_CE, 0);
  digitalWrite(pin_OE, 0);
  for (int i = 0; i < data_pins; i++) {
    pinMode(pinO(i), INPUT);
  }
}

void write_setup(void) {
  common_setup();

  digitalWrite(pin_CE, 0);
  digitalWrite(pin_OE, 1);
  for (int i = 0; i < data_pins; i++) {
    pinMode(pinO(i), OUTPUT);
  }
}

uint8_t read(uint16_t addr) {
  uint8_t output = 0;
  
  set_addr(addr);
  delay(1);

  for (int i = 0; i < data_pins; i++) {
    output |= (digitalRead(pinO(i)) << i);
  }

  return output;
}

void write(uint16_t addr, uint8_t data) {
  set_addr(addr);
  for (int i = 0; i < data_pins; i++) {
    digitalWrite(pinO(i), (data & (1 << i)) >> i);
  }
  delay(1);

#ifdef LOW_VOLTAGE
  digitalWrite(pin_VPP, 0);
  delay(1);
  digitalWrite(pin_VPP, 1);
  delay(1);
#else
  digitalWrite(pin_CE, 1);
  delay(50);
  digitalWrite(pin_CE, 0);
#endif
}

void write_proc(const uint8_t *data, uint16_t offset, uint16_t len) {
  bool verify_ok = true;

  write_setup();
  ser.println("Ready to write. Apply programming voltage and press Enter.");
  while (ser.read() != '\n');

  ser.println("Writing...");
  for (uint16_t addr = offset; addr < offset + len; addr++) {
    write(addr, data[addr - offset]);
  }

  ser.println("Ready to verify. Reset programming voltage to +5V and press Enter.");
  while (ser.read() != '\n');

  read_setup();
  ser.println("Verifying...");
  for (uint16_t addr = offset; addr < offset + len; addr++) {
    if (read(addr) != data[addr - offset]) {
      verify_ok = false;
    }
  }

  if (verify_ok) {
    ser.println("Verfied OK");
  } else {
    ser.println("Verification failed! Printing dump...");
    dump_proc();
  }
}

void silent_write_proc(const uint8_t *data, uint16_t offset, uint16_t len) {
  write_setup();
  blink_and_wait(500);
  for (uint16_t addr = offset; addr < offset + len; addr++) {
    write(addr, data[addr - offset]);
  }
  blink_and_wait(250);
}

void dump_proc(void) {
  uint8_t data;
  read_setup();
  for (uint16_t addr = 0; addr < (1 << addr_pins_total); addr++) {
    data = read(addr);
    if (data < 0x10) {
      ser.print("0");
    }
    ser.print(read(addr), HEX);
  }
  ser.println();
}

void setup(void) {
  //ser.begin(115200);
  //dump_proc();
  //write_proc(rx, 0, BIN_SZ);
  silent_write_proc(rx, 0, BIN_SZ);
}

void loop(void) {

}
