/* Konfiguration */
#define DoorAlarmEnabled
#define KeyboardAlarmEnabled
#define LightEnabled

#define SerialEnabled
#define EthernetShieldEnabled
#define OneWireEnabled

#define OwnPasswordFile

/* Keyboard */
// Folie
//#define KEY_ROWS 8, 7, 6, 5
//#define KEY_COLS 4, 3, 2

// Telefon
//#define KEY_ROWS 2, 3, 4, 5
//#define KEY_COLS 6, 7, 8

// Paketfach
#define KEY_ROWS 2, 3, 8, 7
#define KEY_COLS 4, 5, 6

/* Ethernet-Shield */
#define MAC_LOCAL 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
#define IP_LOCAL 192, 168, 1, 8
#define PORT_UDP_LOCAL 8888

#define IP_UDP_SERVER 192, 168, 1, 123
#define PORT_UDP_SERVER 9999
