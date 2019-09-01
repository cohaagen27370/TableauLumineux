#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>
#include <DS3231.h>
#include <avr/sleep.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

unsigned long previousMillis = 0;
unsigned long interval = 3000;
unsigned long puissanceMax = 4096;

DS3231 clock;
RTCDateTime dt;

int counter;

#define serialspeed 9600

// production hours
int houralarm = 20;
int minutealarm = 0;
int sleephour = 7;
int sleepminute = 0;
int hourphase2 = 20;
int minutephase2 = 45;

// test hours
/*int houralarm = 12;
  int minutealarm = 45;
  int sleephour = 12;
  int sleepminute = 50;
  int hourphase2 = 12;
  int minutephase2 = 47;*/

void Sleep() {

  Serial.println("Sleep in progress");

  for (int indexLed = 0; indexLed < 16; indexLed ++)
  {
    pwm.setPWM(indexLed, 0, 0);
  }

  // disable ADC
  ADCSRA = 0;

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // Do not interrupt before we go to sleep, or the
  // ISR will detach interrupts and we won't wake.
  noInterrupts();

  // will be called when pin D2 goes low
  attachInterrupt (0, wake, FALLING);
  EIFR = bit (INTF0);  // clear flag for interrupt 0

  // turn off brown-out enable in software
  // BODS must be set to one and BODSE must be set to zero within four clock cycles
  MCUCR = bit (BODS) | bit (BODSE);
  // The BODS bit is automatically cleared after three clock cycles
  MCUCR = bit (BODS);

  // We are guaranteed that the sleep_cpu call will be done
  // as the processor executes the next instruction after
  // interrupts are turned on.
  interrupts();  // one cycle
  sleep_cpu();   // one cycle
}

void SetAlarm() {

  clock.armAlarm1(false);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();

  Serial.println("Alarm initialized");
  clock.setAlarm1(0, houralarm, minutealarm, 0, DS3231_MATCH_H_M_S, true);
  clock.enableOutput(false);

  RTCAlarmTime a1;

  if (clock.isArmed1())
  {
    a1 = clock.getAlarm1();

    Serial.print("Alarm1 is triggered ");
    switch (clock.getAlarmType1())
    {
      case DS3231_EVERY_SECOND:
        Serial.println("every second");
        break;
      case DS3231_MATCH_S:
        Serial.print("when seconds match: ");
        Serial.println(clock.dateFormat("__ __:__:s", a1));
        break;
      case DS3231_MATCH_M_S:
        Serial.print("when minutes and sencods match: ");
        Serial.println(clock.dateFormat("__ __:i:s", a1));
        break;
      case DS3231_MATCH_H_M_S:
        Serial.print("when hours, minutes and seconds match: ");
        Serial.println(clock.dateFormat("__ H:i:s", a1));
        break;
      case DS3231_MATCH_DT_H_M_S:
        Serial.print("when date, hours, minutes and seconds match: ");
        Serial.println(clock.dateFormat("d H:i:s", a1));
        break;
      case DS3231_MATCH_DY_H_M_S:
        Serial.print("when day of week, hours, minutes and seconds match: ");
        Serial.println(clock.dateFormat("l H:i:s", a1));
        break;
      default:
        Serial.println("UNKNOWN RULE");
        break;
    }
  } else
  {
    Serial.println("Alarm1 is disarmed.");
  }
}

void wake() {
  // cancel sleep as a precaution
  sleep_disable();
  // precautionary while we do other stuff
  detachInterrupt(0);

  Serial.println("Waking up");
  //pwm.wakeUp(100);

}  // end of wake

void setup() {

  Serial.begin(serialspeed);
  Serial.println("Start serial");

  pwm.begin();
  pwm.setPWMFreq(100);
  Serial.println("PWM initialized");

  // if you want to really speed stuff up, you can go into 'fast 400khz I2C' mode
  // some i2c devices dont like this so much so if you're sharing the bus, watch
  // out for this!
  Wire.setClock(5);

  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  clock.begin();
  Serial.println("Clock initialized");

  // Set sketch compiling time
  //clock.setDateTime(__DATE__, __TIME__);

  dt = clock.getDateTime();

  Serial.print("Date and time are : ");
  Serial.print(dt.year);   Serial.print("-");
  Serial.print(dt.month);  Serial.print("-");
  Serial.print(dt.day);    Serial.print(" ");
  Serial.print(dt.hour);   Serial.print(":");
  Serial.print(dt.minute); Serial.print(":");
  Serial.print(dt.second); Serial.println("");

  SetAlarm();

}

void loop() {

  unsigned long currentMillis = millis();

  // Phase d'endormissement
  if (IsInPhase1())
  {
    for (int indexLed = 0; indexLed < 16; indexLed ++)
    {
      pwm.setPWM(indexLed, random(0, puissanceMax + 1), 0);
    }

    delay(random(100, 130 + 1));

    if  (currentMillis > previousMillis + interval) {
      previousMillis = currentMillis;

      digitalWrite(4, HIGH);
      digitalWrite(5, HIGH);
      digitalWrite(6, HIGH);
      delay(100);
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);
      delay(50);
      digitalWrite(4, HIGH);
      digitalWrite(5, HIGH);
      digitalWrite(6, HIGH);
      delay(100);
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);

      Serial.print(dt.hour);   Serial.print(":");
      Serial.print(dt.minute); Serial.print(":");
      Serial.print(dt.second); Serial.println("");

    }
  }

  // Phase de nuit
  if (IsInPhase2())
  {
    for (int fadeValue = 0 ; fadeValue <= 3; fadeValue += 1) {
      analogWrite(5, fadeValue);
      analogWrite(6, fadeValue);

      delay(500);
    }

    for (int fadeValue = 3 ; fadeValue >= 0; fadeValue -= 1) {
      analogWrite(5, fadeValue);
      analogWrite(6, fadeValue);

      delay(500);
    }

    for (int indexLed = 0; indexLed < 16; indexLed ++)
    {
      pwm.setPWM(indexLed, 0, 0);
    }
  }

  // Phase de jour (veille)
  if (IsInStandBy())
  {
    Sleep();
  }


  //dt = clock.getDateTime();

  /*Serial.print("Heure actuelle : ");
    Serial.print(dt.hour);
    Serial.print(":");
    Serial.println(dt.minute);

    Serial.print("Veille : ");

    Serial.print(sleephour);
    Serial.print(":");
    Serial.println(sleepminute);*/

  /*if ((dt.hour < houralarm || (dt.hour == houralarm && dt.minute < minutealarm)) || (dt.hour > sleephour || (dt.hour == sleephour && dt.minute > sleepminute)))
    {
    Sleep();
    }*/
}

// Endormissement
bool IsInPhase1() {
  dt = clock.getDateTime();

  if (dt.hour > 12)
  {
    bool isInPhase1 = ((dt.hour > houralarm || (dt.hour == houralarm && dt.minute >= minutealarm)) && (dt.hour < hourphase2 || (dt.hour == hourphase2 && dt.minute < minutephase2)));
    
    return isInPhase1;
  }
  else
  {
    return false;
  }
}

// Phase de nuit
bool IsInPhase2() {
  dt = clock.getDateTime();

  bool isInPhase2 = false;

  if (dt.hour > 12)
  {
    isInPhase2 = (dt.hour > hourphase2 || (dt.hour == hourphase2 && dt.minute >= minutephase2));
  }
  else if (dt.hour < 12) 
  {
     isInPhase2 = (dt.hour < sleephour || (dt.hour == sleephour && dt.minute <= sleepminute));
  }

  return isInPhase2;
}

// Phase de jour (veille)
bool IsInStandBy() {

  dt = clock.getDateTime();
  
  bool isInSleep = !IsInPhase1() && !IsInPhase2() && ((dt.hour > sleephour || (dt.hour == sleephour && dt.minute >= sleepminute )) && (dt.hour < houralarm || (dt.hour == houralarm && dt.minute <= minutealarm )));

  return isInSleep;
}
