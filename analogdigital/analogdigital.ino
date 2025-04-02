#include <Ethernet.h>
#include <LiquidCrystal.h>
#include <PubSubClient.h>

constexpr uint8_t MAGIC_PIN = A0;
constexpr float vcc = 4.77;
constexpr int MAX_COUNT = 10;

volatile byte pulse = 0;

float digitalDataMap[MAX_COUNT]{};
float analogDataMap[MAX_COUNT]{};
int analogDataMapIndex = 0;
int digitalDataMapIndex = 0;
char buffer[100];

constexpr byte customChar[8] = {B00000, B01000, B10100, B01000,
                                B00000, B00000, B00000, B00000};

const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// SERVER STUFF
byte server[] = {10, 6, 0, 23};               // MQTT-palvelimen IP-osoite
unsigned int Port = 1883;                     // MQTT-palvelimen portti
EthernetClient ethClient;                     // Ethernet-kirjaston client-olio
PubSubClient client(server, Port, ethClient); // PubSubClient-olion luominen

#define outTopic "ICT_2021" // Aihe, jolle viesti lähetetään

static uint8_t mymac[6] = {0xA8, 0x61, 0x0A, 0xAE,
                           0x46, 0x8B}; // MAC-osoite Ethernet-liitäntää varten

char *clientId = "sigma-24TIETOA"; // MQTT-clientin tunniste
char *deviceId = "sigma";          // Laitteen tunniste
char *deviceSecret = "tamk";

void interupt() { pulse++; }

int calculateAverage(const float *array) {
  float a = 0.0;
  for (int i = 0; i < MAX_COUNT; i++) {
    a += array[i];
  }
  return (int)(a / MAX_COUNT);
}

const char *getDirectionStr(const int direction) {
  if ((direction >= 0 && direction < 45) ||
      (direction >= 315 && direction <= 360)) {
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
    return " SW";
  } else if (direction >= 270 && direction < 315) {
    return " NW";
  }

  return " NULL";
}

void printData(int direction, char *directionStr, int speed) {
  lcd.setCursor(0, 0);
  lcd.print("Tuulensuunta:");
  lcd.setCursor(0, 1);
  lcd.print(direction);
  lcd.write(byte(0));
  lcd.print(directionStr);
  lcd.setCursor(0, 2);
  lcd.print("Tuulennopeus:");
  lcd.setCursor(0, 3);
  lcd.print(speed);
  lcd.print(" m/s");
}

void fetch_IP() {
  bool connectionSuccess = Ethernet.begin(
      mymac); // Yhdistäminen Ethernet-verkkoon ja tallennetaan yhteyden tila
  if (!connectionSuccess) {
    Serial.println(
        "Failed to access Ethernet controller"); // Jos yhteys ei onnistunut ->
                                                 // yhteysvirheilmoitus
  } else {
    Serial.println("Connected with IP: " +
                   Ethernet.localIP()); // Onnistuessa tulostetaan IP-osoite
  }
}

void send_MQTT_message(char *message) {
  if (!client.connected()) { // Tarkistetaan onko yhteys MQTT-brokeriin
                             // muodostettu
    connect_MQTT_server();   // Jos yhteyttä ei ollut, kutsutaan yhdistä
                             // -funktiota
  }
  if (client.connected()) { // Jos yhteys on muodostettu
    client.publish(outTopic,
                   message); // Lähetetään viesti MQTT-brokerille
    Serial.println(
        "Message sent to MQTT server."); // Tulostetaan viesti onnistuneesta
                                         // lähettämisestä
  } else {
    Serial.println(
        "Failed to send message: not connected to MQTT server."); // Ei yhteyttä
                                                                  // ->
                                                                  // Yhteysvirheilmoitus
  }
}

void connect_MQTT_server() {
  Serial.println("Connecting to MQTT"); // Tulostetaan vähän info-viestiä
  if (client.connect(
          clientId)) { // Tarkistetaan saadaanko yhteys MQTT-brokeriin
    Serial.println("Connected OK"); // Yhdistetty onnistuneesti
  } else {
    Serial.println("Connection failed."); // Yhdistäminen epäonnistui
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(MAGIC_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), interupt, RISING);
  lcd.begin(20, 4);
  lcd.createChar(0, customChar);
  fetch_IP();
}

void loop() {
  static unsigned long lastMillis = 0;

  if (millis() - lastMillis < 1000) {
    return;
  }

  lcd.clear();

  lastMillis = millis();

  const float voltage =
      (static_cast<float>(analogRead(MAGIC_PIN)) / 1023) * vcc;
  const float windDirection = 94.5703 * voltage + 0.1586;
  analogDataMap[analogDataMapIndex] = windDirection;
  Serial.println(windDirection);

  analogDataMapIndex = (analogDataMapIndex + 1) % MAX_COUNT;
  digitalDataMap[digitalDataMapIndex] = -0.24f + pulse * 0.699f;
  digitalDataMapIndex = (digitalDataMapIndex + 1) % MAX_COUNT;
  pulse = 0;

  const int direction = calculateAverage(analogDataMap);
  const char *directionStr = getDirectionStr(direction);
  const int speed = calculateAverage(digitalDataMap);

  printData(direction, directionStr, speed);
  snprintf(buffer, sizeof(buffer),
           "IOTJS={\"S_name1\":\"tuulensuunta\",\"S_value1\":%d,\"S_name2\":"
           "\"tuulensuunta-kirjain\",\"S_value2\":%s,\"S_name3\":"
           "\"tuulennopeus\",\"S_value3\":%d}",
           direction, *directionStr, speed);

  send_MQTT_message(buffer); // Kutsutaan MQTT-viestin lähettämis-funktiota
}
