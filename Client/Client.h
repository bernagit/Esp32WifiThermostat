//SSID e PASS del wifi
#define WIFI_SSID "Telecom-33486489"
#define WIFI_PASSWORD "bernocchi-2016"

//indirizzo e porta del server MQTT
#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883

//Pin del sensore di temperatura
#define DHTPIN 23
//tipo di sensore di temperatura
#define DHTTYPE DHT11
//pin de sensore fotoelettrico GL55
#define FOTO_PIN 32
//pin del led che indica il riscaldamento acceso
#define LED_PIN 5
//Ohm resistenza usata per il partitore di tensione
#define R 10000
//tensione di alimentazione del partitore
#define VREF 3300
//tempo di attesa tra i tentativi di connessione al WiFi
#define DELAY_WIFI_TRY 100
//Tempo di attesa dopo essersi collegati al server MQTT 
#define DELAY_AFTER_SETUP 1000
//Intervallo di invio dei dati al server MQTT
#define SEND_TIME 7000
