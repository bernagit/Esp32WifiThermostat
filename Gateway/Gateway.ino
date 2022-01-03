#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal.h>
#include <PubSubClient.h>
#include "Gateway.h"

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
  float setTemp;
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

unsigned long lastTimeBotRan;

//robe di MQTT
WiFiClient espClient;
PubSubClient client(espClient);

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
    delay(DELAY_WIFI_TRY);
    Serial.print(".");  
  }

  Serial.println("");
  Serial.println("Wifi connected");
  Serial.println("Indirizzo IP: ");
  Serial.println(WiFi.localIP());
}

int timeUpDown = 0;
bool upPressed = false;
bool downPressed = false;
void IRAM_ATTR butUpInterrupt(){
  upPressed = true;
}
void IRAM_ATTR butDownInterrupt(){
  downPressed = true;
}

bool buttonPressed = false;
void IRAM_ATTR buttonPressedInterrupt() {
  if(timeUpDown == 0) {
    buttonPressed = true; 
  }
}

void setTemperature(String sensorName, float temperature) {
  String topic = "aaabbbccc/" + sensorName + "/setT";
  client.publish(topic.c_str(), String(temperature).c_str());
}

int clearSensorList() {
  Sensor newArray[MAX_SENSOR_NUM];
  int newSize = 0;
  int now = millis();
  
  for(int i = 0; i < arraySize; i++) {
    if(now - sensors[i].lastConnection < DELETE_SENSOR_TIME * 1000) {
      newArray[newSize] = sensors[i];
      newSize++;

      //Ai sensori collegati invia la temperatura settata
      String topic = "aaabbbccc/" + sensors[i].name + "/setT";
      client.publish(topic.c_str(), String(sensors[i].setTemp).c_str());
    }
  }

  if(newSize == 0) {
    selectedIndex = -1;
  } else {
    selectedIndex = 0;
  }

  int difference = arraySize - newSize;
  arraySize = newSize;

  for(int i = 0; i < newSize; i++) {
    sensors[i] = newArray[i];
  }

  return difference;
}

int startMillisDrawTask = 0;
int startMillisClearSensorsList = millis();
/**
 * Funzione che gestisce la scrittura sul display LCD
 */
void DrawTaskCode(void* param) {
  for(;;) {
    delay(DELAY_WATCHDOG_RESET);
    int currentMillis = millis();
  
    if(currentMillis - startMillisDrawTask >= REFRESH_RATE){
      if(buttonPressed && selectedIndex != -1) {
        buttonPressed = false;
        selectedIndex = (selectedIndex + 1) % arraySize;
      }
    
      startMillisDrawTask = currentMillis;
      draw();
    }

    if(currentMillis - startMillisClearSensorsList >= CLEAR_SENSOR_LIST_RATE * 1000) {
      client.unsubscribe("aaabbbccc/#");
      
      int deleted = clearSensorList();
      if(deleted > 0) {
        lcd.clear();
        lcd.home();
        lcd.print("Deleted ");
        lcd.print(deleted);
        lcd.setCursor(0, 1);
        lcd.print("sensors");
        digitalWrite(HUMIDITY_ALARM_PIN, LOW);
        digitalWrite(TEMPERATURE_ALARM_PIN, LOW);
        delay(DELAY_AFTER_REMOVED_SENSOR);
      }

      client.subscribe("aaabbbccc/#");
      startMillisClearSensorsList = currentMillis;
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

  Sensor s = {"", 0, "----", "--", "--", 15.0};
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

    setTemperature(espId, 15.0);
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
  Serial.println(selectedIndex);
  Sensor sensor = sensors[selectedIndex];
  if(sensor.temperature.toFloat() >= WARNING_TEMP) {
    digitalWrite(TEMPERATURE_ALARM_PIN, HIGH);
  } else {
    digitalWrite(TEMPERATURE_ALARM_PIN, LOW);
  }

  if(sensor.humidity.toFloat() >= WARNING_HUM) {
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
  lcd.print(" " + sensor.brightness + " lux ");

  /**
   * Se il sensore non si connette da più di 30 secondi, viene segnalato tramite
   * un'icona nell'angolo in alto a destra del display
   * Nel ramo else viene scritto uno spazio bianco per "pulire" nel caso l'icona
   * vada eliminata
   */
  lcd.setCursor(15, 0);
  if(millis() - sensor.lastConnection > DELAY_SENSOR_WARNING * 1000) {
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

      delay(DELAY_AFTER_SETUP);
    }
  }
}

/**
 * Funzione che gestisce il bot telegram
 */
void TelegramBotTaskCode(void* param) {
  for(;;) {
    delay(DELAY_WATCHDOG_RESET);
    if (millis() > lastTimeBotRan + BOT_DELAY_REFRESH)  {
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
      sensorInfo += "Brightness: " + sensor.brightness + "lux\n";
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

  //setup pin regolazione temperatura
  pinMode(PINUP, INPUT_PULLUP);
  pinMode(PINDOWN, INPUT_PULLUP);
  attachInterrupt(PINUP, butUpInterrupt, FALLING);
  attachInterrupt(PINDOWN, butDownInterrupt, FALLING);
  
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
  delay(DELAY_AFTER_SETUP);

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
  
  if(timeUpDown != 0 && millis() - timeUpDown >= TIMEVISIBLE){
    setTemperature(sensors[selectedIndex].name, sensors[selectedIndex].setTemp);
    timeUpDown = 0;
    vTaskResume(DrawTask);
  }
  
  if(upPressed){
    if(selectedIndex == -1)
      return;
    timeUpDown = millis();
    if(eTaskGetState(DrawTask) != eSuspended){
      vTaskSuspend(DrawTask);
    }
    sensors[selectedIndex].setTemp += 0.5;
    lcd.clear();
    lcd.print(sensors[selectedIndex].setTemp);
    
    upPressed = false;
  }
  if(downPressed){
    if(selectedIndex == -1)
      return;
    timeUpDown = millis();
    if(eTaskGetState(DrawTask) != eSuspended){
      vTaskSuspend(DrawTask);
    }
    sensors[selectedIndex].setTemp -= 0.5;
    lcd.clear();
    lcd.print(sensors[selectedIndex].setTemp);
    
    downPressed = false;
  }
}
