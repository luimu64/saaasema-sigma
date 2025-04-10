#include <LiquidCrystal.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define outTopic "ICT4_out_2020"        // Aihe, jolle viesti lähetetään
constexpr char *clientId = "sigma-24TIETOA"; // MQTT-clientin tunniste
constexpr char *deviceId = "sigma";          // Laitteen tunniste
constexpr char *deviceSecret = "tamk";

constexpr uint8_t ANALOG_PIN = A0;
constexpr byte DIGITAL_PIN = 2;
constexpr float vcc = 4.7;
constexpr int MAX_COUNT = 10;
constexpr byte customChar[8] = {B00000, B01000, B10100, B01000,
                                B00000, B00000, B00000, B00000};

float digitalDataMap[MAX_COUNT]{};
float analogDataMap[MAX_COUNT]{};
volatile byte pulse = 0;
int analogDataMapIndex = 0;
int digitalDataMapIndex = 0;                            
bool messageDedicator = false;


// SERVER STUFF
byte server[] = {10, 6, 0, 23};               // MQTT-palvelimen IP-osoite
constexpr unsigned int port = 1883;                     // MQTT-palvelimen portti
EthernetClient ethernetClient;                     // Ethernet-kirjaston client-olio
PubSubClient client(server, port, ethernetClient); // PubSubClient-olion luominen
constexpr uint8_t myMAC[6] = {0xA8, 0x61, 0x0A, 0xAE,
                           0x46, 0x8B}; // MAC-osoite Ethernet-liitäntää varten

// LCD SCREEN
constexpr int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const char* jsonLayoutStr = "IOTJS={\"S_name\":\"sigma-ts\",\"S_value\":";


void interrupt() { pulse++; };


int calculateAverage(const float* array)
{
  float a = 0.0;  
  for (int i = 0; i < MAX_COUNT; i++)
  {
    a += array[i];
  }
  return (int)(a / MAX_COUNT);
}

const char *getDirectionStr(int direction) {
  if (direction >= 0 && direction < 45 || direction == 360) {
    return " N";
  } else if (direction >= 45 && direction < 90) {
    return " NE";
  } else if (direction >= 90 && direction < 135) {
    return " E";
  } else if (direction >= 135 && direction < 180) {
    return " SE";
  } else if (direction >= 180 && direction < 225) {
    return " S";
  } else if (direction >= 225 && direction < 270) {
    return " EW";
  } else if (direction >= 270 && direction < 315) {
    return " W";
  } else if (direction >= 315 && direction < 359) {
    return " NW";
  }

  return " NULL";
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



void fetch_IP() {
  bool connectionSuccess = Ethernet.begin(
      myMAC); // Yhdistäminen Ethernet-verkkoon ja tallennetaan yhteyden tila
  if (!connectionSuccess) {
    Serial.println(
        "Failed to access Ethernet controller"); // Jos yhteys ei onnistunut ->
                                                 // yhteysvirheilmoitus
  } else {
    Serial.println("Connected with IP: " +
                   Ethernet.localIP()); // Onnistuessa tulostetaan IP-osoite
  }
}

void send_MQTT_message(String message) {
  if (!client.connected()) { // Tarkistetaan onko yhteys MQTT-brokeriin
                             // muodostettu
    connect_MQTT_server();   // Jos yhteyttä ei ollut, kutsutaan yhdistä
                             // -funktiota
  }
  if (client.connected()) {
    client.publish(outTopic, message.c_str());
    Serial.println("Message sent to MQTT server.");
  } else {
    Serial.println("Failed to send message: not connected to MQTT server.");
  }
}

void connect_MQTT_server() {
  Serial.println("Connecting to MQTT"); // Tulostetaan vähän info-viestiä
  if (client.connect(clientId, deviceId, deviceSecret)) {
    Serial.println("Connected OK"); // Yhdistetty onnistuneesti
  } else {
    Serial.println("Connection failed."); // Yhdistäminen epäonnistui
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(ANALOG_PIN, INPUT);
  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), interrupt, RISING);
  lcd.begin(20, 4);
  lcd.createChar(0, customChar);
  fetch_IP();
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

  const float voltage =
      ((static_cast<float>(analogRead(ANALOG_PIN)) * (vcc / 1023)));
  // values found by educated guesses (randomized testing)
  const float windDirection = max(0.0f, 94.5703 * voltage - 9);
  analogDataMap[analogDataMapIndex] = windDirection;

  analogDataMapIndex = (analogDataMapIndex + 1) % MAX_COUNT;
  digitalDataMap[digitalDataMapIndex] = -0.24f + pulse * 0.699f;
  digitalDataMapIndex = (digitalDataMapIndex + 1) % MAX_COUNT;
  pulse = 0;

  const int direction = calculateAverage(analogDataMap);
  const char *directionStr = getDirectionStr(direction);
  const int speed = calculateAverage(digitalDataMap);

  printData(); // Print data to the screen.

  const String firstMessage = String(jsonLayoutStr + String(direction) + "}");
  const String secondMessage = String(jsonLayoutStr + String(speed) + "}");

  // Kutsutaan MQTT-viestin lähettämis-funktiota
  messageDedicator = !messageDedicator;
  if (messageDedicator){
    send_MQTT_message(firstMessage);
    return;
  }

  send_MQTT_message(secondMessage);
}
