#include <Arduino.h>
//#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1X15.h>
#include <SPI.h>
#include "Adafruit_ST7796S_kbv.h"
#include "XPT2046.h"

//#define NUM_LEDS 1
//#define PIN_LED_DATA 20

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define SEND_ALL_THRESHOLD 500

#define TFT_CS 4
#define TFT_DC 6
#define TFT_RST -1
#define TOUCH_CS_PIN 8
#define TOUCH_IRQ_PIN 19

bool previous_trigger_state = false;
bool previous_push_state = false;
bool previous_push2_state = false;
bool previous_follow_state = false;

bool identified = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_ADS1115 ads1115;
Adafruit_ST7796S_kbv tft = Adafruit_ST7796S_kbv(TFT_CS, TFT_DC, TFT_RST);
XPT2046 touch(TOUCH_CS_PIN, TOUCH_IRQ_PIN);

uint16_t ads_pins[] = {ADS1X15_REG_CONFIG_MUX_SINGLE_0, ADS1X15_REG_CONFIG_MUX_SINGLE_1, ADS1X15_REG_CONFIG_MUX_SINGLE_2};
int16_t ads_states[] = {0,0,0};
int ads_index = 0;

int send_all = 0;

//CRGB leds[NUM_LEDS];

void display_text(String text) { 
  display.clearDisplay();  // Clear buffer
  display.setTextSize(2);  // Text size
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println(text);
  display.display(); // Show text on screen
}

void read_next_ads() {
  ads_index = (ads_index + 1) % 3;
  ads1115.startADCReading(ads_pins[ads_index], false);
}

void setup()
{
  Serial.begin(115200); //This pipes to the serial monitor

  pinMode(5, INPUT);
  pinMode(7, INPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  ads1115.begin();

  read_next_ads();
  display_text("");

  tft.begin();
  tft.fillScreen(ST7796S_BLACK);
  tft.setTextColor(ST7796S_WHITE, ST7796S_BLACK);
  tft.setRotation(3);
  //tft.drawBitmap(0, 0, epd_bitmap_icon_256, 32, 32, 0xFFFF);

  touch.begin();
  touch.setRotation(3);              // Set the rotation
  //touch.setCalibration(CALIBRATION); // Set the calibration matrix
  touch.setSampleCount(20);          // Set the sample count
  touch.setDebounceTimeout(100);     // Set the debounce timeout
  touch.setTouchPressure(3.5);       // Set the touch pressure
  touch.setDeadZone(50);             // Set the dead zone
  touch.setPowerDown(true);          // Set the power-down state

  //FastLED.addLeds<WS2812, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
}

void exec_command(String cmd) {
  if (cmd == "identify simflow serial") {
    Serial.println("IDENT 123456789");
    identified = true;
  } else if (!identified) {
    Serial.println("not identified");
    return;
  }

  if (cmd.startsWith("disp ")) {
    display_text(cmd.substring(5));
  } else if (cmd.startsWith("tft cursor ")) {
    String coords = cmd.substring(11);
    int index = coords.indexOf(" ");
    int x = coords.substring(0, index).toInt();
    int y = coords.substring(index + 1).toInt();
    tft.setCursor(x, y);
  } else if (cmd.startsWith("tft clear")) {
    tft.fillScreen(ST7796S_BLACK);
    tft.setTextColor(ST7796S_WHITE, ST7796S_BLACK);
  } else if (cmd.startsWith("tft println ")) {
    String writeout = cmd.substring(12);
    writeout.concat("                                        ");
    tft.println(writeout.substring(0, 40));
  } else if (cmd.startsWith("tft print ")) {
    tft.print(cmd.substring(10));
  } else if (cmd.startsWith("tft color ")) {
    String colors = cmd.substring(10);
    int index = colors.indexOf(" ");
    int fg = colors.substring(0, index).toInt();
    int bg = colors.substring(index + 1).toInt();
    tft.setTextColor(fg, bg);
  } else if (cmd.startsWith("tft fontsize ")) {
    tft.setTextSize(cmd.substring(13).toInt());
  }
}

void loop()
{
  if (Serial.available() > 0) {
    String recvLine = Serial.readStringUntil('\n');
    if (recvLine.length() > 0) {
      exec_command(recvLine);
    }
  }

  if (!identified) {
    return;
  }

  bool push_state = digitalRead(9);
  bool push2_state = digitalRead(7);
  bool trigger_state = digitalRead(10);
  bool follow_state = digitalRead(5);

  // rising push state
  if (push_state && !previous_push_state) {
    Serial.println("PUSH 9");
    delay(1);
  }

  // rising push state
  if (push2_state && !previous_push2_state) {
    Serial.println("PUSH 7");
    delay(1);
  }

  // rising trigger state
  if (trigger_state && !previous_trigger_state) {
    int increment = 0;
    if (trigger_state == follow_state) {
      Serial.println("DEC 10");
    } else {
      Serial.println("INC 10");
    }
    delay(1);
  }

  previous_push_state = push_state;
  previous_trigger_state = trigger_state;
  previous_follow_state = follow_state;
  previous_push2_state = push2_state;

  if (ads1115.conversionComplete()) {
    int16_t prev_state = ads_states[ads_index];
    int16_t curr_state = ads1115.getLastConversionResults();
    if (abs(prev_state - curr_state) > 100 || send_all > SEND_ALL_THRESHOLD) {
      ads_states[ads_index] = curr_state;
      Serial.print("A");
      Serial.print(ads_index);
      Serial.print(": ");
      Serial.println(curr_state);
    }
    read_next_ads();
    send_all += 1;

    if (send_all > SEND_ALL_THRESHOLD + 3) {
      send_all = 0;
    }
  }

  if (touch.touched() && touch.released()) {
    // Get the touch position
    XPT2046::Point position = touch.getTouchPosition();

    // Check if the touch position is valid
    if (touch.valid(position)) {
      // Print the position to the Serial Monitor
      Serial.println("touch " + String(position.x) + " " + String(position.y));
    }
  }
}
