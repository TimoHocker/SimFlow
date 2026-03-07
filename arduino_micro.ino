#include <splash.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1X15.h>
#include <SPI.h>
#include "Adafruit_ST7796S_kbv.h"
#include <TFT_Touch.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define SEND_ALL_THRESHOLD 500

#define TFT_CS 4
#define TFT_DC 6
#define TFT_RST -1
#define TOUCH_CS 8

// These are the default min and maximum values, set to 0 and 4095 to test the screen
#define HMIN 0
#define HMAX 4095
#define VMIN 0
#define VMAX 4095
#define XYSWAP 0 // 0 or 1

// This is the screen size for the raw to coordinate transformation
// width and height specified for landscape orientation
#define HRES 320 /* Default screen resulution for X axis */
#define VRES 480 /* Default screen resulution for Y axis */

bool previous_trigger_state = false;
bool previous_push_state = false;
bool previous_push2_state = false;
bool previous_follow_state = false;

bool identified = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_ADS1115 ads1115;
Adafruit_ST7796S_kbv tft = Adafruit_ST7796S_kbv(TFT_CS, TFT_DC, TFT_RST);
TFT_Touch touch = TFT_Touch(TOUCH_CS, 15, 16, 14);

uint16_t ads_pins[] = {ADS1X15_REG_CONFIG_MUX_SINGLE_0, ADS1X15_REG_CONFIG_MUX_SINGLE_1, ADS1X15_REG_CONFIG_MUX_SINGLE_2};
int16_t ads_states[] = {0,0,0};
int ads_index = 0;

int send_all = 0;

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
  Serial.begin(57600); //This pipes to the serial monitor

  pinMode(7, INPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  pinMode(5, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println("SSD1306 allocation failed");
      for (;;);
  }

  if (!ads1115.begin()) {
      Serial.println("ASD1115 allocation failed");
      for (;;);
  }

  read_next_ads();
  display_text("");

  tft.begin();
  tft.fillScreen(ST7796S_BLACK);
  tft.setTextColor(ST7796S_WHITE, ST7796S_BLACK);
  tft.setRotation(-1);

  touch.setCal(HMIN, HMAX, VMIN, VMAX, HRES, VRES, XYSWAP); // Raw xmin, xmax, ymin, ymax, width, height
  touch.setRotation(-1);
}

void exec_command(String cmd) {
  if (cmd == "identify simflow serial") {
    Serial.println("IDENT 123456789");
    identified = true;
  } else if (!identified) {
    Serial.println("not identified");
    return;
  }

  Serial.print("RECEIVED: ");
  Serial.println(cmd);
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
  } else if (cmd.startsWith("tft color ")) {
    tft.setTextColor(cmd.substring(10).toInt());
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

  if (touch.Pressed()) // Note this function updates coordinates stored within library variables
  {
    /* Read the current X and Y axis as raw co-ordinates at the last touch time*/
    // The values returned were captured when Pressed() was called!
    
    unsigned int X_Raw = touch.RawX();
    unsigned int Y_Raw = touch.RawY();

    /* Output the results to the serial port */
    Serial.print("Touch ");
    Serial.print(X_Raw);
    Serial.print(",");
    Serial.println(Y_Raw);
  }
}
