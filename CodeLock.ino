
/*
   DONE:
   ----------------------
   # average value of temperatures
   # fixed: hang on writing to Display (support RTC again) removed R2 and R3 of RTC I2C Pullup Resistor 3,3k
   # synchronize system-millis and rtc-seconds
   # support watchdogtimer
   # change UDP Retry an Timeout
   # add "thank-you"-message on Display if door closed
   
   ToDo:
   ----------------------
   # if three udp-packets not send any pakets anymore
   # check UDP-connection in longer period. Enable packet sendig if succesfull
   # break door Open or light if cleare-key pressed???
   # clean code
   # maximim of pin numbers (6?)
   # fix pin clear at sleep
   # fixed???: opens every time with pin4all
   # option for "Display Off" in sleep or "contrast=1" for better lifespan
   # display average Temperature in higher resolution
   # save ds18b20 ids to eeprom
   # find ids after booting and display missing and new sensors
   # suport time range for access (use Bit 4, or new Bit 3, for signaling) or timesets bit 5 and 6
   # additional functions for master pin??
   # return pin count or pin ID number for PIN Names??
   # support names for pins
   
   MayBe:
   ----------------------
   # open at "n" number autmatically
   # support SD-Card
   # store and verify time at boot time with latest NTP-Timestamp
     (update NTP-timestamp every hour if timedif <100)
   # support SD-logging and send sd-saved logs after log-server is available again
   # special welcome messages after wakeup or after opening door
   # send email if opened
   # display time in screenSaveMode
*/

#include <EEPROM.h>
#include <Keypad.h>
#include "Config.h"
#include "ds18b20.h"
#include <avr/wdt.h>


/* Keypad configuration */

  /* Keypad definition */
#include <Wire.h>
#ifdef KEY_PCF
  #include <Keypad_I2C.h>
#endif //KEY_PCF
#ifdef KEY_MCP
  #include <Keypad_MCP.h>
#endif //KEY_MCP

  /* Keypad Type */
#ifdef KEY16
  const byte ROWS = 4; //four rows
  const byte COLS = 4; //four columns
  char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
  };
#else
  const byte ROWS = 4; //four rows
  const byte COLS = 3; //four columns
  /* KeyPad Definitonen */
  char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
  };
#endif

  /* Keyboard  connections */
#ifdef KEY_PIN
    /* Port Pin connections */
  #ifdef KEY_FOIL
    // Port Folie
    #ifdef KEY16
//      byte rowPins[ROWS] = {A15, A14, A13, A12};
//      byte colPins[COLS] = {A11, A10, A9, A8};
      byte rowPins[ROWS] = {A8, A9, A10, A11}; //reversed
      byte colPins[COLS] = {A12, A13, A14, A15}; //reversed
    #else //KEY16
//      byte rowPins[ROWS] = {A14, A13, A12, A11};
//      byte colPins[COLS] = {A10, A9, A8};
      byte rowPins[ROWS] = {A8, A9, A20, A11}; //reversed
      byte colPins[COLS] = {A12, A13, A14}; //reversed
    #endif //KEY16
  #else  //KEY_FOIL
    // Port ALU
    byte rowPins[ROWS] = {A15, A14, A8, A9};
    byte colPins[COLS] = {A13, A12, A11};
  #endif  //KEY_FOIL
#else //KEY_PIN
    /* I2C Keypad connections: */// I2C Folie
  #ifdef KEY_FOIL
    #ifdef KEY16
      byte rowPins[ROWS] = {0, 1, 2, 3};
      byte colPins[COLS] = {4, 5, 6, 7};
    #else //KEY16
      byte rowPins[ROWS] = {1, 2, 3, 4};
      byte colPins[COLS] = {5, 6, 7};
    #endif //KEY16
  #else //KEY_FOIL
    // I2C ALU
    byte rowPins[ROWS] = {1, 2, 7, 6};
    byte colPins[COLS] = {3, 4, 5};
  #endif //KEY_FOIL
#endif //KEY_PIN

#ifdef OneWireEnabled
  /* Counter for number of Values */
  byte dsCount[]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  /* addes values of all read values */
  int32_t dsAverage[]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  #include <OneWire.h>
#endif //OneWireEnabled

#ifdef DisplayEnabled
  #include "U8g2lib.h"
  //U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
  //U8G2_SSD1309_128X64_NONAME2_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
  u8g2_uint_t offset;
  u8g2_uint_t width;
  const char *text = "CodeLock V2.0";
  bool screenSaveStatus = OFF;
  long screenSaveStart;
#endif //DisplayEnabled

#ifdef DisplayEnabled_1
  #include "lcdgfx.h"
  DisplaySSD1306_128x32_I2C display(-1);
#endif //DisplayEnabled_1

#ifdef DisplayEnabled_2
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 32 // OLED display height, in pixels
  #define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif //DisplayEnabled_2

#ifdef EthernetShieldEnabled
  #include <Dhcp.h>
  #include <Dns.h>
  #include <Ethernet.h>
  #include <EthernetClient.h>
  #include <EthernetServer.h>
  #include <EthernetUdp.h>
  #include <NTPClient.h>
#endif //EthernetShieldEnabled

//RTC
#ifdef RTCEnabled
  //  #include <Wire.h>
  #include <RTClib.h>
#endif //RTCEnabled

/* Arduino Uno Connections :
    0:      RX0
    1:      TX0
    2:      INT0: Door contact (n.c.)
   ~3:      INT1: PFC8574 INT Keyboard keys and alarm contact
    4:      SS, SD-Card
   ~5:      Keypad Light
   ~6:      status LED (to GND)
    7:      Rel3: ---
    8:      Rel2: Door Opener
   ~9:      Rel1: Light
   ~10:     SS: Ethernet Shield
   ~11:     MOSI: Ethernet Shield, SDCard
    12:     MISO: Ethernet Shield, SDCard
    13:     SCK: Ethernet Shield, SDCard
    14:     BUZZER oder ADC0: ---
    15:     ADC1: ---
    16:     ADC2: ---
    17:     1Wire (DS18B20)
    18:     SDA: 1²C (RTC, PFC8574-Keypad)
    19:     SCL: 1²C (RTC, PFC8574-Keypad)
*/

/*  Port Extender PFC8574/MCP23008
    0:    Alarm Loop (n.c.)
    1:    Row 1 (Y1)
    2:    Row 2 (Y2)
    3:    Column 1 (X1)
    4:    Column 2 (X2)
    5:    Column 3 (X3)
    6:    Row 4 (Y4)
    7:    Row 3 (Y3)
    8:    INT
*/


#ifdef KEY_PCF
  Keypad_I2C myKeypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR);
#endif //KEY_PCF

#ifdef KEY_MCP
  Keypad_MCP myKeypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR);
#endif //KEY_MCP

#ifdef KEY_PIN
  Keypad myKeypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);
#endif //KEY-PIN

/* Hardwarebelegung */
int KEY_LED_DEFAULT = 0xFF;
int KEY_LED_OPEN = 0xFF;

int TUERKONTAKT = 2;
int TASTATURKONTAKT = 3;
int SD_CS = 4;
int KEYPAD_LIGHT = 5;
int KEY_LED = 6;
int RELAIS_3 = 7;
int TUEROEFFNER = 8;
int LICHT = 9;
int BUZZER = 14;
int ONE_WIRE = 17;

int milliOffset = 0;
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
EthernetUDP Udp;
unsigned int portLocal = PORT_UDP_LOCAL;
unsigned int portServer = PORT_UDP_SERVER;
byte mac[] = { MAC_LOCAL };
IPAddress ip(IP_LOCAL);
IPAddress ipRemote(IP_UDP_SERVER);
NTPClient timeClient(Udp, "de.pool.ntp.org", NTP_OFFSET, NTP_REFRESH * 1000);
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
long startConvert = millis(); //+750;
//byte ntp = 0;
byte convert = 0;
byte data[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte addr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
boolean power;
byte i;
byte dsID = 0;
byte present;
byte type_s;
float celsius, fahrenheit;
#endif //OneWireEnabled

//RTC
#ifdef RTCEnabled
long updateTimeNTP = millis() + 10000 - NTP_REFRESH; //
RTC_DS1307 rtc; //for RTClib
#endif //RTCEnabled


void setup() {

analogWrite(KEYPAD_LIGHT, 255);
delay(100);
analogWrite(KEYPAD_LIGHT, 128);
delay(100);
analogWrite(KEYPAD_LIGHT, 64);
delay(100);
analogWrite(KEYPAD_LIGHT, 32);
delay(100);
analogWrite(KEYPAD_LIGHT, 0);
delay(100);
 
wdt_enable(WDTO_8S);   // Watchdog auf 1 s stellen

  //I2C keypad
#ifndef KEY_PIN
  myKeypad.begin();
#endif //KEY_PIN

#ifdef RTCEnabled
  rtc.begin();
#endif //RTCEnabled

#ifdef DisplayEnabled_1
  display.setFixedFont( ssd1306xled_font8x16 );
  display.begin();
  display.clear();
  //display.printFixed(0,  0, "Normal text", STYLE_NORMAL);
  display.printFixed(0, 0, " Codelock V2.0", STYLE_BOLD);
  //   display.printFixed(0, 11, "Italic text", STYLE_ITALIC);
  display.negativeMode();
  display.printFixed(0, 16, " * Clr ", STYLE_BOLD);
  display.printFixed(80, 16, " # Ok ", STYLE_BOLD);
  display.positiveMode();
  lcd_delay(3000);
#endif //DisplayEnabled_1


#ifdef DisplayEnabled
  long screenSaveStart;
  u8g2.setI2CAddress(0x78);
  u8g2.begin();
  u8g2.setPowerSave(0);
  u8g2.setFont(u8g2_font_logisoso32_tf);
  width = u8g2.getUTF8Width(text);
  u8g2.setFontMode(0);
  u8g2.setFontPosTop();


  int i=0;
 // for (int i = 0 ; i < 128 + width*3 ; i++ ){
  u8g2.setContrast(0);
  u8g2.clearBuffer();
  //u8g2.firstPage();
  //u8g2.setFont(u8g2_font_t0_22b_tf);
  //u8g2.drawUTF8(i, 2, "CodeLock");
  u8g2.setFont(u8g2_font_logisoso22_tf);
  u8g2.drawUTF8(i, 0, "CodeLock");
  //u8g2.setFont(u8g2_font_7x13B_tf);
  //    u8g2.setFont(u8g2_font_helvB10_tr);
  //u8g2.drawUTF8(i, 25, "~ CodeLock V2.0 ~");
  //    u8g2.drawBox(0,48,128,12);
  //   u8g2.setFont(u8g2_font_t0_13b_tf);
  //u8g2.drawUTF8(i, 37, "PIN: *0123456789#");
  u8g2.sendBuffer();
  //    u8g2.nextPage();
  u8g2.setContrast(8);
  delay(ScreenFade);
  u8g2.setContrast(16);
  delay(ScreenFade);
  u8g2.setContrast(32);
  delay(ScreenFade);
  u8g2.setContrast(64);
  delay(ScreenFade);
  u8g2.setContrast(128);
  delay(ScreenFade);
  u8g2.setContrast(255);

  delay(1000);
  //u8g2.drawBox(0,50,48,13);
  //u8g2.setContrast(255);
  //u8g2.drawBox(80,50,48,13);
  // u8g2.setFont(u8g2_font_7x13B_tf);
  //u8g2.setDrawColor(0);
  //u8g2.drawUTF8(i, 51, " * CLR" );
  //u8g2.drawUTF8(80, 51, " # OK " );
  //u8g2.setDrawColor(1);

  //u8g2.sendBuffer();
  
  
  /* write to buffer but do not Display */
  //u8g2.setFont(u8g2_font_7x13B_tf);
  //u8g2.setDrawColor(1);
  u8g2.drawBox(0,50,48,13);
  //u8g2.setContrast(255);
  u8g2.drawBox(80,50,48,13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_7x13B_tf);
  u8g2.drawUTF8(i, 51, " * CLR" );
  u8g2.drawUTF8(80, 51, " # OK " );
  //u8g2.setDrawColor(1);
  delay(1000);
  u8g2.setDrawColor(0);
  u8g2.drawBox(0,0,128,24);
  u8g2.setDrawColor(1);

  u8g2.setContrast(127);
  delay(ScreenFade);
  u8g2.setContrast(63);
  delay(ScreenFade);
  u8g2.setContrast(31);
  delay(ScreenFade);
  u8g2.setContrast(15);
  delay(ScreenFade);
  u8g2.setContrast(7);
  delay(ScreenFade);
  u8g2.setContrast(255);
  u8g2.setPowerSave(1);
  //u8g2.nextPage();
  //} //for
  /* following will clear Buffer */
  //u8g2.clearBuffer();
  //u8g2.sendBuffer(); will clear Display
#endif //DisplayEnabled


#ifdef DisplayEnabled_2
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x78)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  for (int16_t i = 64; i < 71; i++) {
    if (i == '\n') display.write(' ');
    else          display.write(i);
  }
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setCursor(0, 16);     // Start at top-left corner
  display.setTextColor(SSD1306_WHITE); // Draw inverted text
  display.write("Test");
  for (int16_t i = 97; i < 120; i++) {
    display.write(i);
  }
  display.display();
#endif //DisplayEnabled_2

  /*Passwörter und Rechte */ {
#ifdef OwnPasswordFile
  #include "Passwords.h"
#else
  #include "ExamplePasswords.h"
#endif //OwnPasswordFile
  }

  /* Hardware Konfiguration */ {
    //Ausgabe
    pinMode(KEY_LED, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(KEY_LED, OUTPUT);
    pinMode(TUEROEFFNER, OUTPUT);
    pinMode(LICHT, OUTPUT);
    pinMode(RELAIS_3, OUTPUT);

    analogWrite(KEY_LED, KEY_LED_DEFAULT);
    noTone(BUZZER);
    digitalWrite(TUEROEFFNER, HIGH);
    digitalWrite(LICHT, HIGH);
    digitalWrite(RELAIS_3, HIGH);

    //Alarm
    //digitalWrite(TUERKONTAKT, HIGH);
    //digitalWrite(TASTATURKONTAKT, HIGH);
    pinMode(TUERKONTAKT, INPUT_PULLUP);
    pinMode(TASTATURKONTAKT, INPUT_PULLUP);



  }

/* Software Initialisierung */ {
#ifdef SerialEnabled
    Serial.begin(9600);
#endif //SerialEnabled

    logSerialMessage(F("start Ethernet..."));

#ifdef EthernetShieldEnabled
    Ethernet.begin(mac, ip);
    Ethernet.setRetransmissionCount(3);  // configure the Ethernet controller to only attempt one (default 8)
    Ethernet.setRetransmissionTimeout(50);  // set the Ethernet controller's timeout to 50 ms (default 200)
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      logSerialMessage("Ethernet: unknown");
    }
    else if (Ethernet.hardwareStatus() == EthernetW5100) {
      logSerialMessage("Ethernet: W5100");
    }
    else if (Ethernet.hardwareStatus() == EthernetW5200) {
      logSerialMessage("Ethernet: W5200");
    }
    else if (Ethernet.hardwareStatus() == EthernetW5500) {
      logSerialMessage("Ethernet: W5500");
    }
      
    Udp.begin(portLocal);
    timeClient.begin();
//      // ethernet-link Status only works on W5200 and W5500 chips
//      if (Ethernet.linkStatus() == Unknown) {
//        logSerialMessage("Link: unknown");
//      }
//      else if (Ethernet.linkStatus() == LinkON) {
//        logSerialMessage("Link: ON");
//      }
//      else if (Ethernet.linkStatus() == LinkOFF) {
//        logSerialMessage("Link: OFF");
//      }

#endif //EthernetShieldEnabled

    logSerialMessage(F("Codelock Serial V1.0"));


    //RTC
#ifdef RTCEnabled
    if (! rtc.isrunning()) {
      rtc.adjust(DateTime(F("Jan  1 2000"), F("00:00:00")));
    }
    timeClient.begin();
#endif //RTCEnabled

  } //Software Initialisierung

}


void loop(void) {
  
wdt_reset();

#ifdef OneWireEnabled
  check1Wire();
#endif //OneWireEnabled

#ifdef RTCEnabled
  if ((millis() - updateTimeNTP) > NTP_REFRESH * 1000) {
    timeClient.update();
    long diff2ntp = rtc.now().unixtime() - timeClient.getEpochTime();
    String ntpStr = F("NTP-Zeit(UTC): ");
    ntpStr += timeClient.getFormattedTime();
    ntpStr += " Diff:";
    ntpStr += diff2ntp;
    logMessage(ntpStr);
    rtc.adjust(DateTime(timeClient.getEpochTime()));
    updateTimeNTP = millis();
    /* adjust milliOffset */
    int adjSecond = rtc.now().second();
    do {
      //logMessage("dbug 1: "+String(adjSecond)+" > "+String(rtc.now().second()));
    } while ( adjSecond >= rtc.now().second());
    milliOffset = (millis() % 1000 );
    logMessage("ms_offs: "+String(milliOffset));
  }

#endif //RTCEnabled

#ifdef DisplayEnabled
  #ifdef RTCEnabled
    if (rtc.now().unixtime() > screenSaveStart + ScreenSaveDelay) {
      screenSave(1);
    } else {
      screenSave(0);
    }
  #else //RTCEnabled
      if (millis() > screenSaveStart + ScreenSaveDelay * 1000) {
        screenSave(1);
      } else {
        screenSave(0);
      }
  #endif //RTCEnabled
#endif //Display Enabled


  //void loop(){
  //  char customKey = keypad.getKey();
  //  if (customKey){

  char customKey = myKeypad.getKey();
  if (customKey != NO_KEY) {
#ifdef DisplayEnabled
  #ifdef RTCEnabled
    screenSaveStart = rtc.now().unixtime();
  #else //RTCEnabled
    screenSaveStart = millis();
  #endif //RTCEnabled
#endif //DisplayEnabled
int pwdAccess = 0;
    switch (customKey) {
      case '#': KEY_ACK();
        clearPin();
        //displayPin(PWDCount, customKey);
        logMessage("#");
        //Serial.print('\t');
        PWDCount = 0;
        pwdAccess = checkPWDs(PWD);
        grantAccess(pwdAccess);
        PWD = 0;
        break;
      case '*': KEY_ACK();
        clearPin();
        logMessage("*");
        PWDCount = 0;
        PWD = 0;
        break;
      default:  KEY_ACK();
        logMessage((String)customKey);
        #ifdef DisplayEnabled
          displayPin(PWDCount, customKey);
        #endif // DisplayEnabled
        PWDCount += 1;
        PWD *= 10;
        PWD += (customKey - 0x30);
    }
  }

  delay(25);
#ifdef DoorAlarmEnabled
  if (digitalRead(TUERKONTAKT) == 1) {
    if (alarmDoor) { // Alarm stand bereits an
      //alarmCountDoor++;
      alarmCountDoor = (millis() - alarmStartDoor);
      if ((alarmCountDoor / 25) % 72000 == 0) { // alle 30Minuten (1000*60*30=1800000/25=72000)(25ms Toleranz)
        String message = "Sagotage Alarm Tür aktiv seit über ";
        if ((alarmCountDoor >= 1000) && (alarmCountDoor < 120000)) { // < 2Minuten
          message += (alarmCountDoor / 1000);
          message += ",";
          message += ((alarmCountDoor % 1000) / 100);
          message += " Sekunden!";
        }
        if ((alarmCountDoor >= 120000) && (alarmCountDoor < 7200000)) { // >= 2 Minuten
          message += (alarmCountDoor / 60000);
          message += ",";
          message += ((alarmCountDoor % 60000) / 6000);
          message += (" Minuten!");
        }
        if (alarmCountDoor >= 7200000) { // >= 2 Stunden
          message += (alarmCountDoor / 3600000);
          message += ",";
          message += ((alarmCountDoor % 3600000) / 360000);
          message += (" Stunden!");
        }
        logMessage(message);
      }
    } else { // Alarm aufgetreten
      alarmDoor = true;
      alarmStartDoor = millis();
      logMessage(F("!!! SABOTAGE ALARM TÜR !!!"));
      clearPin();
      usedFlag = true;
    }
  } else { // Tür geschlossen
    if ((alarmDoor)) { // Alarm stand aber an
      //alarmCountDoor++;
      alarmCountDoor = (millis() - alarmStartDoor);
      String message = "Sabotage Alarm Tür beendet: (Dauer: ";
      if (alarmCountDoor < 1000) { // < 2Minuten
        message += (alarmCountDoor);
        message += " ms)";
      }
      if ((alarmCountDoor >= 1000) && (alarmCountDoor < 120000)) { // < 2Minuten
        message += (alarmCountDoor / 1000);
        message += ",";
        message += ((alarmCountDoor % 1000) / 100);
        message += " Sekunden)";
      }
      if ((alarmCountDoor >= 120000) && (alarmCountDoor < 7200000)) { // >= 2 Minuten
        message += (alarmCountDoor / 60000);
        message += ",";
        message += ((alarmCountDoor % 60000) / 6000);
        message += " Minuten)";
      }
      if (alarmCountDoor >= 7200000) { // >= 2 Stunden
        message += (alarmCountDoor / 3600000);
        message += (",");
        message += ((alarmCountDoor % 3600000) / 3600000);
        message += " Stunden)";
      }
      logMessage(message);
      alarmCountDoor = 0;
      alarmDoor = false;
      alarmStartDoor = 0;
      usedFlag = true;
    }
  }
#endif //DoorAlarmEnabled

#ifdef KeyboardAlarmEnabled
  // TODO: Keyboard-Sabotage-Alarm auch bei geöffneter Tür
  if (digitalRead(TASTATURKONTAKT) == 1) {
    if (alarmKeypad) { // Alarm stand bereits an
      //alarmCountKeypad++;
      alarmCountKeypad = (millis() - alarmStartKeypad);
      if ((alarmCountKeypad / 25) % 72000 == 0) { // alle 30Minuten (1000*60*30=1800000/25=72000)(25ms Toleranz)
        String message = "Sagotage Alarm Tastatur aktiv seit über ";
        if ((alarmCountKeypad >= 1000) && (alarmCountKeypad < 120000)) { // < 2Minuten
          message += (alarmCountKeypad / 1000);
          message += ",";
          message += ((alarmCountKeypad % 1000) / 100);
          message += " Sekunden!";
        }
        if ((alarmCountKeypad >= 120000) && (alarmCountKeypad < 7200000)) { // >= 2 Minuten
          message += (alarmCountKeypad / 60000);
          message += ",";
          message += ((alarmCountKeypad % 60000) / 6000);
          message += " Minuten!";
        }
        if (alarmCountKeypad >= 7200000) { // >= 2 Stunden
          message += (alarmCountKeypad / 3600000) + (",");
          message += ((alarmCountKeypad % 3600000) / 360000);
          message += " Stunden!";
        }
        logMessage(message);
      }
    } else { // Alarm aufgetreten
      alarmKeypad = true;
      alarmStartKeypad = millis();
      logMessage(F("!!! SABOTAGE ALARM TASTATUR !!!"));
      clearPin();
      usedFlag = true;
    }
  } else { // Tür geschlossen
    if ((alarmKeypad)) { // Alarm stand aber an
      //alarmCountKeypad++;
      alarmCountKeypad = (millis() - alarmStartKeypad);
      String message = "Sabotage Alarm Tastatur beendet: (Dauer: ";
      if (alarmCountKeypad < 1000) { // < 2Minuten
        message += (alarmCountKeypad);
        message += " ms)";
      }
      if ((alarmCountKeypad >= 1000) && (alarmCountKeypad < 120000)) { // < 2Minuten
        message += (alarmCountKeypad / 1000);
        message += ",";
        message += ((alarmCountKeypad % 1000) / 100);
        message += " Sekunden)";
      }
      if ((alarmCountKeypad >= 120000) && (alarmCountKeypad < 7200000)) { // >= 2 Minuten
        message += (alarmCountKeypad / 60000);
        message += ",";
        message += ((alarmCountKeypad % 60000) / 6000);
        message += " Minuten)";
      }
      if (alarmCountKeypad >= 7200000) { // >= 2 Stunden
        message += (alarmCountKeypad / 3600000);
        message += (",");
        message += ((alarmCountKeypad % 3600000) / 3600000);
        message += (" Stunden)");
      }
      logMessage(message);
      alarmCountKeypad = 0;
      alarmKeypad = false;
      alarmStartKeypad = 0;
      usedFlag = true;
    }
  }
#endif //KeyboardAlarmEnabled

}



#ifdef OneWireEnabled
void check1Wire(void) {
  present = 0;
  String dsMessage = "";
  byte continueConvert = 0;
  if (power) {
    if (ds.read_bit()) {
      continueConvert = 1;
    }
  } else {
    if ((millis() - startConvert) > 750) {
      continueConvert = 1;
    }
  }
  if (continueConvert) {

    if (convert == 0) {
      if ( !ds.search(addr)) {
        //logMessage(F("Keine weiteren Addressen."));
        /*if ( dsID >= 1 ) {
          logMessage(" (" + String(dsCount[1])+ ")");
        }*/
        if ( dsCount[1] == 0 ) {
          //logMessage("");
        }
         
        ds.reset_search();
        dsID = 0;
        return;
      } else {
        dsID += 1;
      }

      ds.reset();
      ds.write(SkipROM);
      ds.write(ReadPowerSupply);
      power = ds.read_bit();



      dsMessage = ("R=");
      //Serial.print("R=");
      for ( i = 0; i < 8; i++) {
        dsMessage += " ";
        dsMessage += hexString(addr[i]);
      }
      //## logMessage(dsMessage); //##

      if ( OneWire::crc8( addr, 7) != addr[7]) {
        logMessage(F("CRC nicht gültig!"));
        return;
      } else {
        //logMessage(F("[CRC OK]")); //##
      }
      if ( addr[0] == 0x10) {
        type_s = 1;
        dsMessage += " (DS18S20)";
      }
      else if ( addr[0] == 0x28) {
        type_s = 0;
        dsMessage += " (DS18B20)";
      }
      else if ( addr[0] == 0x22) {
        type_s = 0;
        dsMessage += " (DS1822)";
      }
      else {
        dsMessage += " Gerätefamilie unbekannt : 0x" + hexString(addr[0]);
        logMessage(dsMessage);
        return;
      }
      //Serial.println();
      ////####  logMessage(dsMessage); //##

      ds.reset();
      ds.select(addr);
      ds.write(ConvertT, 1);        // start Konvertierung, mit power-on am Ende


      startConvert = millis();
      convert = 1;
    } else {

      // man sollte ein ds.depower() hier machen, aber ein reset tut das auch
      present = ds.reset();
      ds.select(addr);
      ds.write(ReadScratchPad);         // Wert lesen


      //Serial.print("P=");
      //Serial.print(present,HEX);
      //printHex(present);
      //Serial.print(" ");


      ////####  dsMessage= "P=" + hexString(present) + " ";

      for ( i = 0; i < 9; i++) {           // 9 bytes
        data[i] = ds.read();
        ////####    dsMessage+= hexString(data[i]);
        ////####    dsMessage+= " ";
      }


      //##//##logMessage(dsMessage); //##
      //Serial.print(" CRC=");
      ////Serial.print( OneWire::crc8( data, 8), HEX);
      //printHex( OneWire::crc8( data, 8));
      //           // Serial.println();

      //## dsMessage+= " CRC=";
      //Serial.print( OneWire::crc8( data, 8), HEX);
      //## dsMessage+= hexString( OneWire::crc8( data, 8));
      //          //  Serial.println();
      ////####   dsMessage+= "  ";
      if (power) {
        //Serial.print("  External");
        dsMessage += " [" + String(dsID) + "] External";
     } else {
        //Serial.print("  Parasite");
        dsMessage += " [" + String(dsID) + "] Parasite";
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
       //get average of values
       dsAverage[dsID-1]+=raw;
       dsCount[dsID-1]++;

#ifdef Celsius
      celsius = (float)raw / 16.0;
      dsMessage += " T=" + String(celsius) + " °C";
#endif //Celsius

#ifdef Fahrenheit
      fahrenheit = (float)raw / 16.0 * 1.8 + 32.0;
      dsMessage += " T=" + String(fahrenheit) + " °F";
#endif //Fahrenheit

      raw = (dsAverage[dsID-1] * 8) / dsCount[dsID-1];
//      logMessage(dsMessage);

//if (dsCount[dsID-1] == tempAverageCount ) {
//     if (dsID == 1 ) {
//        logMessage("§");
//     }
//   }
   
if (dsCount[dsID-1] >= tempAverageCount ) {    
   dsMessage += " Avg(" + String(dsCount[dsID-1]) + ")";
   

#ifdef Celsius
      celsius = (float)raw / 128.0;
      dsMessage += " T=" + String(celsius) + " °C";
#endif //Celsius
#ifdef Fahrenheit
      fahrenheit = (float)raw / 128.0 * 1.8 + 32.0;
      dsMessage += " T=" + String(fahrenheit) + " °F";
#endif //Fahrenheit
    dsCount[dsID-1]=0;
    dsAverage[dsID-1]=0;
    logMessage(dsMessage);
  }      
    //if (dsID == 1) {
    //  if (dsCount[dsID-1] > 0 ) {
    //    //logMessage(String(dsCount[dsID-1])+":"+dsMessage);
    //    logMessage(dsMessage);
    //  }
    //}
    dsMessage = "";
    convert = 0;
    }
  }
}
#endif //OneWireEnabled

void printHex(int value) {
  if (value < 0x10) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

String hexString(int value) {
  String hexReturn = "";
  if (value < 0x10) {
    hexReturn += "0";
  }
  hexReturn += String(value, HEX);
  return hexReturn;
}


int grantAccess(int access) {

  // TODO: Abbruch bei Tastendruck ????
  if (access == 0) {
    logMessage(F("PIN Falsch!"));
#ifdef DisplayEnabled
    displayPinText(" Fehler");
#endif //DiplayEnabled
    deactivateDoor();
    LED_NACK();
    clearPin();
    return 0;
  } else {
    if (bitRead(access, 1) == false) {
      logMessage(F("PIN hat keine Rechte für Paketfach!"));
#ifdef DisplayEnabled
      displayPinText(" Fehler");
#endif //DiplayEnabled
      deactivateDoor();
      LED_NACK();
      clearPin();
      return 0;
    }
    if ((bitRead(access, 4) == false) && (usedFlag == true)) {
      logMessage(F("PIN Paketfach bereits belegt"));
#ifdef DisplayEnabled
      displayPinText(" Fehler");      
#endif //DiplayEnabled

      deactivateDoor();
      LED_NACK();
      clearPin();
      return 0;
    }

    logMessage(F("PIN OK"));
#ifdef DisplayEnabled
    displayPinText(" PIN OK");
#endif //DiplayEnabled
    activateDoor();
    if (bitRead(access, 0)) { // Master-PIN resettet "usedFlag"
      usedFlag = false;
    }

    bool tmpOpen = openDoor(10);
#ifdef DisplayEnabled
    displayPinText(" PIN OK");
#endif //DisplayEnabled
    doorOpened(tmpOpen);
    if (tmpOpen) {
      if (!bitRead(access, 0)) { // Master-PIN resettet "usedFlag"
        usedFlag = true;
      }
      clearPin();
      doorClosed(closeDoor(50, bitRead(access, 2)));
    }
    deactivateDoor();
    clearPin();
#ifdef DisplayEnabled
    displayPinText("D A N K E");
#endif //DiplayEnabled
    return 1;
  }
}

String getMillis() {
  unsigned long tmp = millis();
  String tmpStr = "";
  int tmpSec = tmp / 1000;
  int tmpMil = tmp % 1000;
  tmpStr += formatNumber((int)tmpSec, 7, ' ') + '.' + formatNumber(tmpMil, 3, '0') + ": ";
  return tmpStr;
}

String getSubSec() {
  unsigned long tmp = millis();
  String tmpStr = "";
  int tmpMil = (tmp - milliOffset) % 1000;
  tmpStr += formatNumber(tmpMil, 3, '0');
  return tmpStr;
}

void activateDoor() {
  digitalWrite(KEY_LED, HIGH);
  digitalWrite(TUEROEFFNER, LOW);
}

void deactivateDoor() {
  //digitalWrite(KEY_LED, LOW);
  digitalWrite(TUEROEFFNER, HIGH);
}

void doorClosed(bool flag) {
  if (flag) {
    analogWrite(KEY_LED, KEY_LED_DEFAULT);
    logMessage(F("Tür geschlossen."));
  } else {
    logMessage(F("Tür NICHT geschlossen!"));
  }
}

void doorOpened(bool flag) {
  if (flag) {
    digitalWrite(KEY_LED, LOW);
    analogWrite(KEY_LED, KEY_LED_OPEN);
    logMessage("Tür geöffnet.");
  } else {
    logMessage(F("Tür NICHT geöffnet!"));
  }
}

int openDoor(byte seconds) {
  for (int i = 0; i < seconds * 10; i++) {
    digitalWrite(TUEROEFFNER, LOW);
    delay(100);
    if (seconds % 2 == 0){
      wdt_reset();
    }
    if (digitalRead(TUERKONTAKT) == 1) {
      digitalWrite(TUEROEFFNER, HIGH);
      return 1;
    }
  }
  digitalWrite(TUEROEFFNER, HIGH);
  return 0; //Timeout
}

int closeDoor(byte seconds, bool light) {
  for (int i = 0; i < seconds * 10; i++) {
     if (seconds % 2 == 0){
      wdt_reset();
    }
   if (digitalRead(TUERKONTAKT) == 0) {
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
   Gibt "Rechte" Zurück wenn Passwort vergleich OK
   Wenn Passwort falsch dann wird 0 zurückgegeben
*/
int checkPWDs(long password) {

  int eeByte;
  int eeCount = 0;
  long eePassword = 0;

  while (EEPROM[eeCount] != 0) {
    eePassword = 0;
    while (EEPROM[eeCount] < 0x80)  {
      eeByte = EEPROM[eeCount++] - 0x30;
      if (EEPROM[eeCount - 1] == 0) {
        //Serial.println("END!");
        return 0;
      }
      eePassword *= 10;
      eePassword += eeByte;
    }
    eeCount++;
    if (password == eePassword) {
      return EEPROM[eeCount - 1];
    }
  }
  return 0;
}

String formatNumber(unsigned long number, byte digit, char fill) {
  String tmpStr = "";
  for (byte i = digit - 1; i > 0 ; i--) {
    long value = 10;
    for (byte j = 1; j < i; j++) {
      value *= 10;
    }
    if (number < value) {
      tmpStr += fill;
    }
  }
  tmpStr += number;
  return tmpStr;
}

void logMessage(String message) {
#ifdef RTCEnabled
  message = getTimeStr() + " (" + getSubSec() + "): " + message;
#else
  message = getMillis() + message;
#endif //RTCEnabled

#ifdef DisplayEnabled_1
  display.printFixed(0, 0, getClockStr(), STYLE_BOLD);
#endif //DisplayEnabled_1

#ifdef DisplayEnabled
  //ToDo:
#endif //DisplayEnabled

#ifdef SerialEnabled
  Serial.println(message);
#endif //SerialEnabled

#ifdef EthernetShieldEnabled
  char messageBuffer[message.length()];
  message.toCharArray(messageBuffer, message.length() + 1);
  Udp.beginPacket(ipRemote, portServer);
  Udp.write(messageBuffer, message.length());
   if (!Udp.endPacket()) {
    logSerialMessage("ERROR: sending UDP paket failed!");
//   } else {
//    logSerialMessage("successful send UDP paket");
   }
#endif //EthernetShieldEnabled
}

void logSerialMessage(String msg) {
  #ifdef RTCEnabled
  msg = getTimeStr() + " (" + getSubSec() + "): " + msg;
#else
  msg = getMillis() + msg;
#endif //RTCEnabled
#ifdef SerialEnabled
  Serial.println(msg);
#endif //SerialEnabled

}


void KEY_ACK() {
  statusLED(ON);
  delay(150);
  statusLED(OFF);
}

void LED_ACK() {
  statusLED(ON);
  delay(2000);
  wdt_reset();
  statusLED(OFF);
}

void LED_NACK() {
  statusLED(ON);
  delay(100);
  statusLED(OFF);
  delay(100);
  statusLED(ON);
  delay(100);
  statusLED(OFF);
  delay(100);
  statusLED(ON);
  delay(100);
  statusLED(OFF);
  delay(100);
  statusLED(ON);
  delay(100);
  statusLED(OFF);
  delay(100);
  statusLED(ON);
  delay(100);
  statusLED(OFF);
  delay(100);
  statusLED(ON);
  delay(100);
  statusLED(OFF);
  delay(100);
  statusLED(ON);
  delay(100);
  statusLED(OFF);
  delay(100);
  wdt_reset();
}

void statusLED(int value) {
  digitalWrite(KEY_LED, value);
  if (value == ON) {
    //digitalWrite(KEY_LED, HIGH);
    analogWrite(KEY_LED, 0x00);
    tone(BUZZER,BUZZER_HIGH);
  } else {
    //digitalWrite(KEY_LED, LOW);
    analogWrite(KEY_LED, 0xff);
    noTone(BUZZER);
  }
}


//RTC tests
#ifdef RTCEnabled
/*Function: Display time on the serial monitor*/
void printTime()
{
  String timeClock = "RTC: ";
  timeClock += getTimeStr();
  logMessage(timeClock);
}



/*Function: Display time on the serial monitor*/
String getTimeStr()
{
  String retStr = "";
  DateTime now = rtc.now();
  switch (now.dayOfTheWeek())// Friendly printout the weekday
  {
    case 1:
      retStr += "Mo";
      break;
    case 2:
      retStr += "Di";
      break;
    case 3:
      retStr += "Mi";
      break;
    case 4:
      retStr += "Do";
      break;
    case 5:
      retStr += "Fr";
      break;
    case 6:
      retStr += "Sa";
      break;
    case 7:
      retStr += "So";
      break;
    default:
      retStr += "??";
  }
  retStr += " ";
  if (now.day() < 10) {
    retStr += " ";
  }
  retStr += now.day();
  if (now.month() < 10) {
    retStr += ".0";
  } else {
    retStr += ".";
  }
  retStr += now.month();
  retStr += ".";
  retStr += now.year();
  if (now.hour() < 10) {
    retStr += " 0";
  } else {
    retStr += " ";
  }
  retStr += now.hour();
  if (now.minute() < 10) {
    retStr += ":0";
  } else {
    retStr += ":";
  }
  retStr += now.minute();

  if (now.second() < 10) {
    retStr += ":0";
  } else {
    retStr += ":";
  }
  retStr += now.second();
  //retStr+= " ";
  //retStr+= now.unixtime();
  return retStr;
}

char* getClockStr()
{
  String retStr;
  static char retString[16] = "               ";
  DateTime now = rtc.now();

  retString[0] = now.hour() / 10 + 0x30;
  retString[1] = now.hour() % 10 + 0x30;
  retString[2] = ':';
  retString[3] = now.minute() / 10 + 0x30;
  retString[4] = now.minute() % 10 + 0x30;
  retString[5] = ':';
  retString[6] = now.second() / 10 + 0x30;
  retString[7] = now.second() % 10 + 0x30;
  retString[8] = ' ';
  retString[9] = ' ';
  retString[10] = now.day() / 10 + 0x30;
  retString[11] = now.day() % 10 + 0x30;
  retString[12] = '.';
  retString[13] = now.month() / 10 + 0x30;
  retString[14] = now.month() % 10 + 0x30;
  retString[15] = '.';
  retString[16] = '\0';
  return retString;

  if (now.day() < 10) {
    retStr += " ";
  }
  retStr += now.day();
  if (now.month() < 10) {
    retStr += ".0";
  } else {
    retStr += ".";
  }
  retStr += now.month();
  retStr += ".";
  retStr += now.year();
  if (now.hour() < 10) {
    retStr += " 0";
  } else {
    retStr += " ";
  }
  retStr += now.hour();
  if (now.minute() < 10) {
    retStr += ":0";
  } else {
    retStr += ":";
  }
  retStr += now.minute();

  if (now.second() < 10) {
    retStr += ":0";
  } else {
    retStr += ":";
  }
  retStr += now.second();
  retStr += " ";
  return retString;
}
#endif //RTCEnabled

#ifdef DisplayEnabled
/* enable and disable display */
void screenSave(bool value) {
  if (value ^ screenSaveStatus == 1) {
    //u8g2.setPowerSave(value);
    screenSaveStatus = value;
    u8g2.setContrast((!value)*255);
    if (value == 1) {
      logMessage("gehe schlafen...");
      u8g2.setContrast(128);
      delay(ScreenFade);
      u8g2.setContrast(64);
      delay(ScreenFade);
      u8g2.setContrast(32);
      delay(ScreenFade);
      u8g2.setContrast(16);
      delay(ScreenFade);
      u8g2.setContrast(8);
      delay(ScreenFade);
      u8g2.setContrast(4);
      clearPin();
      u8g2.setPowerSave(value);
     } else {
      //logMessage("wache auf..."); // will delay key input
      u8g2.setPowerSave(value);
      u8g2.setContrast(8);
      delay(ScreenFade);
      u8g2.setContrast(16);
      delay(ScreenFade);
      u8g2.setContrast(32);
      delay(ScreenFade);
      u8g2.setContrast(128);
      delay(ScreenFade);
      u8g2.setContrast(255);
    }
  }
}
#endif //DisplayEnabled


#ifdef DisplayEnabled
void displayPin(int pos, char key) {
  char pinKey[] ={key, '\0'};
  u8g2.setPowerSave(0);
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_logisoso22_tf);
  u8g2.drawUTF8( (pos * 20 ) + 10, 0, pinKey);
  u8g2.sendBuffer();
  u8g2.setPowerSave(0);
}

void displayPinText(char *text) {
  u8g2.setPowerSave(0);
  u8g2.setDrawColor(0);
  u8g2.drawBox(0,0,128,24);
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_logisoso22_tf);
  u8g2.drawStr( 0, 0, text);
  u8g2.sendBuffer();
  u8g2.setPowerSave(0);
}

void clearPin(void) {
  //PWDCount = 0;
  //PWD = 0;
  u8g2.setPowerSave(0);
  u8g2.setDrawColor(0);
  u8g2.drawBox(0,0,128,24);
  //u8g2.setDrawColor(1);
  u8g2.sendBuffer();
  u8g2.setPowerSave(0);
}

#else //DisplayEnabled
void clearPin(void) {
  //PWDCount = 0;
  //PWD = 0;
}
#endif //DisplayEnabled
