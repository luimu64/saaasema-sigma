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

enum Tabs : uint8_t {
  SERVER_INFO,
  DATA,
  STATS,
  CUSTOM
};

struct StatsData {
  float lowest;
  float highest;
};

StatsData digitalStats{};
StatsData analogStats{};

float digitalDataMap[MAX_COUNT]{};
float analogDataMap[MAX_COUNT]{};
volatile byte pulse = 0;
int analogDataMapIndex = 0;
int digitalDataMapIndex = 0;                            
bool messageDedicator = false;
uint8_t currentTab = Tabs::DATA;

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


// KEYBOARD
enum ButtonPins : uint8_t {
 BUTTON_DOWN = 0
 ONE = 1
 TWO = 2
 THREE = 3
 A = 4 
};




//Json layout string
const char* jsonLayoutStr = "IOTJS={\"S_name\":\"sigma-ts\",\"S_value\":";

// CUSTOM
struct Vec2u8 {
  uint8_t x;
  uint8_t y;
};

constexpr byte swedishLetterA[8] = { B00000, B10101, B00000, B01110,
                                     B00001, B01111, B10001, B01111};

constexpr byte letterO[8] = { B00000, B10101, B00000, B01110,
                              B10001, B10001, B10001, B01110};

constexpr uint8_t MAX_LETTER_COUNT = 28;
constexpr uint8_t FIRST_CUSTOM_CHAR_INDEX = 26;
constexpr uint8_t SCREEN_WIDTH = 20;
constexpr uint8_t TOTAL_SCREEN_SIZE = SCREEN_WIDTH*4;

Vec2u8 currentPosition = {0, 0};
uint8_t iterator = 0;
uint8_t iteratorChar = 0;


Vec2u8 indexToScreenPosition(uint8_t index) {
  Vec2u8 result{};
  result.x = index % SCREEN_WIDTH;
  result.y = index / SCREEN_WIDTH;
  return result;
}

void interrupt() { pulse++; };


void render(const int direction, const char* directionStr, const int speed) {
  switch (currentTab) {
    case Tabs::SERVER_INFO:
      printServerInfo();
      break;
    case Tabs::DATA:
      printData(direction, directionStr, speed);
      break;
    case Tabs::STATS:
      printStats();
    case Tabs::CUSTOM:
      printCustom();
    default:
      return;
  }
}

void printCustom() {
  currentPosition = indexToScreenPosition(iterator);

  int rowStartDirection = currentPosition.y % 4;
  if (rowStartDirection == 1 || rowStartDirection == 3) {
    currentPosition.x = SCREEN_WIDTH - 1 - currentPosition.x;
  }


  lcd.setCursor(currentPosition.x, currentPosition.y);

  if (iteratorChar >= FIRST_CUSTOM_CHAR_INDEX) {
    lcd.write(byte(iteratorChar - FIRST_CUSTOM_CHAR_INDEX)+1); // INFO: JOS SPESSU KIRJAIMET MENEVÄT HUPSUSTI NIIN POISTA TÄMÄ "+ 1" KOHTA.
  }
  else {
    const char charToWrite = 65 + (iteratorChar % 26); // Restart from A after Z
    lcd.write(charToWrite);
  }
  iteratorChar = (iteratorChar + 1) % MAX_LETTER_COUNT; 
  iterator = (iterator + 1) % TOTAL_SCREEN_SIZE;
}


int calculateAverage(const float* array) {
  float a = 0.0;  
  for (int i = 0; i < MAX_COUNT; i++) {
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


void updateStatus(StatsData& data, const float input) {
  if (input < data.lowest) data.lowest = input;
  if (input >  data.highest) data.highest = input;
}

char* getConnectionStusMessage() {
  if (client.connected()) {
    return "Online";
  }
  return "Offline";
}

void printServerInfo() {
  lcd.setCursor(0, 0);
  lcd.print("IP: ");
  lcd.print(Ethernet.localIP());
  lcd.setCursor(0, 1);
  lcd.print("MQTT Yhteys: ");
  lcd.print(getConnectionStusMessage());
}

void printStats() {
  lcd.setCursor(0, 0);
  lcd.print("Digital Max: ");
  lcd.print(digitalStats.highest);
  lcd.setCursor(0, 1);
  lcd.print("Digital Low: ");
  lcd.print(digitalStats.lowest);
  lcd.setCursor(0, 2);
  lcd.print("Analog Max: ");
  lcd.print(analogStats.highest);
  lcd.setCursor(0, 3);
  lcd.print("Analog Low: ");
  lcd.print(analogStats.lowest);
}

void printData(const int direction, const char* directionStr, const int speed) {
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

  pinMode(ButtonPins::BUTTON_DOWN, INPUT_PULLUP);
  pinMode(ButtonPins::ONE, INPUT_PULLUP);
  pinMode(ButtonPins::TWO, INPUT_PULLUP);
  pinMode(ButtonPins::THREE, INPUT_PULLUP);
  pinMode(ButtonPins::A, INPUT_PULLUP);


  attachInterrupt(digitalPinToInterrupt(2), interrupt, RISING);
  lcd.begin(20, 4);
  lcd.createChar(0, customChar);
  lcd.createChar(1, swedishLetterA);
  lcd.createChar(2, letterO);
  fetch_IP();
}

void checkKeyPressed() {
  if (digitalRead(ButtonPins::BUTTON_DOWN) == HIGH)
  {
    return; 
  }

  if (digitalRead(ButtonPins::ONE) == LOW) {
    currentTab = Tabs::SERVER_INFO;
  } 
  else if (digitalRead(ButtonPins::TWO) == LOW) {
    currentTab = Tabs::DATA;
  } 
  else if (digitalRead(ButtonPins::THREE) == LOW) {
    currentTab = Tabs::STATS;
  } 
  else if (digitalRead(ButtonPins::A) == LOW) {
    currentTab = Tabs::CUSTOM;
  }

}



void loop() {
  static unsigned long lastMillis = 0;

  if (millis() - lastMillis < 1000)
  {
    return; 
  }
  
  lcd.clear();

  lastMillis = millis();

  // check key press
  checkKeyPressed();

  const float voltage =
      ((static_cast<float>(analogRead(ANALOG_PIN)) * (vcc / 1023)));
  // values found by educated guesses (randomized testing)
  const float windDirection = max(0.0f, 94.5703 * voltage - 9);
  analogDataMap[analogDataMapIndex] = windDirection;
  updateStatus(analogStats, windDirection);
  analogDataMapIndex = (analogDataMapIndex + 1) % MAX_COUNT;
  
  const float digitalOutput = -0.24f + pulse * 0.699f;
  digitalDataMap[digitalDataMapIndex] = digitalOutput;
  updateStatus(digitalStats, digitalOutput);
  digitalDataMapIndex = (digitalDataMapIndex + 1) % MAX_COUNT;
  pulse = 0;

  const int direction = calculateAverage(analogDataMap);
  const char *directionStr = getDirectionStr(direction);
  const int speed = calculateAverage(digitalDataMap);

  render(direction, directionStr, speed);

  const String firstMessage = String(jsonLayoutStr + String(direction) + "}");
  const String secondMessage = String(jsonLayoutStr + String(speed) + "}");

  // Kutsutaan MQTT-viestin lähettämis-funktiota
  messageDedicator = !messageDedicator;
  if (messageDedicator) {
    send_MQTT_message(firstMessage);
    return;
  }

  send_MQTT_message(secondMessage);
}
