/* Konfiguration */
#define DoorAlarmEnabled
#define KeyboardAlarmEnabled
#define LightEnabled

#define ScreenSaveDelay 15 // seconds
#define ScreenFade 10 // ms

#define BUZZER_LOW 500
#define BUZZER_HIGH 1000

#define ON 1
#define OFF 0

#define OwnPasswordFile

#define OneWireEnabled
#define tempAverageCount 28 // Count for values used for average value 
#define tempAverageSeconds 60 // Average of temperature ensors over x seconds
#define Celsius
//#define Fahrenheit

#define SerialEnabled
#define EthernetShieldEnabled
#define RTCEnabled
//#define DisplayEnabled_1// lcdgfx-library (lower memory consumtion ~4k)
//#define DisplayEnabled_2  // Ardafruit-library (lot of memory needed ~10k + fonts)
#define DisplayEnabled // U8G2-library (lot of memory needed ~8K + fonts)

/* Keyboard configuration */
#define KEY_FOIL
/* Subtype for KEY_FOIL */
#define KEY16   //ignored if NOT foil-keypad
/* define only one Keypad-Type */
#define KEY_PIN   //PINS
//#define KEY_PCF   //PCF8754
//#define KEY_MCP   //MCP23008

#ifndef KEY_PIN
  #define I2CADDR 0x20
#endif //KEY_PIN
//#warning "########################"I2CADDR
//#error "########################"I2CADDR
/* Ethernet-Shield */
#define MAC_LOCAL 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
#define IP_LOCAL 192, 168, 1, 9
#define PORT_UDP_LOCAL 8888

#define IP_UDP_SERVER 192, 168, 1, 218
#define PORT_UDP_SERVER 4711

#define NTP_OFFSET  0*3600 // UTC=0hCET=+1h; CEST=+2h
#define NTP_REFRESH 5*60*1000 // every 5 Minutes
