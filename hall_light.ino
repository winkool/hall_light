#include "EspMQTTClient.h"
#include <Adafruit_NeoPixel.h>
#include "env.h"
#include <FastLED.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define FOR_i(from, to) for(int i = (from); i < (to); i++)

#define LED_PIN    14
#define LED_COUNT 158

#define CANDLE_HUE_GAP 2000      // заброс по hue
#define CANDLE_FIRE_STEP 200    // шаг огня
#define CANDLE_HUE_START 0     // начальный цвет огня (0 красный, 80 зелёный, 140 молния, 190 розовый)
#define CANDLE_MIN_BRIGHT -100   // мин. яркость огня
#define CANDLE_MAX_BRIGHT 255  // макс. яркость огня
#define CANDLE_MIN_SAT 250     // мин. насыщенность
#define CANDLE_MAX_SAT 255     // макс. насыщенность
#define CANDLE_DT 20
#define CANDLE_COUNTER_STEP 20

#define OCEAN_HUE_GAP 25000      // заброс по hue
#define OCEAN_FIRE_STEP 20    // шаг огня
#define OCEAN_HUE_START 18000     // начальный цвет огня (0 красный, 80 зелёный, 140 молния, 190 розовый)
#define OCEAN_MIN_BRIGHT -20   // мин. яркость огня
#define OCEAN_MAX_BRIGHT 255  // макс. яркость огня
#define OCEAN_MIN_SAT 245     // мин. насыщенность
#define OCEAN_MAX_SAT 255     // макс. насыщенность
#define OCEAN_DT 2
#define OCEAN_COUNTER_STEP 1

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
EspMQTTClient client(
  WifiSSID,
  WifiPassword,
  Host,  // MQTT Broker server ip
  ClientName
);


String state = "none";
long firstPixelHue = 0;
uint32_t curent_collor = strip.Color(255,   255,   255);
bool curent_state = false;
uint8_t curent_brightness = 200;
void setup()
{
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.
  Serial.begin(115200);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(curent_brightness); // Set BRIGHTNESS to about 1/5 (max = 255)
  strip.fill(strip.Color(255,   255,   255), 0 , LED_COUNT);
  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  client.subscribe("kitchen/table/light/on_off", [](const String & payload) {
    if (payload == "ON") {
      curent_state = true;
      strip.setBrightness(curent_brightness);
      if (state == "none") {
        strip.fill(strip.gamma32(curent_collor), 0 , LED_COUNT);
        strip.show();
      }
    }
    if (payload == "OFF") {
      curent_state = false;
      if (state == "none") {
        strip.setBrightness(0);
        strip.show();
      }
    }
  });

  client.subscribe("kitchen/table/light/brightness", [](const String & payload) {
    curent_state = true;
    curent_brightness = payload.toInt();
    if (curent_state) {
      strip.setBrightness(curent_brightness);
      if (state == "none") {
        strip.fill(strip.gamma32(curent_collor), 0 , LED_COUNT);
        strip.show();
      }
    }

  });

  client.subscribe("kitchen/table/light/color", [](const String & payload) {
    curent_state = true;
    state = "none";
    if (curent_collor != payload.toInt()) {
      FOR_i(0, 100) {
        strip.setBrightness(map(i, 0, 100, curent_brightness, 0));
        strip.show();
        delay(10);
      }
      curent_collor = payload.toInt();
      strip.fill(strip.gamma32(curent_collor), 0 , LED_COUNT);
      FOR_i(0, 100) {
        strip.fill(strip.gamma32(curent_collor), 0 , LED_COUNT);
        strip.setBrightness(map(i, 0, 100, 0, curent_brightness));
        strip.show();
        delay(10);
      }
      strip.show();
    }


  });

  client.subscribe("kitchen/table/light/scene", [](const String & payload) {
    curent_state = true;
    if (payload == "party") {
      //      theaterChaseRainbow(50);
      state = "party";
      firstPixelHue = 0;
    } else {
      state = payload;
    }


  });
  
//  // Subscribe to "mytopic/wildcardtest/#" and display received message to Serial
//  client.subscribe("mytopic/wildcardtest/#", [](const String & topic, const String & payload) {
//    Serial.println("(From wildcard) topic: " + topic + ", payload: " + payload);
//  });
//
//  // Publish a message to "mytopic/test"
//  client.publish("mytopic/test", "This is a message"); // You can activate the retain flag by setting the third parameter to true
//
//  // Execute delayed instructions
//  client.executeDelayed(5 * 1000, []() {
//    client.publish("mytopic/wildcardtest/test123", "This is a message sent 5 seconds later");
//  });
}


void colorWipe(uint32_t color) {
  for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
  }
  strip.show();
}
int counter = 0;
void fireTick(
  int fire_step,
  int hue_start,
  int hue_gap,
  int min_sat,
  int max_sat,
  int min_bright,
  int max_bright,
  int dt,
  int counter_step
) {
  static uint32_t prevTime;

  // двигаем пламя
  if (millis() - prevTime > dt) {
    prevTime = millis();
    int thisPos = 0, lastPos = 0;
    FOR_i(0, LED_COUNT) {
      strip.setPixelColor(
        i,
        getFireColor(
          (inoise8(i * fire_step, counter)),
          hue_start,
          hue_gap,
          min_sat,
          max_sat,
          min_bright,
          max_bright
        )
      );
    }
    counter += counter_step;
    strip.show();
  }
}

// возвращает цвет огня для одного пикселя
uint32_t getFireColor(
  int val,
  int hue_start,
  int hue_gap,
  int min_sat,
  int max_sat,
  int min_bright,
  int max_bright
) {
  // чем больше val, тем сильнее сдвигается цвет, падает насыщеность и растёт яркость
  return strip.ColorHSV(
           hue_start + map(val, 0, 255, 0, hue_gap),                    // H
           constrain(map(val, 0, 255, max_sat, min_sat), 0, 255),       // S
           constrain(map(val, 0, 255, min_bright, max_bright), 0, 255)  // V
         );
}

void loop()
{
  client.loop();
  if (curent_state) {
    if (state == "party" && curent_state) {
      strip.rainbow(firstPixelHue);
      firstPixelHue = firstPixelHue + 256;
      strip.show();
      delay(10);
    }
    if (state == "candle" && curent_state) {
      fireTick(
        CANDLE_FIRE_STEP,
        CANDLE_HUE_START,
        CANDLE_HUE_GAP,
        CANDLE_MIN_SAT,
        CANDLE_MAX_SAT,
        CANDLE_MIN_BRIGHT,
        CANDLE_MAX_BRIGHT,
        CANDLE_DT, //20
        CANDLE_COUNTER_STEP // 20
      );
    }
    if (state == "ocean" && curent_state) {
      fireTick(
        OCEAN_FIRE_STEP,
        OCEAN_HUE_START,
        OCEAN_HUE_GAP,
        OCEAN_MIN_SAT,
        OCEAN_MAX_SAT,
        OCEAN_MIN_BRIGHT,
        OCEAN_MAX_BRIGHT,
        OCEAN_DT, //2
        OCEAN_COUNTER_STEP // 1
      );
    }
  } else {
    strip.setBrightness(0);
    strip.show();
  }

}
