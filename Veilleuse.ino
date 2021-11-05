#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <time.h>


const int LEDROUGE_1 = 14;
const int LEDROUGE_2 = 0;

const int LEDBLEUE_1 = 4;
const int LEDBLEUE_2 = 12;

const int LEDJAUNE_1 = 5;
const int LEDJAUNE_2 = 13;

const int LEDBLANC_1 = 2;
const int LEDBLANC_2 = 16;

#define PHOTORES A0
#define NB_TRYWIFI        15    // Nbr de tentatives de connexion au réseau WiFi | Number of try to connect to WiFi network

const char* ssid = "Livebox-0210_EXT";
const char* password = "YFptPeFiLvmRLcixGo";

#define start_period 20*60 + 30 
#define end_period 7*60 + 30 

WiFiClient espClient;
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 600000);

int raw = 0;
int offset = 1;

float RLDR = 0;
float vout = 0;
float lux = 0;

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
    if ( _try >= NB_TRYWIFI ) {
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

  offset = ManageSummerTime();
  Serial.print("offset :");
  Serial.println(offset);
}

int ManageSummerTime() {

  // Gestion de démarrage des timezones
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;

  // heure d'été
  if ((currentMonth == 3 && monthDay >= 28)
      || (currentMonth > 3 && currentMonth < 10) 
      || (currentMonth == 10 && monthDay < 25))
  {
    return 2;
  }

  // heure d'hiver
  if ((currentMonth == 10 && monthDay >= 25) 
      || (currentMonth > 11 && currentMonth < 3)
      || (currentMonth == 3 && monthDay < 28))      
  {
    return 1;
  }

  return 1;
}

bool GetPeriod() {

  int nowMinutes = (timeClient.getHours() + offset) * 60 + timeClient.getMinutes();

  Serial.println(timeClient.getHours() + offset);
  Serial.println(timeClient.getMinutes());

  if (nowMinutes > start_period || nowMinutes < end_period)
  {
    Serial.println("must be on");
    return true;
  }

  Serial.println("must be off");
  return false;
}


void TurnOnAndOffLed(int numled) {

  digitalWrite(numled,LOW);

  for(int value = 0; value <= 255; value += 10)
  {
    analogWrite(numled,value);
    delay(50);
  }

  for(int value = 255; value >= 0; value -= 10)
  {
    analogWrite(numled,value);
    delay(50);
  }
}

void TurnOnAndOff2Led(int numled1,int numled2) {

  digitalWrite(numled1,LOW);
  digitalWrite(numled2,LOW);

  for(int value = 0; value <= 255; value += 5)
  {
    analogWrite(numled1,value);
    analogWrite(numled2,value);
    delay(20);
  }

  for(int value = 255; value >= 0; value -= 5)
  {
    analogWrite(numled1,value);
    analogWrite(numled2,value);
    delay(20);
  }
}

void TurnOnAndOffNLed(int numLed[], int count) {

  for(int leds = 0; leds < count; leds ++)
  {
    digitalWrite(numLed[leds],LOW);
  }  

  for(int value = 0; value <= 255; value += 5)
  {
    for(int leds = 0; leds < count; leds ++)
    {
      analogWrite(numLed[leds],value);
    }
    delay(20);
  }

  for(int value = 255; value >= 0; value -= 5)
  {
    for(int leds = 0; leds < count; leds ++)
    {    
      analogWrite(numLed[leds],value);
    }
    delay(20);
  }
  
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  pinMode(LEDBLANC_1, OUTPUT);
  pinMode(LEDBLANC_2, OUTPUT);
  pinMode(LEDROUGE_1, OUTPUT);
  pinMode(LEDROUGE_2, OUTPUT);
  pinMode(LEDBLEUE_1, OUTPUT);
  pinMode(LEDBLEUE_2, OUTPUT);
  pinMode(LEDJAUNE_1, OUTPUT);
  pinMode(LEDJAUNE_2, OUTPUT);

  pinMode(PHOTORES, INPUT);

  InitWifi();

  InitNtp();
}



void loop() {

bool isOn = GetPeriod();
int leds[] = {0,2,4,5,12,13,14,16};

  if (isOn)
  {

    TurnOnAndOff2Led(LEDBLANC_1,LEDBLANC_2);
    TurnOnAndOff2Led(LEDROUGE_1,LEDROUGE_2);
    TurnOnAndOff2Led(LEDBLEUE_1,LEDBLEUE_2);
    TurnOnAndOff2Led(LEDJAUNE_1,LEDJAUNE_2);
      
    delay(500);  

    TurnOnAndOffNLed(leds,8);
        
    delay(500);  
  }
  else
  {
    delay(60000);
  }
  

}
