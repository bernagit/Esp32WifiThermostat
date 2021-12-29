#include <WiFi.h>
#include <WiFiClient.h>

#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 23
#define DHTTYPE DHT11

int analogPin = 15;
int tonePin = 5;

int analogInput = 0;
float r0 = 10000.0;
float Vref = 3.3;
float coeff = -1.25;
float Resistenza;
float argLuce;
float lumen;

const char* ssid;
const char* password;
const char* mqtt_server;
const int mqtt_port = 1183;

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


void callback(char* topic, byte* payload, int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  char message[length];
  for(int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }

  Serial.print(message);
}


void mqtt_connect() {
  while(!client.connected()) {
    Serial.print("Attempting MQTT connection ...");

    String clientID = "esp1-";
    clientID += String(random(0xffff), HEX);

    if(client.connect(clientID.c_str())) {
      Serial.print(" Connected");
    } else {

      Serial.print("failed, rc=");
      Serial.print(client.state());
      
      Serial.print(" try again in 5 seconds...");
      delay(5000);
    }
  }
}


float volt_to_lumen(int v) {
  Resistenza = (v * r0)/(v - Vref);

  if(Resistenza <= 0.00) {
    Resistenza = 100000.0;
  }

  argLuce = log10((Resistenza/100000.0));
  argLuce = coeff * argLuce;
  lumen = pow(10, argLuce);

  return lumen;
}



void setup() {
  Serial.begin(115200);
  pinMode(analogPin, INPUT);
  pinMode(tonePin, OUTPUT);

  analogInput = analogRead(analogPin);
  Serial.print("ADC BEGIN= ");
  Serial.print(analogInput);
  Serial.println();

  dht.begin();
  connect_WiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}


void loop() {

  if(!client.connected()) {
    mqtt_connect();
  }

  client.loop();
  delay(2000);

  int humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  analogInput = analogRead(analogPin);

  lumen = volt_to_lumen(analogInput);

  if( isnan(humidity) || isnan(temperature) ) {
    Serial.print("Failed to read from DHT sensor");
    return;
  } else {

    client.publish("aaabbbccc/Room1/temperature", String(temperature).c_str());
    client.publish("aaabbbccc/Room1/humidity", String(humidity).c_str());
    client.publish("aaabbbccc/Room1/brightness", String(lumen).c_str());
  }

}
