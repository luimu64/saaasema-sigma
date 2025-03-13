#include <LiquidCrystal.h>
#define u8 uint8_t

constexpr u8 MAGIC_PIN = A0;
constexpr u8 MAX_LETTER_COUNT = 28;
constexpr u8 FIRST_CUSTOM_CHAR_INDEX = 26;
constexpr u8 SCREEN_WIDTH = 20;


struct Vec2u8
{
  u8 x;
  u8 y;
};


constexpr byte swedishLetterA[8] = {
  B00000,
  B10101,
  B00000,
  B01110,
  B00001,
  B01111,
  B10001,
  B01111
};


constexpr byte letterO[8] = {
  B00000,
  B10101,
  B00000,
  B01110,
  B10001,
  B10001,
  B10001,
  B01110
};



const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
Vec2u8 currentPosition = {0,0};
u8 iterator = 0;

Vec2u8 indexToScreenPosition(u8 index)
{
  Vec2u8 result{};
  result.x = index % SCREEN_WIDTH;
  result.y = index / SCREEN_WIDTH;
  return result;
}

void setup()
{
  Serial.begin(9600);
  lcd.begin(20,4);
  lcd.setCursor(currentPosition.x, currentPosition.y);
  lcd.createChar(0, swedishLetterA);
  lcd.createChar(1, letterO);
}

void loop()
{
  lcd.clear();
  // calc position
  currentPosition = indexToScreenPosition(iterator);
  if (currentPosition.y == 1)
  {
    currentPosition.x = SCREEN_WIDTH - 1 - currentPosition.x;
  }

  lcd.setCursor(currentPosition.x, currentPosition.y);
  
  if (iterator >= FIRST_CUSTOM_CHAR_INDEX)
  {
    lcd.write(byte(iterator - FIRST_CUSTOM_CHAR_INDEX));
    Serial.println(currentPosition.x);
    Serial.println(currentPosition.y);
    Serial.println(iterator - FIRST_CUSTOM_CHAR_INDEX);
  }
  else
  {
    const char charToWrite = 65 + iterator;
    lcd.write(charToWrite);
    Serial.println(currentPosition.x);
    Serial.println(currentPosition.y);
    Serial.println(charToWrite);
  }

  iterator = (iterator + 1) % MAX_LETTER_COUNT;
  delay(400); 
}