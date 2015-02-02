#include <Streaming.h>
#include <Watchdog.h>

void setup(){

	// Serielle Schnittstelle öffnen
	Serial.begin( 115200 );

	Serial << "* * * * * * * * * * * * * * * * * * * * * * * * * * * * *" << endl;
	Serial << " Intelligenter Rauchmelder - Empfangsstation" << endl;
	Serial << "* * * * * * * * * * * * * * * * * * * * * * * * * * * * *" << endl;

	// Watchdog starten (5 Sekunden)
	WatchdogInitialize();

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