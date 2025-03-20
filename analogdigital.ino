#include <LiquidCrystal.h>
#include <TimerOne.h>


constexpr uint8_t MAGIC_PIN = A0;
constexpr float vcc = 4.77;
constexpr int MAX_COUNT = 10;

volatile byte time = 0;
volatile byte pulse = 0;

float digitalDataMap[MAX_COUNT]{};
float analogDataMap[MAX_COUNT]{};
int analogDataMapIndex = 0;
int digitalDataMapIndex = 0;

const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup()
{
  Serial.begin(9600);
  pinMode(MAGIC_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), interupt, RISING);
  Timer1.initialize(50000); // 50ms
  Timer1.attachInterrupt(timerRoutine);
  lcd.begin(20, 4);
}

void interupt()
{
  pulse++;
}

void timerRoutine()
{
  time++;
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

void printData()
{
  lcd.setCursor(0, 0);
  lcd.print("Tuulensuunta:");
  lcd.setCursor(0, 1);
  lcd.print(calculateAverage(analogDataMap));
  lcd.setCursor(0, 2);
  lcd.print("Tuulennopeus:");
  lcd.setCursor(0, 3);
  lcd.print(calculateAverage(digitalDataMap));
}

void loop()
{
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis < 1000)
  {
    return; 
  }

  lastMillis = millis();

  const float voltage = (static_cast<float>(analogRead(MAGIC_PIN)) / 1023) * vcc;
  const float windDirection = 94.5703 * voltage + 0.1586;
  analogDataMap[analogDataMapIndex] = windDirection;
  Serial.println(windDirection);

  analogDataMapIndex = (analogDataMapIndex + 1) % MAX_COUNT;

  digitalDataMap[digitalDataMapIndex] = pulse / 5;
  digitalDataMapIndex = (digitalDataMapIndex + 1) % MAX_COUNT; 
  pulse = 0; 

  printData();  
}
