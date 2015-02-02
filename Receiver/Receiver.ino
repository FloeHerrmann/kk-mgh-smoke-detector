#include <SPI.h>
#include <SD.h>
#include <Streaming.h>
#include <Watchdog.h>

#define SD_CARD_SELECT 4

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