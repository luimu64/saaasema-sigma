constexpr uint8_t MAGIC_PIN = A0;


float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup()
{
  Serial.begin(9600);
  pinMode(MAGIC_PIN,INPUT);
}

void loop()
{
  constexpr float vcc = 4.77; // Replace this with vcc measured from arduino 5V pin when connected to usb

  const float voltage = (static_cast<float>(analogRead(MAGIC_PIN)) / 1023) * vcc;
  const float windDirection = 94.5703 * voltage + 0.1586; 

  Serial.print("Wind Direction: ");
  Serial.println(windDirection);
  delay(100);
}
