//SSID e PASS del wifi
#define WIFI_SSID "alessandro"
#define WIFI_PASSWORD ""

//indirizzo e porta del server MQTT
#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883

//robe di telegram
#define BOTtoken "5039029687:AAE1sgr9d_WOaiKACnrFCg6RyBhlpFtysj0"  // your Bot Token (Get from Botfather)
#define CHATS_ID "485901444|364021314|848220826"

//Numero massimo di sensori collegabili
#define MAX_SENSOR_NUM 10

//Pin sul quale è "collegato" il bottone
#define BUTTON_PIN 22
//Frequenza di aggiornamento dello schermo LCD (in ms)
#define REFRESH_RATE 200
//Tempo di attesa prima della "pulizia" della lista dei sensori per rimuovere quelli non connessi (in Secondi)
#define CLEAR_SENSOR_LIST_RATE 30
//Tempo massimo di attesa prima dell'eliminazione di un sensore (in Secondi)
#define DELETE_SENSOR_TIME 60
//Tempo di attesa prima di mostrare il WARNING dovuto alla disconnesione di un sensore (in Secondi)
#define DELAY_SENSOR_WARNING 15
//Pin che accende il led rosso per segnalare l'allarme relativo alla temperatura
#define TEMPERATURE_ALARM_PIN 16
//Pin che accende il led verde per segnalare l'allarme relativo all'umidità
#define HUMIDITY_ALARM_PIN 17

//Tempo di aggiornamento delle richieste da telegram
#define BOT_DELAY_REFRESH 1000
//Tempo di attesa dopo essersi collegati al WiFi o al server MQTT 
#define DELAY_AFTER_SETUP 1000
//tempo di attesa tra i tentativi di connessione al WiFi
#define DELAY_WIFI_TRY 100
//tempo minimo di attesa per attivare il delay e resettare il timer di watchdog del microprocessore
#define DELAY_WATCHDOG_RESET 1
//tempo di visualizzazione della scritta "Deleted N sensors" dopo aver eliminato dei sensori che non pubblicano più dati
#define DELAY_AFTER_REMOVED_SENSOR 4000

//temperatura e umidità limite per la segnalazione tramite led
#define WARNING_TEMP 22
#define WARNING_HUM 70


//pin dei tasti per il controllo tepmeratura
#define PINUP 21
#define PINDOWN 23
//durata visualizzazione della temperatura settata sul display (in ms)
#define TIMEVISIBLE 3000

//Simboli del display

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
byte fuego[8] = {
  B11111,
  B11111,
  B11000,
  B11110,
  B11110,
  B11000,
  B11000,
  B11000,
};
