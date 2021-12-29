#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal.h>
#include <PubSubClient.h>

//SSID e PASS del wifi
#define WIFI_SSID "eir-4927"
#define WIFI_PASSWORD "89574951881699087854"

//indirizzo e porta del server MQTT
#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883

//robe di telegram
#define BOTtoken "5039029687:AAE1sgr9d_WOaiKACnrFCg6RyBhlpFtysj0"  // your Bot Token (Get from Botfather)
#define CHATS_ID "485901444"

//Numero massimo di sensori collegabili
#define MAX_SENSOR_NUM 10

//Pin sul quale è "collegato" il bottone
#define BUTTON_PIN 22
//Frequenza di aggiornamento dello schermo LCD (in ms)
#define REFRESH_RATE 200

//Pin che accende il led rosso per segnalare l'allarme relativo alla temperatura
#define TEMPERATURE_ALARM_PIN 16
//Pin che accende il led verde per segnalare l'allarme relativo all'umidità
#define HUMIDITY_ALARM_PIN 17

/**
 * Struttura nella quale vengono salvati i dati relativi ad ogni
 * sensore connesso al sistema
 */
struct Sensor {
  String name;
  int lastConnection;
  String temperature;
  String humidity;
  String brightness;
};
//Lista dove vengono salvati tutti i sensori connessi
Sensor sensors[MAX_SENSOR_NUM];
/**
 * Indice che "scorre" sulla lista definita sopra. Questo indice punta
 * al sensore del quale si vogliono scrivere le informazioni sullo schermo
 * LCD. Ogni volta che il bottone viene premuto il suo valore è incrementato,
 * e nel caso superi il limite (arraySize, definito sotto) allora riparte da 0
 */
int selectedIndex = -1;
/**
 * Variabile che salva il numero di sensori attualmente attivi
 * Viene incrementata ogni volta che un nuovo sensore si connette
 */
int arraySize = 0;

//inizializzazione del display
LiquidCrystal lcd(14, 12, 33, 25, 26, 27);

WiFiClientSecure clientSec;
UniversalTelegramBot bot(BOTtoken, clientSec);

int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

//robe di MQTT
WiFiClient espClient;
PubSubClient client(espClient);

//robe del display

byte lampadina[] = {
  B00000,
  B01110,
  B10111,
  B10001,
  B10001,
  B01010,
  B01110,
  B00100
};
byte termometro[8] = {
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110
};
byte goccia[8] = {
  B00100,
  B00100,
  B01010,
  B01010,
  B10001,
  B10001,
  B10001,
  B01110,
};
byte warning[8] = {
  B01110,
  B10001,
  B10101,
  B10101,
  B10001,
  B10101,
  B10001,
  B01110,
};
//delay for some refresh 
int delayAfterOn = 100;


void connectToWifi() {
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  #ifdef ESP32
    clientSec.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");  
  }

  Serial.println("");
  Serial.println("Wifi connected");
  Serial.println("Indirizzo IP: ");
  Serial.println(WiFi.localIP());
}


bool buttonPressed = false;
void IRAM_ATTR buttonPressedInterrupt() {
  buttonPressed = true;
}

int startMillisDrawTask = 0;
/**
 * Funzione che gestisce la scrittura sul display LCD
 */
void DrawTaskCode(void* param) {
  for(;;) {
    delay(1);
    int currentMillis = millis();
  
    if( currentMillis - startMillisDrawTask >= REFRESH_RATE){
      if(buttonPressed && selectedIndex != -1) {
        buttonPressed = false;
        selectedIndex = (selectedIndex + 1) % arraySize;
      }
    
      startMillisDrawTask = currentMillis;
      draw();
    }
  }
}
TaskHandle_t DrawTask;

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  int firstIndex = String(topic).indexOf('/');
  String rightTopic = String(topic).substring(firstIndex + 1);
  int secondIndex = rightTopic.indexOf('/');
  String espId = rightTopic.substring(0, secondIndex);
  String param = String(rightTopic).substring(secondIndex + 1);

  Sensor s = {"", 0, "----", "--", "---"};
  Sensor *sensor = &s;
  for(int i = 0; i < arraySize; i++) {
    if(sensors[i].name == espId) {
      sensor = &sensors[i];
    }
  }

  if(sensor->name == "" && arraySize < MAX_SENSOR_NUM) {
    sensor->name = espId;
    sensors[arraySize] = *sensor;
    arraySize++;

    if(arraySize == 1) {
       selectedIndex = 0;
    }
  }

  sensor->lastConnection = millis();
  if(param == "temperature") {
    messageTemp.remove(4);
    sensor->temperature = messageTemp;
  }
  if(param == "humidity") {
    sensor->humidity = messageTemp;
  }
  if(param == "brightness") {
    sensor->brightness = messageTemp;
  }
}

void draw() {
  lcd.setCursor(0, 0);
  if(selectedIndex == -1) {
    lcd.print("Waiting for     ");
    lcd.setCursor(0, 1);
    lcd.print("connections     ");
    return;  
  }

  Sensor sensor = sensors[selectedIndex];
  if(sensor.temperature.toFloat() >= 22) {
    digitalWrite(TEMPERATURE_ALARM_PIN, HIGH);
  } else {
    digitalWrite(TEMPERATURE_ALARM_PIN, LOW);
  }

  if(sensor.humidity.toFloat() >= 65) {
    digitalWrite(HUMIDITY_ALARM_PIN, HIGH);
  } else {
    digitalWrite(HUMIDITY_ALARM_PIN, LOW);
  }
  
  lcd.print(sensor.name);
  lcd.print(" ");
  lcd.write(1);
  lcd.print(" " + sensor.temperature);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.write(2);
  lcd.print(" " + sensor.humidity + "%  ");

  lcd.setCursor(6, 1);
  lcd.write(3);
  lcd.print(" " + sensor.brightness + "Lum ");

  /**
   * Se il sensore non si connette da più di 30 secondi, viene segnalato tramite
   * un'icona nell'angolo in alto a destra del display
   * Nel ramo else viene scritto uno spazio bianco per "pulire" nel caso l'icona
   * vada eliminata
   */
  lcd.setCursor(15, 0);
  if(millis() - sensor.lastConnection > 30 * 1000) {
    lcd.write(4);
  } else {
    lcd.print(" ");
  }
}

void connectToMQTTServer() {
  //Cicla fino a che non si connette
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    //Prova a connettersi
    if (client.connect("ESP_GATEWAY")) {
      Serial.println("connected");
      //Se la connessiona va a buon fine, si fa la subscribe al topic
      client.subscribe("aaabbbccc/#");
    } else {
      //Se la connessione non va a buon fine, si scrivono le informazioni di errore e si riprova dopo 1 secondo
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      delay(1000);
    }
  }
}

/**
 * Funzione che gestisce il bot telegram
 */
void TelegramBotTaskCode(void* param) {
  for(;;) {
    delay(1);
    if (millis() > lastTimeBotRan + botRequestDelay)  {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while(numNewMessages) {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      lastTimeBotRan = millis();
    }
  }
}
TaskHandle_t TelegramBotTask;

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if(String(CHATS_ID).indexOf(chat_id) == -1){
      bot.sendMessage(chat_id, "Unauthorized user " + chat_id, "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following command to know the connected sensor.\n\n";
      welcome += "/sensors\n";
      bot.sendMessage(chat_id, welcome, "");

      continue;
    }

    if(text == "/sensors") {
      if(arraySize == 0) {
        bot.sendMessage(chat_id, "No sensor is connected. Try later", "");
        continue;
      }

      String devicesList = "Sensors connected:\n";
      for(int i = 0; i < arraySize; i++) {
        devicesList += "/" + sensors[i].name + "\n";
      }

      bot.sendMessage(chat_id, devicesList, "");
      continue;
    }

    String sensorName = text.substring(1);
    Sensor sensor = {"", 0, "", "", ""};
    for(int i = 0; i < arraySize; i++) {
        if(sensors[i].name == sensorName) {
          sensor = sensors[i];
          break;
        }
    }

    String sensorInfo;
    if(sensor.name == "") {
      sensorInfo = "Sensor with name " + sensorName + " not found";
    } else {
      sensorInfo = "Data of " + sensorName + "\n";
      sensorInfo += "Temperature: " + sensor.temperature + "°C\n";
      sensorInfo += "Humidity: " + sensor.humidity + "%\n";
      sensorInfo += "Brightness: " + sensor.brightness + "Lum\n";
    }

    bot.sendMessage(chat_id, sensorInfo, "");
  }
}

void setup() {
  Serial.begin(115200);

  //Pin degli allarmi settati come output
  pinMode(TEMPERATURE_ALARM_PIN, OUTPUT);
  pinMode(HUMIDITY_ALARM_PIN, OUTPUT);

  //Per sicurezza li setto subito a low, se dovranno essere high verrano impostati nella funzione draw
  digitalWrite(TEMPERATURE_ALARM_PIN, LOW);
  digitalWrite(HUMIDITY_ALARM_PIN, LOW);
  
  //pin del bottone come input
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, buttonPressedInterrupt, FALLING);
  
  // Setto il numero di righe e di colonne del display
  lcd.begin(16, 2);
  //Messaggio iniziale
  lcd.print("Welcome...");
  lcd.setCursor(0, 1);
  lcd.print("WIFI Thermostat");
  //creo i caratteri speciali per LCD
  lcd.createChar(1, termometro);
  lcd.createChar(2, goccia);
  lcd.createChar(3, lampadina);
  lcd.createChar(4, warning);
  //Connessione al wifi
  connectToWifi();
  delay(delayAfterOn);

  //connessione a MQTT
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);

  draw();

  //Istanziamento del task relativo alla gestione della scrittura sul display sul primo core del processore
  xTaskCreatePinnedToCore(DrawTaskCode, "DrawTask", 10000, NULL, 1, &DrawTask, 0);
  //Istanziamento del task relativo alla gestione del bot telegram sul secondo core del processore
  xTaskCreatePinnedToCore(TelegramBotTaskCode, "TelegramBotTask", 10000, NULL, 1, &TelegramBotTask, 1);
}

void loop() {
  if (!client.connected()) {
    connectToMQTTServer();
  }
  client.loop();
}
