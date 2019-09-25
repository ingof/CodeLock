
#include <EEPROM.h>
#include <Keypad.h>
#include "Config.h"
#include "ds18b20.h"

#ifdef OneWireEnabled
  #include <OneWire.h>
  #endif //OneWireEnabled

#ifdef EthernetShieldEnabled
  #include <Dhcp.h>
  #include <Dns.h>
  #include <Ethernet.h>
  #include <EthernetClient.h>
  #include <EthernetServer.h>
  #include <EthernetUdp.h>
#endif //EthernetShieldEnabled

//RTC
#ifdef RTCEnabled
  #include <Wire.h>
  #include "DS1307.h"
#endif //RTCEnabled

/* Arduino Uno Connections :
 *  0-1:    Serial
 *  2-8:    Keypad
 *  9:      key LED (to GND)
 *  10-13:  Ethernet Shield
 *  14:     Door opener
 *  15:     Light
 *  16:     ( Relais 3)
 *  17:     1Wire???
 *  18:     Sabotage contact (n.c.)
 *  19:     Door contact (n.c.)
 */

/* KeyPad Definitonen */
  const byte ROWS = 4; //four rows
  const byte COLS = 3; //four columns
  char keys[ROWS][COLS] = {
    {'1','2','3'},
    {'4','5','6'},
    {'7','8','9'},
    {'*','0','#'}
  };
 /* Keyboard */
  //byte rowPins[ROWS] = {8, 7, 6, 5}; //connect to the row pinouts of the keypad
  //byte colPins[COLS] = {4, 3, 2}; //connect to the column pinouts of the keypad
  byte rowPins[ROWS] = {KEY_ROWS}; //connect to the row pinouts of the keypad
  byte colPins[COLS] = {KEY_COLS}; //connect to the column pinouts of the keypad
   
  Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS); 


/* Hardwarebelegung */
  int KEY_LED = 9;
 
  int KEY_LED_DEFAULT = 0xFF;
  int KEY_LED_OPEN = 0xFF;
  int TUEROEFFNER = 14;
  int LICHT = 15;
  int RELAIS_3 = 16;
  int ONE_WIRE = 17;
  int TASTATURKONTAKT= 18;
  int TUERKONTAKT = 19;

  int PWDCount = 0;
  long PWD = 0;
  
  
  bool usedFlag = true;  //Default-Pin erst nach Freigabe durch "Master"-PIN ( ! Stromausfall nach Paketablage ! )
  bool alarmDoor = false;
  bool alarmKeypad = false;
  long alarmStartDoor = 0;
  long alarmStartKeypad = 0;
  long alarmCountDoor = 0;
  long alarmCountKeypad = 0;


#ifdef EthernetShieldEnabled
  //char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  //buffer to hold incoming packet,
  EthernetUDP Udp;
  unsigned int portLocal = PORT_UDP_LOCAL;
  unsigned int portServer = PORT_UDP_SERVER;
  byte mac[] = {
    MAC_LOCAL
  };
  IPAddress ip(IP_LOCAL);
  IPAddress ipRemote(IP_UDP_SERVER);  
#endif //EthernetShieldEnabled


/* Rechte-Byte Info*/
    //------------------------------------------------------------
    // Rechte-Byte:
    //============================================================
    // Bit7: 1= Rechte-Byte
    //       0= Zeichen
    // Bit4: 1= acces every time
    //       0= acces only if not USED / Default-Pin
    // Bit2: 1= Licht einschalten
    //       0= kein Licht
    // Bit1: 1= Tür öffnen, used-Flag setzen
    //       0= Tür nicht öffnen
    // Bit0: 1= USED-Flag löschen
    //       0= USED-Flag setzen
    //------------------------------------------------------------

#ifdef OneWireEnabled
  OneWire ds(ONE_WIRE);
  long startConvert=millis();//+750;
  byte convert = 0;
  byte data[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  byte addr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  boolean power;
  byte i;
  byte present;
  byte type_s;
  float celsius, fahrenheit;
#endif //OneWireEnabled

//RTC
#ifdef RTCEnabled
  DS1307 clock;//define a object of DS1307 class
#endif //RTCEnabled
void setup(){

 /*Passwörter und Rechte */ {
#ifdef OwnPasswordFile
  #include "Passwords.h"
#else
  #include "ExamplePasswords.h"
#endif //OwnPasswordFile
}  

/* Hardware Konfiguration */ {
  //Ausgabe
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(KEY_LED, OUTPUT);
  pinMode(TUEROEFFNER, OUTPUT);
  pinMode(LICHT, OUTPUT);
  pinMode(RELAIS_3, OUTPUT);
  //Alarm
  pinMode(TUERKONTAKT, INPUT_PULLUP);
  pinMode(TASTATURKONTAKT, INPUT_PULLUP);
  
  digitalWrite(TUEROEFFNER, HIGH);
  digitalWrite(LICHT, HIGH);
  digitalWrite(RELAIS_3, HIGH);
  //digitalWrite(TUERKONTAKT, HIGH);
  analogWrite(KEY_LED, KEY_LED_DEFAULT);

  }
  
/* Software Initialisierung */ {
  #ifdef EthernetShieldEnabled
    Ethernet.begin(mac, ip);
    Udp.begin(portLocal);
  #endif //EthernetShieldEnabled
  
  #ifdef SerialEnabled
    Serial.begin(9600);  
  #endif //SerialEnabled
  
  logMessage("Codelock Serial V1.0");

//RTC
#ifdef RTCEnabled
  printTime();
#endif //RTCEnabled

}

}
 
//int divider=1;



void loop(void) {
  
#ifdef OneWireEnabled
  check1Wire();
#endif //OneWireEnabled



//void loop(){
  char customKey = keypad.getKey();
  if (customKey){
    int pwdAccess = 0;
    switch (customKey) {
      case '#': KEY_ACK();
                logMessage("#");
                //Serial.print('\t');
                PWDCount = 0;
                pwdAccess = checkPWDs(PWD);
                grantAccess(pwdAccess);
                PWD = 0;
                break;
      case '*': KEY_ACK();
                logMessage("*");
                PWDCount = 0;
                PWD = 0;
                break;
      default:  KEY_ACK();
                logMessage((String)customKey);
                PWDCount += 1;
                PWD *= 10;
                PWD += (customKey-0x30);
    }
  }
    
    delay(25);
#ifdef DoorAlarmEnabled
    if (digitalRead(TUERKONTAKT)==1) {
      if(alarmDoor) { // Alarm stand bereits an
        //alarmCountDoor++;
        alarmCountDoor=(millis() - alarmStartDoor);
        if ((alarmCountDoor/25)%72000==0) { // alle 30Minuten (1000*60*30=1800000/25=72000)(25ms Toleranz)
          String message="Sagotage Alarm Tür aktiv seit über ";
          if ((alarmCountDoor>=1000)&&(alarmCountDoor<120000)) { // < 2Minuten
            message+= (alarmCountDoor/1000); 
            message+= ",";
            message+= ((alarmCountDoor%1000)/100); 
            message+= " Sekunden!";
          }
          if ((alarmCountDoor>=120000)&&(alarmCountDoor<7200000)) { // >= 2 Minuten
            message+= (alarmCountDoor/60000);
            message+= ",";
            message+= ((alarmCountDoor%60000)/6000); 
            message+= (" Minuten!");
          }          
          if (alarmCountDoor>=7200000) { // >= 2 Stunden
            message+= (alarmCountDoor/3600000);
            message+= ",";
            message+= ((alarmCountDoor%3600000)/360000); 
            message+= (" Stunden!");
          }
          logMessage(message);
        }
      } else { // Alarm aufgetreten
        alarmDoor=true;
        alarmStartDoor=millis();
        logMessage("!!! SABOTAGE ALARM TÜR !!!");
        usedFlag=true;
      }
    } else { // Tür geschlossen
      if ((alarmDoor)) { // Alarm stand aber an
        //alarmCountDoor++;
        alarmCountDoor=(millis() - alarmStartDoor);
        String message="Sabotage Alarm Tür beendet: (Dauer: ";
        if (alarmCountDoor<1000) { // < 2Minuten
          message+= (alarmCountDoor); 
          message+= " ms)";
        }
        if ((alarmCountDoor>=1000)&&(alarmCountDoor<120000)) { // < 2Minuten
          message+= (alarmCountDoor/1000); 
          message+= ",";
          message+= ((alarmCountDoor%1000)/100); 
          message+= " Sekunden)";
        }
        if ((alarmCountDoor>=120000)&&(alarmCountDoor<7200000)) { // >= 2 Minuten
          message+= (alarmCountDoor/60000);
          message+= ",";
          message+= ((alarmCountDoor%60000)/6000); 
          message+= " Minuten)";            
        }
        if (alarmCountDoor>=7200000) { // >= 2 Stunden
          message+= (alarmCountDoor/3600000);
          message+= (",");
          message+= ((alarmCountDoor%3600000)/3600000); 
          message+= " Stunden)";            
        }
        logMessage(message); 
        alarmCountDoor=0;
        alarmDoor=false;
        alarmStartDoor=0;
        usedFlag=true;
      }
    }
#endif //DoorAlarmEnabled

#ifdef KeyboardAlarmEnabled
    // TODO: Keyboard-Sabotage-Alarm auch bei geöffneter Tür
    if (digitalRead(TASTATURKONTAKT)==1) {
      if(alarmKeypad) { // Alarm stand bereits an
        //alarmCountKeypad++;
        alarmCountKeypad=(millis() - alarmStartKeypad);
        if ((alarmCountKeypad/25)%72000==0) { // alle 30Minuten (1000*60*30=1800000/25=72000)(25ms Toleranz)
          String message="Sagotage Alarm Tastatur aktiv seit über ";
          if ((alarmCountKeypad>=1000)&&(alarmCountKeypad<120000)) { // < 2Minuten
            message += (alarmCountKeypad/1000); 
            message += ",";
            message += ((alarmCountKeypad%1000)/100); 
            message += " Sekunden!";
          }
          if ((alarmCountKeypad>=120000)&&(alarmCountKeypad<7200000)) { // >= 2 Minuten
            message += (alarmCountKeypad/60000);
            message += ",";
            message += ((alarmCountKeypad%60000)/6000); 
            message += " Minuten!";
          }          
          if (alarmCountKeypad>=7200000) { // >= 2 Stunden
            message += (alarmCountKeypad/3600000)+(",");
            message += ((alarmCountKeypad%3600000)/360000); 
            message += " Stunden!";            
          }
          logMessage(message);
        }
      } else { // Alarm aufgetreten
        alarmKeypad=true;
        alarmStartKeypad=millis();
        logMessage("!!! SABOTAGE ALARM TASTATUR !!!");
        usedFlag=true;
      }
    } else { // Tür geschlossen
      if ((alarmKeypad)) { // Alarm stand aber an
        //alarmCountKeypad++;
        alarmCountKeypad=(millis() - alarmStartKeypad);
        String message = "Sabotage Alarm Tastatur beendet: (Dauer: ";
        if (alarmCountKeypad<1000) { // < 2Minuten
          message += (alarmCountKeypad); 
          message += " ms)";
        }
        if ((alarmCountKeypad>=1000)&&(alarmCountKeypad<120000)) { // < 2Minuten
          message += (alarmCountKeypad/1000); 
          message += ",";
          message += ((alarmCountKeypad%1000)/100); 
          message += " Sekunden)";
        }
        if ((alarmCountKeypad>=120000)&&(alarmCountKeypad<7200000)) { // >= 2 Minuten
          message += (alarmCountKeypad/60000);
          message += ",";
          message += ((alarmCountKeypad%60000)/6000); 
          message += " Minuten)";            
        }
        if (alarmCountKeypad>=7200000) { // >= 2 Stunden
          message += (alarmCountKeypad/3600000);
          message += (",");
          message += ((alarmCountKeypad%3600000)/3600000); 
          message += (" Stunden)");            
        }
        logMessage(message);
        alarmCountKeypad=0;
        alarmKeypad=false;
        alarmStartKeypad=0;
        usedFlag=true;
      }
    }
#endif //KeyboardAlarmEnabled

}

#ifdef OneWireEnabled
void check1Wire(void) {  
        present = 0;
      byte continueConvert = 0;
      if (power) {
        while (!ds.read_bit()){
          delay(25);
        }
        continueConvert=1;
      } else {
        if ((millis()-startConvert)>750) {
          continueConvert=1;
        }
      }
      if (continueConvert) {
           //divider=100;
            
         if (convert == 0) {
            if ( !ds.search(addr)) {
//                Serial.println();
                Serial.print("Keine weiteren Addressen.\n");
//                Serial.println();
                Serial.println();
                ds.reset_search();
                return;
            }
            
            ds.reset();
            ds.write(SkipROM);
            ds.write(ReadPowerSupply);
            power=ds.read_bit();


               
//            Serial.println();
            Serial.print("R=");
            for( i = 0; i < 8; i++) {
              //Serial.print(addr[i], HEX);
              printHex(addr[i]);
              Serial.print(" ");
            }
           
            if ( OneWire::crc8( addr, 7) != addr[7]) {
                Serial.print("CRC nicht gültig!\n");
                return;
            }
           
            if ( addr[0] == 0x10) {
                type_s = 1;
                Serial.print("(DS18S20)");
            }
            else if ( addr[0] == 0x28) {
                type_s = 0;
                Serial.print("(DS18B20)");
            }
            else if ( addr[0] == 0x22) {
                type_s = 0;
                Serial.print("(DS1822)");
            }
            else {
                Serial.print("Gerätefamilie unbekannt : 0x");
                printHex(addr[0]);
                Serial.println();
                return;
            }
            Serial.println();
            
                       
            ds.reset();
            ds.select(addr);
            ds.write(ConvertT,1);         // start Konvertierung, mit power-on am Ende
            
      
            startConvert = millis();
            convert = 1;
         } else {
           //if ((millis()-startConvert)>1000) {
          //    delay(100);
         //     Serial.println(millis()-startConvert);
        //    }
      //      startConvert=millis();
            
//            delay(750);     // 750ms sollten ausreichen /*#############################*/
      
            
            // man sollte ein ds.depower() hier machen, aber ein reset tut das auch
            present = ds.reset();
            ds.select(addr);    
            ds.write(ReadScratchPad);         // Wert lesen
           
            
            Serial.print("P=");
            //Serial.print(present,HEX);
            printHex(present);
            Serial.print(" ");
            for ( i = 0; i < 9; i++) {           // 9 bytes
              data[i] = ds.read();
              //Serial.print(data[i], HEX);
              printHex(data[i]);
              Serial.print(" ");
            }
            Serial.print(" CRC=");
            //Serial.print( OneWire::crc8( data, 8), HEX);
            printHex( OneWire::crc8( data, 8));
//            Serial.println();
            if (power){
               Serial.print("  External");
            } else {
              Serial.print("  Parasite");
            }
            
            // Convert the data to actual temperature
            // because the result is a 16 bit signed integer, it should
            // be stored to an "int16_t" type, which is always 16 bits
            // even when compiled on a 32 bit processor.
            int16_t raw = (data[1] << 8) | data[0];
            if (type_s) {
              raw = raw << 3; // 9 bit resolution default
              if (data[7] == 0x10) {
                // "count remain" gives full 12 bit resolution
                raw = (raw & 0xFFF0) + 12 - data[6];
              }
            } else {
              byte cfg = (data[4] & 0x60);
              // at lower res, the low bits are undefined, so let's zero them
              if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
              else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
              else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
              //// default is 12 bit resolution, 750 ms conversion time
            }
            
            #ifdef Celsius
            celsius = (float)raw / 16.0;
            Serial.print("  T=");
            Serial.print(celsius);
            Serial.print("°C");
            #endif //Celsius
            
            #ifdef Fahrenheit
            fahrenheit = (float)raw / 16.0 * 1.8 + 32.0;
            Serial.print("  T=");
            Serial.print(fahrenheit);
            Serial.print("°F");
            #endif //Fahrenheit
            Serial.println();
            convert = 0;
            
         }
      }
  }
#endif //OneWireEnabled

void printHex(int value) {
  if (value<0x10){
    Serial.print('0');
    }
  Serial.print(value, HEX);
}


int grantAccess(int access) {

  // TODO: Abbruch bei Tastendruck ????
  if (access == 0) {
    logMessage("PIN Falsch!\n");
    deactivateDoor();
    LED_NACK();
    return 0;
  } else {
    if (bitRead(access,1)==false) {
      logMessage("PIN hat keine Rechte für Paketfach!");
      deactivateDoor();
      LED_NACK();
      return 0;
    }
    if ((bitRead(access,4)==false)&&(usedFlag==true)) {
      logMessage("PIN Paketfach bereits belegt");
      deactivateDoor();
      LED_NACK();
      return 0;
    }
    
    logMessage("PIN OK");
    activateDoor();
    if (bitRead(access,0)) { // Master-PIN resettet "usedFlag"
      usedFlag=false;
    }

    bool tmpOpen=openDoor(10);
    doorOpened(tmpOpen);
    if (tmpOpen) {
      if (!bitRead(access,0)) { // Master-PIN resettet "usedFlag"
        usedFlag=true;
      }
      doorClosed(closeDoor(50,bitRead(access,2)));
    }
    
    deactivateDoor();
    return 1;
  }
}

String getMillis() {
  unsigned long tmp=millis();
  String tmpStr="";
  int tmpSec=tmp/1000;
  int tmpMil=tmp%1000;
  tmpStr += formatNumber((int)tmpSec,7,' ')+'.'+formatNumber(tmpMil,3,'0')+": ";
  return tmpStr;
}

void activateDoor() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(TUEROEFFNER, LOW);
}

void deactivateDoor() {
  //digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(TUEROEFFNER, HIGH);
}

void doorClosed(bool flag) {
  if (flag) {
    analogWrite(KEY_LED, KEY_LED_DEFAULT);
    logMessage("Tür geschlossen.");
  } else {
    logMessage("Tür NICHT geschlossen!");
  }
}

void doorOpened(bool flag) {
  if (flag) {
    digitalWrite(LED_BUILTIN, LOW);
    analogWrite(KEY_LED, KEY_LED_OPEN);
    logMessage("Tür geöffnet.");
  } else {
    logMessage("Tür NICHT geöffnet!");
  }
}

int openDoor(byte seconds) {
  for (int i = 0; i < seconds*10; i++) {
    digitalWrite(TUEROEFFNER, LOW);
    delay(100);
    if (digitalRead(TUERKONTAKT)==1) {
      digitalWrite(TUEROEFFNER, HIGH);
      return 1;
    }
  }
  digitalWrite(TUEROEFFNER, HIGH);
  return 0; //Timeout
}

int closeDoor(byte seconds, bool light) {
  for (int i = 0; i < seconds*10; i++) {
    if (digitalRead(TUERKONTAKT)==0) {
      
        digitalWrite(LICHT, HIGH);
      
      // Bei beliebigen Tastendruck (oder nur Stern-Taste) abbrechen.
      return 1;
    }
    if (light) { // 2=Darf Licht schalten
      digitalWrite(LICHT, LOW);
    }
    delay(100);
  }
  digitalWrite(LICHT, HIGH);
  return 0; //Timeout
}

/*
 * Gibt "Rechte" Zurück wenn Passwort vergleich OK
 * Wenn Passwort falsch dann wird 0 zurückgegeben
 */
int checkPWDs(long password) {

  int eeByte;
  int eeCount = 0;
  long eePassword = 0;

  while (EEPROM[eeCount]!= 0) {
    eePassword=0;
    while (EEPROM[eeCount]<0x80)  {
      eeByte= EEPROM[eeCount++] - 0x30;
      if (EEPROM[eeCount-1] == 0) {
        //Serial.println("END!");
        return 0;
      }
      eePassword *= 10;
      eePassword += eeByte;      
    } 
    eeCount++;
    if (password==eePassword) {
      return EEPROM[eeCount-1];
    }
  }
  return 0;
}

String formatNumber(unsigned long number, byte digit, char fill) {
  String tmpStr="";
  for (byte i=digit-1; i>0 ;i--) {
    long value=10;
    for (byte j=1; j<i; j++) {
      value*=10;
    }
    if (number<value) {
      tmpStr += fill;
    }
  }
  tmpStr += number;
 return tmpStr;
}

 void logMessage(String message) {
  message = getMillis() + message;

  #ifdef SerialEnabled
    Serial.println(message);
  #endif //SerialEnabled
  
  #ifdef EthernetShieldEnabled
    char messageBuffer[message.length()];
    message.toCharArray(messageBuffer,message.length()+1);
    Udp.beginPacket(ipRemote, portServer);
    Udp.write(messageBuffer,message.length());
    Udp.endPacket();
  #endif //EthernetShieldEnabled

 }

 
void KEY_ACK() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(150);
  digitalWrite(LED_BUILTIN, LOW);
}

void LED_ACK() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  digitalWrite(LED_BUILTIN, LOW);
}

void LED_NACK() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
}
//RTC tests
#ifdef RTCEnabled
  /*Function: Display time on the serial monitor*/
  void printTime()
  {
    clock.getTime();
    Serial.print(clock.hour, DEC);
    Serial.print(":");
    Serial.print(clock.minute, DEC);
    Serial.print(":");
    Serial.print(clock.second, DEC);
    Serial.print("  ");
    Serial.print(clock.month, DEC);
    Serial.print("/");
    Serial.print(clock.dayOfMonth, DEC);
    Serial.print("/");
    Serial.print(clock.year+2000, DEC);
    Serial.print(" ");
    Serial.print(clock.dayOfMonth);
    Serial.print("*");
    switch (clock.dayOfWeek)// Friendly printout the weekday
    {
      case MON:
        Serial.print("MON");
        break;
      case TUE:
        Serial.print("TUE");
        break;
      case WED:
        Serial.print("WED");
        break;
      case THU:
        Serial.print("THU");
        break;
      case FRI:
        Serial.print("FRI");
        break;
      case SAT:
        Serial.print("SAT");
        break;
      case SUN:
        Serial.print("SUN");
        break;
    }
    Serial.println(" ");
  }
#endif //RTCEnabled
