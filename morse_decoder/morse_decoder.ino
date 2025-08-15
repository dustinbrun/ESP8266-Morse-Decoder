#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "private/logo_svntk.h"

#define BUTTON_PIN D5
#define BUTTON_ONBOARD_PIN D6
#define LED_PIN LED_BUILTIN

#define DOT_THRESHOLD 250
#define LETTER_PAUSE 1500
#define TEXT_CLEAR_TIMEOUT 10000

#define DEBOUNCE_DELAY 30

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

String currentMorse = "";
String currentWord = "";

unsigned long pressStartTime = 0;
unsigned long lastInputTime = 0;
unsigned long lastActivityTime = 0;
bool rawButtonState;
bool lastButtonState = HIGH;
bool debouncedButtonState = HIGH;
bool isPressing = false;
int pressDuration = 0;
unsigned long lastDebounceTime = 0;

struct MorseSymbol {
  const char* code;
  char letter;
};

MorseSymbol morseTable[] = {
  {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},
  {".", 'E'}, {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'},
  {"..", 'I'}, {".---", 'J'}, {"-.-", 'K'}, {".-..", 'L'},
  {"--", 'M'}, {"-.", 'N'}, {"---", 'O'}, {".--.", 'P'},
  {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
  {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'},
  {"-.--", 'Y'}, {"--..", 'Z'},
  {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
  {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'},
  {"---..", '8'}, {"----.", '9'}
};

char decodeMorse(String code) {
  for (int i = 0; i < sizeof(morseTable) / sizeof(morseTable[0]); i++) {
    if (code.equals(morseTable[i].code)) {
      return morseTable[i].letter;
    }
  }
  return '?';
}

void drawStatus() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_fub17_tr);

  u8g2.drawXBMP(128 - 55, 0, 55, 16, epd_bitmap_logo);  // top right corner

  // Zeige Morsezeichen
  u8g2.drawStr(10, 20, currentMorse.c_str());

  // Zeige dekodierten Text
  u8g2.drawStr(10, 45, currentWord.c_str());

  // Fortschrittsbalken immer anzeigen (auch wenn nicht gedrückt)
  u8g2.drawFrame(0, 57, 127, 7);
  if (isPressing) {
    int barWidth = map(millis() - pressStartTime, 0, DOT_THRESHOLD, 0, 128);
    if (barWidth > 128) barWidth = 128;
    u8g2.drawBox(0, 57, barWidth, 7);
  }

  u8g2.sendBuffer();
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_ONBOARD_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  u8g2.begin();
  u8g2.enableUTF8Print();
  drawStatus();
  Serial.println("Morse Decoder bereit");
}

void loop() {
  rawButtonState = digitalRead(BUTTON_ONBOARD_PIN);
  rawButtonState &= digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  digitalWrite(LED_PIN, debouncedButtonState);

  if (rawButtonState != lastButtonState) {
    lastDebounceTime = now;
  }
  lastButtonState = rawButtonState;

  if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {

    if (rawButtonState != debouncedButtonState) {
      debouncedButtonState = rawButtonState;

      // Taster gedrückt
      if (debouncedButtonState == LOW) {
        pressStartTime = now;
        isPressing = true;
      }
      // Taster losgelassen
      else {
        pressDuration = now - pressStartTime;
        lastInputTime = now;
        lastActivityTime = now;
        isPressing = false;

        if (pressDuration < DOT_THRESHOLD) {
          currentMorse += ".";
          Serial.println("Eingabe: Punkt (.)");
        } else {
          currentMorse += "-";
          Serial.println("Eingabe: Strich (-)");
        }

        Serial.print("Aktueller Morsecode: ");
        Serial.println(currentMorse);
      }
    }

    // Buchstabe fertig
    if (currentMorse.length() > 0 && (now - lastInputTime > LETTER_PAUSE)) {
      char decoded = decodeMorse(currentMorse);
      Serial.print("Morse abgeschlossen: ");
      Serial.println(currentMorse);
      Serial.print("Dekodiert: ");
      Serial.println(decoded);

      if (currentMorse.length() > 5) {
        Serial.println("Morsecode zu lang - Eingabe zurückgesetzt.");
        currentMorse = "";
        currentWord = "";
        lastActivityTime = millis();
      }
      currentMorse = "";

      if (decoded != '?') {
        currentWord += decoded;
        if (currentWord.length() > 7) {
          currentWord = currentWord.substring(currentWord.length() - 7);
        }
      }

      lastActivityTime = now;
      Serial.print("Aktueller Text: ");
      Serial.println(currentWord);
    }
  }

  // Text löschen nach Timeout
  if (currentWord.length() > 0 && (now - lastActivityTime > TEXT_CLEAR_TIMEOUT)) {
    currentWord = "";
    Serial.println("Text automatisch gelöscht wegen Inaktivität.");
  }

  drawStatus();
}

