#include <LiquidCrystal.h>

constexpr uint8_t MAGIC_PIN = A0;
constexpr float vcc = 4.77;
constexpr int MAX_COUNT = 10;

volatile byte pulse = 0;

float digitalDataMap[MAX_COUNT]{};
float analogDataMap[MAX_COUNT]{};
int analogDataMapIndex = 0;
int digitalDataMapIndex = 0;

constexpr byte customChar[8] = {
  B00000,
  B01000,
  B10100,
  B01000,
  B00000,
  B00000,
  B00000,
  B00000
};

const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup()
{
  Serial.begin(9600);
  pinMode(MAGIC_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), interupt, RISING);
  lcd.begin(20, 4);
  lcd.createChar(0,customChar);
}

void interupt()
{
  pulse++;
}

int calculateAverage(const float* array)
{
  float a = 0.0;  
  for (int i = 0; i < MAX_COUNT; i++)
  {
    a += array[i];
  }
  return (int)(a / MAX_COUNT);
}

const char* getDirectionStr(const int direction)
{
  if ((direction >= 0 && direction < 45) || (direction >= 315 && direction <= 360))
  {
    return " N";
  }
  else if (direction >= 45  && direction < 90)
  {
    return " NE";
  }
  else if (direction >= 90  && direction < 135)
  {
    return " E";
  }
  else if (direction >= 135  && direction < 180)
  {
    return " SE";
  }
  else if (direction >= 180  && direction < 225)
  {
    return " S";
  }
  else if (direction >= 225  && direction < 270)
  {
    return " SW";
  }
  else if (direction >= 270  && direction < 315)
  {
    return " NW";
  }
  
  return " ERROR Tehee";
}

void printData()
{
  lcd.setCursor(0, 0);
  lcd.print("Tuulensuunta:");
  lcd.setCursor(0, 1);
  const int direction = calculateAverage(analogDataMap);
  lcd.print(direction);
  lcd.write(byte(0));
  lcd.print(getDirectionStr(direction));
  lcd.setCursor(0, 2);
  lcd.print("Tuulennopeus:");
  lcd.setCursor(0, 3);
  lcd.print(calculateAverage(digitalDataMap));
  lcd.print(" m/s");
}

void loop()
{
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis < 1000)
  {
    return; 
  }
  
  lcd.clear();

  lastMillis = millis();

  const float voltage = (static_cast<float>(analogRead(MAGIC_PIN)) / 1023) * vcc;
  const float windDirection = 94.5703 * voltage + 0.1586;
  analogDataMap[analogDataMapIndex] = windDirection;
  Serial.println(windDirection);

  analogDataMapIndex = (analogDataMapIndex + 1) % MAX_COUNT;


  digitalDataMap[digitalDataMapIndex] = -0.24f + pulse * 0.699f;
  // U * 0.24 + F  * 0.699
  

  digitalDataMapIndex = (digitalDataMapIndex + 1) % MAX_COUNT; 
  pulse = 0; 

  printData();  
}
