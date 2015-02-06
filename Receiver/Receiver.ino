#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <SD.h>
#include <IniFile.h>
#include <Streaming.h>
#include <Watchdog.h>
#include <rtc_clock.h>

#define WIFI_IRQ 3
#define WIFI_VBAT 5
#define WIFI_SELECT 10
#define WIFI_IDLE_TIMEOUT 5000

#define SD_CARD_SELECT 4

char WiFiSSID[32] = "";
char WiFiPassphrase[32] = "";
uint8_t WiFiSecurity = WLAN_SEC_UNSEC;
uint8_t WiFiMacAddress[6];

// HEX Ziffern für MAC Adresse
const char HexLetters[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

// Globaler Buffer zur anwendungsweiten Verwendung
#define ApplicationBufferSize 200
char ApplicationBuffer[ ApplicationBufferSize ];

// RTC (Real Time Clock)
RTC_clock rtc_clock( XTAL );

// WIFI Objekt
Adafruit_CC3000 wifi = Adafruit_CC3000( WIFI_SELECT , WIFI_IRQ , WIFI_VBAT , SPI_CLOCK_DIVIDER );
Adafruit_CC3000_Client wifiClient;

void setup(){

	// Serielle Schnittstelle öffnen
	Serial.begin( 115200 );

	Serial << "* * * * * * * * * * * * * * * * * * * * * * * * * * * * *" << endl;
	Serial << " Intelligenter Rauchmelder - Empfangsstation" << endl;
	Serial << "* * * * * * * * * * * * * * * * * * * * * * * * * * * * *" << endl;

	// Watchdog abschalten
	WatchdogDisable();

	// SD Harware initialisieren
	bool isInitialized = SDCardInitialize();
	if( isInitialized ) {
		Serial << F( "[" ) << millis() << F( "] ") << F( "Datei 'config.ini' vorhanden..." );

		// Überprüfen ob die Datei config.ini existiert
		if( SD.exists( "config.ini") ) {
			Serial << F( "OK" ) << endl;
			
			// Einstellungen aus der config.ini laden
			bool isLoaded = SDCardLoadConfiguration();
			if( isLoaded ) {

				// WiFi Hardware initialisieren
				isInitialized = WiFiInitialize();
				if( isInitialized ) {
					
					// MAC Adresse des WiFi Chips
					WiFiGetMacAddress();
					
					// Alte Verbindungsprofile löschen
					WiFiDeleteOldProfiles();
					
					// Verbindung zum konfigurierten WiFi herstellen
					bool isConnected = WiFiConnectToNetwork();

					if( isConnected ) {
						// IP per DHCP beziehen
						WiFiCheckDHCP();

						// Datum und Uhrzeit vom Server holen
						uint8_t httpState;
						String httpContent;
						HttpGetRequest( "www.pro-kitchen.net" , "/demo/serverTime.do" , httpState , httpContent );

						if( httpState == 200 && !httpContent.equals( "" ) ) {
							// Uhrenbaustein intialisieren
							RtcEnable();

							// Datum und Uhrzeit einstellen
							RtcSetDateTime( httpContent );
						}
					}
				}
			}
		} else {
			Serial << F( "FEHLER" ) << endl;
		}
	}

}

void loop() {

	// Datum und Uhrzeit ausgeben
	Serial << rtc_clock.get_days() << "." << rtc_clock.get_months() << "." << rtc_clock.get_years() << " " ;
	Serial << rtc_clock.get_hours() << ":" << rtc_clock.get_minutes() << ":" << rtc_clock.get_seconds() << endl;
	delay( 2000 );
}

// WiFi Modul initialisieren
bool WiFiInitialize(){

	Serial << F( "[" ) << millis() << F( "] ") << F( "WiFi Modul - Initialisieren..." );

	if( wifi.begin() ){
		Serial << F( "OK" ) << endl;
		return true;
	} else {
		Serial << F( "FEHLER (INIT)" ) << endl;
		return false;
	}
}

// MAC Adresse des WiFi Moduls
bool WiFiGetMacAddress(){
	Serial << F( "[" ) << millis() << F( "] ") << F( "WiFi Modul - MAC Adresse auslesen..." );
	if( wifi.getMacAddress( WiFiMacAddress ) ) {
		Serial << MacAddressToString( WiFiMacAddress ) << endl;
		return true;
	} else {
   		Serial << F( "FEHLER (MAC)" ) << endl;
   		return false;
	}
}

// WiFi Modul initialisieren
bool WiFiDeleteOldProfiles(){

	Serial << F( "[" ) << millis() << F( "] ") << F( "WiFi Modul - Alte Profile löschen..." );

	if( wifi.deleteProfiles() ){
		Serial << F( "OK" ) << endl;
		return true;
	} else {
		Serial << F( "FEHLER (DELETE)" ) << endl;
		return false;
	}
}

// Verbindung zum konfigurierten WiFi herstellen
bool WiFiConnectToNetwork(){

	Serial << F( "[" ) << millis() << F( "] ") << F( "WiFi Modul - Verbindung herstellen..." );

	if( wifi.connectToAP( WiFiSSID , WiFiPassphrase , WiFiSecurity , 2 ) ){
		Serial << F( "OK" ) << endl;
		return true;
	} else {
		Serial << F( "FEHLER (WIFI CONNECT)" ) << endl;
		return false;
	}
}

// IP Adresse, etc. vom DHCP Server beziehen
bool WiFiCheckDHCP(){

	Serial << F( "[" ) << millis() << F( "] ") << F( "WiFi Modul - DHCP Konfiguration..." );

	unsigned long dhcpTimeout = 10000;
	unsigned long dhcpStart = millis();
  
	while ( !wifi.checkDHCP() ){
		if( (millis() - dhcpStart) > dhcpTimeout ) {
			Serial << F( "FEHLER (DHCP)" ) << endl;
			return false;
		}
		delay(100);
	}

	Serial << F( "OK" ) << endl;

	uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  	if( wifi.getIPAddress( &ipAddress , &netmask , &gateway , &dhcpserv , &dnsserv ) ) {
  		Serial << F( "[" ) << millis() << F( "] ") << F("WiFi Modul - IP Addr: ") << IpAddressToString( ipAddress ) << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F("WiFi Modul - Netmask: ") << IpAddressToString( netmask ) << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F("WiFi Modul - Gateway: ") << IpAddressToString( gateway ) << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F("WiFi Modul - DHCPsrv: ") << IpAddressToString( dhcpserv ) << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F("WiFi Modul - DNSserv: ") << IpAddressToString( dnsserv ) << endl;
		return true;
	} else {
		Serial << F( "FEHLER (IP)" );
		return false;
	}

	return true;
}

// Hardware für SD Karte initialisieren
bool SDCardInitialize(){

	Serial << F( "[" ) << millis() << F( "] ") << F( "SD Karte initialisieren..." );

	bool isAvailable = SD.begin( SD_CARD_SELECT );

	if( isAvailable == true ) {
		Serial << F( "OK" ) << endl;
		return true;
	} else {
		Serial << F( "FEHLER < < <  K E I N E  SD KARTE EINGELEGT?" ) << endl;
		return false;
	}
}

// Laden der Konfiguration aus der config.ini
bool SDCardLoadConfiguration() {

	//WatchdogReset();

	IniFile configFile = IniFile( "config.ini" );

	Serial << F( "[" ) << millis() << F( "] " ) << F( "Datei 'config.ini' öffnen..." );

	if( configFile.open() ) {


		Serial << F( "OK" ) << endl;
		Serial << F( "[" ) << millis() << F( "] " ) << F( "Inhalt der 'config.ini' ist vailde..." );

		if( configFile.validate( ApplicationBuffer , ApplicationBufferSize ) ) {
			Serial << F( "OK" ) << endl;

			Serial << F( "[" ) << millis() << F( "] " ) << F( "WiFi SSID: " );
			if( configFile.getValue( "WIFI" , "ssid" , ApplicationBuffer , ApplicationBufferSize ) ){
				strcpy( WiFiSSID , ApplicationBuffer );
				Serial << WiFiSSID << endl;
			} else {
				Serial << F( "FEHLER (SSID)" ) << endl;
				return false;
			}

			Serial << F( "[" ) << millis() << F( "] " ) << F( "WiFi Passphrase: " );
			if( configFile.getValue( "WIFI" , "pass" , ApplicationBuffer , ApplicationBufferSize ) ){
				strcpy( WiFiPassphrase , ApplicationBuffer );
				Serial << WiFiPassphrase << endl;
			} else {
				Serial << F( "FEHLER (PASS)" ) << endl;
				return false;
			}

			Serial << F( "[" ) << millis() << F( "] " ) << F( "WiFi Security: " );
			if( configFile.getValue( "WIFI" , "sec" , ApplicationBuffer , ApplicationBufferSize ) ){
				if( strcmp( ApplicationBuffer , "WPA2" ) == 0 ) {
					WiFiSecurity = WLAN_SEC_WPA2;
					Serial << F("WPA2" ) << endl;
				} else if( strcmp( ApplicationBuffer , "WPA" ) == 0 ) {
					WiFiSecurity = WLAN_SEC_WPA;
					Serial << F("WPA" ) << endl;
				} else if( strcmp( ApplicationBuffer , "WEP" ) == 0 ) {
					WiFiSecurity = WLAN_SEC_WEP;
					Serial << F("WEP" ) << endl;
				} else {
					WiFiSecurity = WLAN_SEC_UNSEC;
					Serial << F("NO SECURITY" ) << endl;
				}
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

// HTTP GET Request durchführen
void HttpGetRequest( char *Server , char *Page , uint8_t &State , String &Content ){

	Serial << F( "[" ) << millis() << F( "] ") << F( "HTTP GET Request an der Server '" ) << Server << F( "' senden" ) << endl;
	
	// IP Adresse des Server beim DNS Server nachfragen
	Serial << F( "[" ) << millis() << F( "] ") << F( "IP Adresse des Servers..." );
	
	uint32_t ipAddress = 0;
	uint64_t dnsTimeout = 10000;
	uint64_t dnsStart = millis();

	while( ipAddress == 0 ) {
		if( !wifi.getHostByName( Server , &ipAddress ) ) {
			if( (millis() - dnsStart) > dnsTimeout ) {
				Serial << F( "FEHLER (DNS)" ) << endl;
				return;
			}
		}
		delay(500);
	}

	Serial << IpAddressToString( ipAddress ) << endl;

	Serial << F( "[" ) << millis() << F( "] ") << F( "Verbindung zum Server aufbauen..." );
	wifiClient = wifi.connectTCP( ipAddress , 80 );
	if( wifiClient.connected() ) {
		Serial << F( "OK" ) << endl;

		// GET /index.html HTTP/1.1
		String request = "GET ";
		request += Page;
		request += " HTTP/1.1\r\n";

		// Host: pro-kitchen.net
		request += "Host: ";
		request += Server;
		request += "\r\n";

		// Connection: close
		request += "Connection: close\r\n";

		// Abschließen
		request += "\r\n\r\n";
		wifiClient.print( request );

		Serial << F( "[" ) << millis() << F( "] ") << F( "Gesendeter HTTP GET Request..." ) << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     GET ") << Page << F(" HTTP/1.1") << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     Host: ") << Server << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     Connection: close") << endl;

		HttpReadResponse( State , Content , WIFI_IDLE_TIMEOUT );
		
		Serial << F( "[" ) << millis() << F( "] ") << F( "Verbindung zum Server schließen..." );
		wifiClient.close();
		Serial << F( "OK" ) << endl;
	} else {
		Serial << F( "FEHLER (VERBINDUNGSAUFBAU" );
		return;
	}
}

// Antwort vom Webserver lesen
void HttpReadResponse( uint8_t &State , String &Content , uint32_t Timeout ){
	uint64_t lastRead = millis();
	String response = "";
	while( wifiClient.connected() && ( millis() - lastRead < Timeout ) ) {
		while( wifiClient.available() ) {
			char c = wifiClient.read();
			response += c;
			lastRead = millis();
		}
	}

	// HTTP Status auslesen
	String state = response.substring( 9 , 12 );
	State = state.toInt();

	// Antwort des Servers
	uint32_t contentStart = response.indexOf( "\r\n\r\n" );
	Content = response.substring( contentStart + 4 , response.length() - 1 );
	
	Serial << F( "[" ) << millis() << F( "] ") << F( "Empfangene HTTP Response..." ) << endl;
	Serial << F( "[" ) << millis() << F( "] ") << F( "     Status ") << State << endl;
	Serial << F( "[" ) << millis() << F( "] ") << F( "     Inhalt: ") << Content << endl;
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

// Watchdog starten
void WatchdogDisable(){
	Serial << F( "[" ) << millis() << F( "] ") << F( "Watchdog abschalten..." );
	watchdog.disable();
	Serial << F( "OK" ) << endl;
}

// Uhrenbaustein initialiiseren
void RtcEnable(){
	Serial << F( "[" ) << millis() << F( "] ") << F( "RTC initialisieren..." );
	rtc_clock.init();
	Serial << F( "OK" ) << endl;
}

// Datum und Uhrzeit setzen
void RtcSetDateTime( String dateTime ) {
	// Datum
	String day = dateTime.substring( 0 , dateTime.indexOf( "." ) );
	if( day.toInt() < 10 ) day.replace( "0" , "" );
	dateTime = dateTime.substring( dateTime.indexOf( "." ) + 1 , dateTime.length() );

	String month = dateTime.substring( 0 , dateTime.indexOf( ".") );
	if( month.toInt() < 10 ) month.replace( "0" , "" );
	dateTime = dateTime.substring( dateTime.indexOf( "." ) + 1 , dateTime.length() );

	String year = dateTime.substring( 0 , dateTime.indexOf( " ") );
	dateTime = dateTime.substring( dateTime.indexOf( " " ) + 1 , dateTime.length() );

	// Uhrzeit
	String hours = dateTime.substring( 0 , dateTime.indexOf( ":" ) );
	if( hours.toInt() < 10 ) hours.replace( "0" , "" );
	dateTime = dateTime.substring( dateTime.indexOf( ":" ) + 1 , dateTime.length() );

	String minutes = dateTime.substring( 0 , dateTime.indexOf( ":") );
	if( minutes.toInt() < 10 ) minutes.replace( "0" , "" );
	dateTime = dateTime.substring( dateTime.indexOf( ":" ) + 1 , dateTime.length() );

	String seconds = dateTime.substring( 0 , dateTime.indexOf( ".") );
	if( seconds.toInt() < 10 ) seconds.replace( "0" , "" );
	dateTime = dateTime.substring( dateTime.indexOf( "." ) + 1 , dateTime.length() );
	
	// Uhrzeit stellen
	rtc_clock.set_time( hours.toInt() , minutes.toInt() , seconds.toInt() );
	// Datum stellen
	rtc_clock.set_date( day.toInt() , month.toInt() , year.toInt() );
}

// String für eine MAC Adresse erzeugen
String MacAddressToString( uint8_t* macAddress ) {
	String macAddr = "";
	for( byte i = 0 ; i < 6 ; i++ ) {
		byte value = macAddress[i];
		byte first = value / 16;
		byte second = value - (first * 16);
		macAddr += HexLetters[ first ];
		macAddr += HexLetters[ second ];
		if( i < 5 ) macAddr += ":";
	}
	return macAddr;
}

// String für eine IP-Adresse erzeugen
String IpAddressToString( uint32_t ipAddress ) {
	String address = "";
	address += ( (uint8_t) ( ipAddress >> 24 ) );
	address += ".";
	address += ( (uint8_t) ( ipAddress >> 16 ) );
	address += ".";
	address += ( (uint8_t) ( ipAddress >> 8 ) );
	address += ".";
	address += ( (uint8_t) ipAddress );
	return address;
}