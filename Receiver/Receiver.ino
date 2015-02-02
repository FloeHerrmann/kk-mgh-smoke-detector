#include <SPI.h>
#include <SD.h>
#include <IniFile.h>
#include <Ethernet.h>
#include <Streaming.h>
#include <Watchdog.h>

#define SD_CARD_SELECT 4

char WiFiSSID[32] = "";
char WiFiPassphrase[32] = "";
char WiFiSecurity[40] = "";

// Globaler Buffer zur anwendungsweiten Verwendung
#define ApplicationBufferSize 200
char ApplicationBuffer[ ApplicationBufferSize ];

void setup(){

	// Serielle Schnittstelle öffnen
	Serial.begin( 115200 );

	Serial << "* * * * * * * * * * * * * * * * * * * * * * * * * * * * *" << endl;
	Serial << " Intelligenter Rauchmelder - Empfangsstation" << endl;
	Serial << "* * * * * * * * * * * * * * * * * * * * * * * * * * * * *" << endl;

	// Watchdog starten (5 Sekunden)
	WatchdogInitialize();

	// SD Karte initialisieren
	bool isInitialized = SDCardInitialize();

	// Überprüfen ob Datei config.ini vorhanden ist
	if( isInitialized ) {
		Serial << F( "[" ) << millis() << F( "] ") << F( "Datei 'config.ini' vorhanden..." );
		if( SD.exists( "config.ini") ) {
			Serial << F( "OK" ) << endl;
			
			bool isLoaded = SDCardLoadConfiguration();

		} else {
			Serial << F( "FEHLER" ) << endl;
		}
	}

}

void loop() {

	// Test des Watchdogs
	delay( 1000 );
	if( millis() > 15000 ) {
		for(;;){
			Serial << millis() << endl;	
			delay( 1000 );
		}
	}
	WatchdogReset();

}

// Verbindung zur SD Karte initialisieren
bool SDCardInitialize(){

	WatchdogReset();

	Serial << F( "[" ) << millis() << F( "] ") << F( "SD Karte initialisieren..." );

	bool isAvailable = SD.begin( SD_CARD_SELECT );

	if( isAvailable == true ) {
		Serial << F( "OK" ) << endl;
		return true;
	} else {
		Serial << F( "FEHLER" ) << endl;
		return false;
	}
}

// * * * Load configuration from emodul5.ini * * *
bool SDCardLoadConfiguration() {

	WatchdogReset();

	IniFile configFile = IniFile( "config.ini" );

	Serial << F( "[" ) << millis() << F( "] " ) << F( "Datei 'config.ini' öffnen..." );

	if( configFile.open() ) {


		Serial << F( "OK" ) << endl;
		Serial << F( "[" ) << millis() << F( "] " ) << F( "Inhalt der 'config.ini' ist vailde..." );

		if( configFile.validate( ApplicationBuffer , ApplicationBufferSize ) ) {
			Serial << F( "OK" ) << endl;

			if( configFile.getValue( "WIFI" , "ssid" , ApplicationBuffer , ApplicationBufferSize ) ){
				strcpy( WiFiSSID , ApplicationBuffer );
				Serial << F( "[" ) << millis() << F( "] " ) << F( "WiFi SSID: " ) << WiFiSSID << endl;
			} else {
				Serial << F( "FEHLER (SSID)" ) << endl;
				return false;
			}

			if( configFile.getValue( "WIFI" , "pass" , ApplicationBuffer , ApplicationBufferSize ) ){
				strcpy( WiFiPassphrase , ApplicationBuffer );
				Serial << F( "[" ) << millis() << F( "] " ) << F( "WiFi Passphrase: " ) << WiFiPassphrase << endl;
			} else {
				Serial << F( "FEHLER (PASS)" ) << endl;
				return false;
			}

			if( configFile.getValue( "WIFI" , "sec" , ApplicationBuffer , ApplicationBufferSize ) ){
				strcpy( WiFiSecurity , ApplicationBuffer );
				Serial << F( "[" ) << millis() << F( "] " ) << F( "WiFi Security: " ) << WiFiSecurity << endl;
			} else {
				Serial << F( "FEHLER (SEC)" ) << endl;
				return false;
			}

			return true;
		} else {
			Serial << F( "FEHLER (VALIDIERUNG)" ) << endl;
			return false;
		}
	} else {
		Serial << F( "FEHLER (ÖFFNEN)" ) << endl;
		return false;
	}

	return true;
}

// Watchdog starten
void WatchdogInitialize(){
	Serial << F( "[" ) << millis() << F( "] ") << F( "Watchdog initialisieren..." );
	watchdog.enable( 5 );
	Serial << F( "OK" ) << endl;
}

// Watchdog zurücksetzen
void WatchdogReset(){
	Serial << F( "[" ) << millis() << F( "] ") << F( "Watchdog zurücksetzen..." );
	watchdog.restart();
	Serial << F( "OK" ) << endl;
}