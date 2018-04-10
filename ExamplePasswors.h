  //------------------------------------------------------------
  // Rechte-Byte:
  //============================================================
  // Bit7: 1= Rechte-Byte
  //       0= Zeichen
  // Bit4: 1= acces every time
  //       0= acces only if not USED / Default-Pin
  // Bit2: 1= Licht einschalten
  //       0= kein Licht
  // Bit1: 1= T�r �ffnen, used-Flag setzen
  //       0= T�r nicht �ffnen
  // Bit0: 1= USED-Flag l�schen
  //       0= USED-Flag setzen
  //------------------------------------------------------------
  
  // um eine eigene Passwortdatei zu benutzen "ExamplePasswords.h" in "Passwords.h" kopieren.
  // Dort können dann die Passwörter und Rechte geändert und erweitert werden.
  // Die maximale-EEPROM-Größe des verwendeten Arduinos darf nicht überschritten werden.
  // in der Config muss dann OwnPasswordFile definiert werden

  long eeCnt=0;

  // Default-PIN#
  EEPROM[eeCnt++] = '0';
  EEPROM[eeCnt++] = 0x86; // nur unbenutzt �ffnen, Licht, "USED" nicht zur�cksetzen
  
  // Spezial-PIN
  EEPROM[eeCnt++] = '0';
  EEPROM[eeCnt++] = '0';
  EEPROM[eeCnt++] = '7';
  EEPROM[eeCnt++] = 0x93; // immer �ffnen, kein Licht, "USED" nicht zur�cksetzen

  // Master-PIN#
  EEPROM[eeCnt++] = '0';
  EEPROM[eeCnt++] = '8';
  EEPROM[eeCnt++] = '1';
  EEPROM[eeCnt++] = '5';
  EEPROM[eeCnt++] = 0x97; // darf alles ;-)

  // Zusteller-PIN#
  EEPROM[eeCnt++] = '4';
  EEPROM[eeCnt++] = '7';
  EEPROM[eeCnt++] = '1';
  EEPROM[eeCnt++] = '1';
  EEPROM[eeCnt++] = 0x96; // immer �ffnen, Licht, "USED" nicht zur�cksetzen
  
  // End-Markierungen !!
  EEPROM[eeCnt++] = 0;   // End-Markierung
  EEPROM[eeCnt++] = 0;   // End-Markierung
