#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_PWMServoDriver.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <time.h>

#define NB_TRYWIFI 15  // Nbr de tentatives de connexion au réseau WiFi | Number of try to connect to WiFi network

const char* ssid = "Livebox-0210_EXT";
const char* password = "YFptPeFiLvmRLcixGo";

int oldMode = 1;
bool force = false;
int offset = 1;

WiFiClient espClient;
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 600000);

ESP8266WebServer server(80);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

unsigned long previousMillis = 0;
unsigned long previousMillisRadioLed = 0;
unsigned long interval = 600000;
unsigned long intervalRadioLed = 3000;
unsigned long puissanceMax = 4096;
unsigned int currentMode = 1;

unsigned int power_start_star_led[16];
unsigned int order_star_led[16];

#define LED_RADIO_1 13
#define LED_RADIO_2 14
#define LED_RADIO_3 15

#define start_period1 20 * 60
#define start_period2 21 * 60
#define start_period3 7 * 60
#define end_period3 20 * 60

#define advice_period1 9 * 60

/**************** INITIALISATIONS *********************/

void initSerial() {

  Serial.begin(115200);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);

  Serial.println();
  Serial.println("Init serial ...");
}

void InitWifi() {

  Serial.println("Init Wifi ...");
  Serial.println("Connecting ...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int _try = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("..");
    delay(2000);
    _try++;
    if (_try >= NB_TRYWIFI) {
      Serial.print("Try ");
      Serial.print(_try);
      Serial.print(" failed");

      ESP.restart();
    }
  }
  Serial.println("Connected to the WiFi network");
}

void InitNtp() {

  Serial.println("Init NTP ...");

  timeClient.begin();

  delay(200);

  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  Serial.println("Time OK ...");

  currentMode = GetPeriod();

  Serial.print("current mode :");
  Serial.println(currentMode);
}

int ManageSummerTime() {

  // Gestion de démarrage des timezones
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm* ptm = gmtime((time_t*)&epochTime);

  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;

  // heure d'été
  if ((currentMonth == 3 && monthDay >= 28)
      || (currentMonth > 3 && currentMonth < 10)
      || (currentMonth == 10 && monthDay < 25)) {
    return 2;
  }

  // heure d'hiver
  if ((currentMonth == 10 && monthDay >= 25)
      || (currentMonth > 11 && currentMonth < 3)
      || (currentMonth == 3 && monthDay < 28)) {
    return 1;
  }

  return 1;
}

void InitLeds() {

  Serial.println("Init LED ...");

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(1600);

  Wire.setClock(400000);

  pinMode(LED_RADIO_1, OUTPUT);
  pinMode(LED_RADIO_2, OUTPUT);
  pinMode(LED_RADIO_3, OUTPUT);

  Serial.println("Init Stars LED ...");
}

/*************FIN INITIALISATIONS *********************/

/************ GESTION SERVEUR WEB *********************/

void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void handleRoot() {

  char buf[16];

  String message = "<html><head><title>Tableau de Mathieu</title></head><body><h5>Bienvenue sur le tableau de Mathieu</h5>";
  message += "Vous &ecirc;tes connect&eacute; sur : <b>";
  message += sprintf(buf, "IP:%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  message += "</b><br/><br/>Le tableau est actuellement en mode : <b>";

  if (force == false) {
    message += "Automatique";
  } else {
    switch (currentMode) {
      case 1:
        message += "Endormissement";
        break;
      case 2:
        message += "Veilleuse";
        break;
      case 4:
        message += "Signal de lever";
        break;
      default:
        message += "Eteint";
    }
  }

  message += "</b><br/><br/>Il est actuellement : <b>";
  message += timeClient.getFormattedTime();
  message += "<br/><br/><table><tr><td><a href='/mode1'>Forcer en mode endormissement</a></td><td>|</td><td>";
  message += "<a href='/mode2'>Forcer en mode veilleuse</a></td><td>|</td><td>";
  message += "<a href='/mode3'>Forcer en mode &eacute;teint</a></td><td>|</td><td>";
  message += "<a href='/mode4'>Forcer en mode signal de lever</a></td><td>|</td><td>";
  message += "<a href='/auto'>Mettre en mode automatique</a></td></tr></table>";
  message += "</b></body></html>";

  server.send(200, "text/html", message);
}

void handleMode1() {
  currentMode = 1;
  force = true;

  for (int index = 0; index < 16; index++) {
    power_start_star_led[index] = random(2000, puissanceMax / 2);
    order_star_led[index] = 1;
  }

  Serial.println("Set automatic mode 1 ...");
  handleRoot();
}

void handleMode2() {
  currentMode = 2;
  force = true;

  for (int indexLed = 0; indexLed < 16; indexLed++) {
    pwm.setPWM(indexLed, 0, 0);
    yield();
  }

  Serial.println("Set automatic mode 2...");
  handleRoot();
}

void handleMode3() {
  currentMode = 3;
  force = true;

  for (int indexLed = 0; indexLed < 16; indexLed++) {
    pwm.setPWM(indexLed, 0, 0);
    yield();
  }

  Serial.println("Set automatic mode 3...");
  handleRoot();
}

void handleMode4() {
  currentMode = 1;
  force = false;
  Serial.println("Switch automatic mode ...");
  handleRoot();
}

void handleMode5() {
  currentMode = 4;
  force = true;

  for (int indexLed = 0; indexLed < 16; indexLed++) {
    pwm.setPWM(indexLed, 0, 0);
    yield();
  }

  Serial.println("Set automatic mode 4...");
  handleRoot();
}

/********* FIN GESTION SERVEUR WEB ********************/

/************** DETECTION PERIODE *********************/

int GetPeriod() {

  offset = ManageSummerTime();

  int nowMinutes = (timeClient.getHours() + offset) * 60 + timeClient.getMinutes();

  /* Période 1 : avant l'endormissement, les étoiles sont actives */
  if (nowMinutes >= start_period1 && nowMinutes <= start_period2) {

    for (int index = 0; index < 16; index++) {
      power_start_star_led[index] = random(2000, puissanceMax / 2);
      order_star_led[index] = 1;
    }

    oldMode = 1;

    if (oldMode != currentMode) {
      Serial.print("current mode : ");
      Serial.println("Endormissement");
    }

    return 1;
  }

  /* Période 2 : pendant le sommeil, mode veilleuse */
  if ((nowMinutes > start_period2 && nowMinutes <= (23 * 60 + 59))) {  // || (nowMinutes >= 0 && nowMinutes < start_period3)) {

    for (int indexLed = 0; indexLed < 16; indexLed++) {
      pwm.setPWM(indexLed, 0, 0);
      yield();
    }

    oldMode = 2;

    if (oldMode != currentMode) {
      Serial.print("current mode : ");
      Serial.println("Veilleuse");
    }

    return 2;
  }

  /* Période 3 : ne se passe rien */
  if (nowMinutes > end_period3 && nowMinutes <= start_period1) {

    for (int indexLed = 0; indexLed < 16; indexLed++) {
      pwm.setPWM(indexLed, 0, 0);
      yield();
    }

    oldMode = 3;

    if (oldMode != currentMode) {
      Serial.print("current mode : ");
      Serial.println("Off");
    }

    digitalWrite(LED_RADIO_1, LOW);
    digitalWrite(LED_RADIO_2, LOW);
    digitalWrite(LED_RADIO_3, LOW);

    return 3;
  }

  /* Période 4 : Signal que Mathieu peut se lever */
  if (nowMinutes > advice_period1 && nowMinutes <= (advice_period1 + 3)) {

    for (int indexLed = 0; indexLed < 16; indexLed++) {
      pwm.setPWM(indexLed, 0, 0);
      yield();
    }

    oldMode = 4;

    if (oldMode != currentMode) {
      Serial.print("current mode : ");
      Serial.println("Off");
    }

    digitalWrite(LED_RADIO_1, LOW);
    digitalWrite(LED_RADIO_2, LOW);
    digitalWrite(LED_RADIO_3, LOW);

    return 4;
  }

  return 3;
}

void DisplayMode2() {

  unsigned long currentMillis = millis();

  if (currentMillis > previousMillisRadioLed) {
    /* Allumage lent des leds */
    for (int fadeValue = 0; fadeValue <= 100; fadeValue++) {
      analogWrite(LED_RADIO_1, fadeValue);
      analogWrite(LED_RADIO_2, fadeValue);
      analogWrite(LED_RADIO_3, fadeValue);

      yield();
      server.handleClient();

      delay(50);
    }
  }

  if (currentMillis > previousMillisRadioLed + 100) {
    previousMillisRadioLed = currentMillis;
    /* Extinction lente des leds */
    for (int fadeValue = 100; fadeValue >= 0; fadeValue--) {
      analogWrite(LED_RADIO_1, fadeValue);
      analogWrite(LED_RADIO_2, fadeValue);
      analogWrite(LED_RADIO_3, fadeValue);

      yield();
      server.handleClient();

      delay(50);
    }
  }
}

void DisplayMode1() {

  for (int indexLed = 0; indexLed < 16; indexLed++) {

    int eclat = random(0, 3);

    if (eclat == 1) {
      pwm.setPWM(indexLed, puissanceMax, 0);
    } else {
      pwm.setPWM(indexLed, power_start_star_led[indexLed], 0);
    }

    delay(25);
    server.handleClient();

    yield();

    unsigned long currentMillis = millis();

    if (currentMillis > previousMillisRadioLed + 3000) {
      digitalWrite(LED_RADIO_1, HIGH);
      digitalWrite(LED_RADIO_2, HIGH);
      digitalWrite(LED_RADIO_3, HIGH);
    }

    if (currentMillis > previousMillisRadioLed + 3100) {
      digitalWrite(LED_RADIO_1, LOW);
      digitalWrite(LED_RADIO_2, LOW);
      digitalWrite(LED_RADIO_3, LOW);
    }

    if (currentMillis > previousMillisRadioLed + 3150) {
      digitalWrite(LED_RADIO_1, HIGH);
      digitalWrite(LED_RADIO_2, HIGH);
      digitalWrite(LED_RADIO_3, HIGH);
    }

    if (currentMillis > previousMillisRadioLed + 3250) {
      digitalWrite(LED_RADIO_1, LOW);
      digitalWrite(LED_RADIO_2, LOW);
      digitalWrite(LED_RADIO_3, LOW);
      previousMillisRadioLed = currentMillis;
    }
  }
}

void DisplayMode4() {

  for (int indexLed = 0; indexLed < 16; indexLed++) {

    server.handleClient();
    yield();

    unsigned long currentMillis = millis();

    if (currentMillis > previousMillisRadioLed + 1000) {
      digitalWrite(LED_RADIO_1, HIGH);
      digitalWrite(LED_RADIO_2, HIGH);
      digitalWrite(LED_RADIO_3, HIGH);

      pwm.setPWM(indexLed, puissanceMax, 0);
    }

    if (currentMillis > previousMillisRadioLed + 1100) {
      digitalWrite(LED_RADIO_1, LOW);
      digitalWrite(LED_RADIO_2, LOW);
      digitalWrite(LED_RADIO_3, LOW);

      pwm.setPWM(indexLed, 0, 0);
    }

    if (currentMillis > previousMillisRadioLed + 1150) {
      digitalWrite(LED_RADIO_1, HIGH);
      digitalWrite(LED_RADIO_2, HIGH);
      digitalWrite(LED_RADIO_3, HIGH);

      pwm.setPWM(indexLed, puissanceMax, 0);
    }

    if (currentMillis > previousMillisRadioLed + 1250) {
      digitalWrite(LED_RADIO_1, LOW);
      digitalWrite(LED_RADIO_2, LOW);
      digitalWrite(LED_RADIO_3, LOW);

      pwm.setPWM(indexLed, 0, 0);

      previousMillisRadioLed = currentMillis;
    }
  }
}

/********** FIN DETECTION PERIODE *********************/

void setup() {
  initSerial();
  InitWifi();
  InitNtp();
  InitLeds();

  Serial.println("Init web server ...");
  server.on("/", handleRoot);
  server.on("/mode1", handleMode1);
  server.on("/mode2", handleMode2);
  server.on("/mode3", handleMode3);
  server.on("/mode4", handleMode5);
  server.on("/auto", handleMode4);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started...");
}

void loop() {

  server.handleClient();

  if (force == false) {
    currentMode = GetPeriod();
  }

  switch (currentMode) {
    case 1:
      DisplayMode1();
      break;
    case 2:
      DisplayMode2();
      break;
    case 4:
      DisplayMode4();
      break;
    default:
      Serial.println("Do nothing...");
      delay(1000);
  }
}
