#include <WiFi.h>
#include <WiFiClient.h>

#include <PubSubClient.h>
#include <DHT.h>
#include <ESP32Servo.h>

#define DHTPIN 23
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Sitecom8110b2";
const char* password = "16266773";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883; 

int analogPin = 15;
int tonePin = 5;

int analogInput = 0;
float Vref = 3.3;
float coeff = -1.25;
float Resistenza;
float luceArg;
float lumen;


WiFiClient espClient;
PubSubClient client(espClient);

void connect_WiFi() {

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  
  
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");

  char message[length];

  for(int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }

  Serial.print(message);
}



void mqtt_connect() {

  while(!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientID = "esp1-";
    clientID += String(random(0xffff), HEX);

    if(client.connect(clientID.c_str())) {
      Serial.println("Connected to MQTT broker!");
    } else {

      Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
    }
  }
}


float volt_to_lumen(int v) {
  Resistenza = ((10000.0 * v)/(v - Vref));

  if(Resistenza <= 0.00) {
    Resistenza = 100000.0;
  }

  luceArg = log10((Resistenza/100000.0));
  luceArg = coeff * luceArg;
  lumen = pow(10, luceArg);

  return lumen;
}

void setup() {
  
  Serial.begin(115200);
  pinMode(analogPin, INPUT);
  pinMode(tonePin, OUTPUT);

  connect_WiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  dht.begin();
}

void loop() {
 

  if (!client.connected()) {
    mqtt_connect();
  }
  
  client.loop();
  delay(2000);
  
  
  int humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  analogInput = analogRead(analogPin);
  Serial.print("ADC= ");
  Serial.print(analogInput);
  

  Serial.print("      Brightness [Lux]= ");
  lumen = volt_to_lumen(analogInput);
  Serial.print(lumen);
  Serial.println();


  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }else{  
    delay(500);

    client.publish("aaabbbccc/Room1/temperature",String(temperature).c_str());
    client.publish("aaabbbccc/Room1/humidity",String(humidity).c_str());
    client.publish("aaabbbccc/Room1/brightness", String(lumen).c_str());  
  }
  
  
}
