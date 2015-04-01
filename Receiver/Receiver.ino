#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <SD.h>
#include <IniFile.h>
#include <Streaming.h>
#include <Watchdog.h>
#include <rtc_clock.h>
#include <Panstamp.h>

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

// Received data from the panstamp modem
String ReceivedData = "";

void setup(){

	// Serielle Schnittstelle öffnen
	Serial.begin( 115200 );

	Serial << endl << endl << "* * * * * * * * * * * * * * * * * * * * * * * * * * * * *" << endl;
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

						// Verbindung zum Panstamp herstellen
						panstamp.init();

						Serial << F( "[" ) << millis() << F( "] ") << F( "Panstamp - Command Mode..." ) << panstamp.switchToCommandMode() << endl;
						
						//Serial << F( "[" ) << millis() << F( "] ") << F( "Panstamp - Set Address..." );
						//panstamp.setAddress( "01" );
						//Serial << F( "DONE" ) << endl;
						Serial << F( "[" ) << millis() << F( "] ") << F( "Panstamp - Address..." ) << panstamp.address() << endl;

						Serial << F( "[" ) << millis() << F( "] ") << F( "Panstamp - Set Synchronization Word..." );
						panstamp.setSynchronizationWord( "6432" );
						Serial << F( "DONE" ) << endl;
						Serial << F( "[" ) << millis() << F( "] ") << F( "Panstamp - Synchronization Word..." ) << panstamp.synchronizationWord() << endl;

						Serial << F( "[" ) << millis() << F( "] ") << F( "Panstamp - Firmware..." ) << panstamp.firmwareVersion() << endl;
						Serial << F( "[" ) << millis() << F( "] ") << F( "Panstamp - Hardware..." ) << panstamp.hardwareVersion() << endl;
						Serial << F( "[" ) << millis() << F( "] ") << F( "Panstamp - Data Mode..." ) << panstamp.switchToDataMode() << endl;

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
	//Serial << rtc_clock.get_days() << "." << rtc_clock.get_months() << "." << rtc_clock.get_years() << " " ;
	//Serial << rtc_clock.get_hours() << ":" << rtc_clock.get_minutes() << ":" << rtc_clock.get_seconds() << endl;

	if( Serial2.available() > 0 ) {
		byte CurrentDataChar = Serial2.read();

		if( CurrentDataChar != 10 && CurrentDataChar != 13 ) ReceivedData += (char)CurrentDataChar;

		if( CurrentDataChar == 10 ) {
			if( ReceivedData.length() > 2  ) ProcessReceivedData( ReceivedData );
			ReceivedData = "";
		}
	}	
}

// Processing the received data from the smoke detector
bool ProcessReceivedData( String data ) {

	Serial << F( "[" ) << millis() << F( "] Empfangene Daten..." ) << data << endl;
	Serial << F( "[" ) << millis() << F( "] Verarbeiten der Daten..." ) << endl;

	String addressS = data.substring( 6 , 8 );
	Serial << F( "[" ) << millis() << F( "]      Rauchmelder-Nr: ") << addressS << endl;

	String rssiS = data.substring( 1 , 3 );
	uint32_t rssiI = HexStringToInt( rssiS );
	Serial << F( "[" ) << millis() << F( "]      RSSI: ") << rssiI << endl;

	String uptimeS = data.substring( 8 , 16 );
	uint32_t uptimeI = HexStringToInt( uptimeS );
	uptimeI = uptimeI / 4;
	Serial << F( "[" ) << millis() << F( "]      Betriebszeit: ") << uptimeI << " Sekunden" << endl;

	String smokeChamberS = data.substring( 16 , 20 );
	uint32_t smokeChamberI = HexStringToInt( smokeChamberS );
	float smokeChamberF = (float)smokeChamberI;
	smokeChamberF = smokeChamberF * 0.003223;
	Serial << F( "[" ) << millis() << F( "]      Rauchkammer-Wert: ") << smokeChamberF << endl;	

	String smokeAlarmsS = data.substring( 20 , 22 );
	uint32_t smokeAlarmsI = HexStringToInt( smokeAlarmsS );
	Serial << F( "[" ) << millis() << F( "]      Anz. Rauchalarme: ") << smokeAlarmsI << endl;

	String pollutionS = data.substring( 22 , 24 );
	uint32_t pollutionI = HexStringToInt( pollutionS );
	Serial << F( "[" ) << millis() << F( "]      Verschmutzungsgrad: ") << pollutionI << endl;

	String batteryS = data.substring( 24 , 28 );
	uint32_t batteryI = HexStringToInt( batteryS );
	float batteryF = (float)batteryI;
	batteryF = batteryF * 0.018369;
	Serial << F( "[" ) << millis() << F( "]      Batterie-Spannung: ") << batteryF << endl;	

	String temp01S = data.substring( 28 , 30 );
	uint32_t temp01I = HexStringToInt( temp01S );
	float temp01F = (float)temp01I;
	temp01F = (temp01F / 2.0 ) - 20.0;
	Serial << F( "[" ) << millis() << F( "]      Temperatur 01: ") << temp01F << endl;	

	String temp02S = data.substring( 30 , 32 );
	uint32_t temp02I = HexStringToInt( temp02S );
	float temp02F = (float)temp02I;
	temp02F = (temp02F / 2.0 ) - 20.0;
	Serial << F( "[" ) << millis() << F( "]      Temperatur 02: ") << temp02F << endl;

	String thermoAlarmsS = data.substring( 32 , 34 );
	uint32_t thermoAlarmsI = HexStringToInt( thermoAlarmsS );
	Serial << F( "[" ) << millis() << F( "]      Anz. Thermoalarme: ") << thermoAlarmsI << endl;

	String testAlarmsS = data.substring( 34 , 36 );
	uint32_t testAlarmsI = HexStringToInt( testAlarmsS );
	Serial << F( "[" ) << millis() << F( "]      Anz. Testalarme: ") << testAlarmsI << endl;

	String wireAlarmsS = data.substring( 36 , 38 );
	uint32_t wireAlarmsI = HexStringToInt( wireAlarmsS );
	Serial << F( "[" ) << millis() << F( "]      Anz. Alarme (Bus): ") << wireAlarmsI << endl;

	String radioAlarmsS = data.substring( 38 , 40 );
	uint32_t radioAlarmsI = HexStringToInt( radioAlarmsS );
	Serial << F( "[" ) << millis() << F( "]      Anz. Alarme (Funk): ") << radioAlarmsI << endl;

	// Generate HTTP Data
	String httpData = "json={\"identifier\":\"";
	httpData += MacAddressToString( WiFiMacAddress ) + ":" + addressS + "\",";
	httpData += "\"data\":[";

	httpData += "{\"name\":\"Uptime\",\"value\":\"";
	httpData += uptimeI;
	httpData += "\"},";

	httpData += "{\"name\":\"SmokeboxValue\",\"value\":\"";
	httpData += smokeChamberF;
	httpData += "\"},";

	httpData += "{\"name\":\"SmokeAlarmsNumber\",\"value\":\"";
	httpData += smokeAlarmsI;
	httpData += "\"},";

	httpData += "{\"name\":\"ContaminationLevel\",\"value\":\"";
	httpData += pollutionI;
	httpData += "\"},";

	httpData += "{\"name\":\"Battery\",\"value\":\"";
	httpData += batteryF;
	httpData += "\"},";

	httpData += "{\"name\":\"Temperature01\",\"value\":\"";
	httpData += temp01F;
	httpData += "\"},";

	httpData += "{\"name\":\"Temperature02\",\"value\":\"";
	httpData += temp02F;
	httpData += "\"},";

	httpData += "{\"name\":\"ThermoAlarmsNumber\",\"value\":\"";
	httpData += thermoAlarmsI;
	httpData += "\"},";

	httpData += "{\"name\":\"TestAlarmsNumber\",\"value\":\"";
	httpData += testAlarmsI;
	httpData += "\"}";

	httpData += "]}";

	//Datum und Uhrzeit vom Server holen
	uint8_t httpState;
	String httpContent;
	HttpPostRequest( "www.pro-kitchen.net" , "/demo/data/jsonData.do" , httpData , httpState , httpContent );
}

// Get the numeric value for a HEX string
uint32_t HexStringToInt( String hexString ) {
	if( hexString.length() == 8 ) {
		return( 
			GetValueForHex( hexString.charAt( 7 ) ) + 
			( GetValueForHex( hexString.charAt( 6 ) ) << 4 ) +
			( GetValueForHex( hexString.charAt( 5 ) ) << 8 ) + 
			( GetValueForHex( hexString.charAt( 4 ) ) << 12 ) +
			( GetValueForHex( hexString.charAt( 3 ) ) << 16 ) +
			( GetValueForHex( hexString.charAt( 2 ) ) << 20 ) + 
			( GetValueForHex( hexString.charAt( 1 ) ) << 24 ) +
			( GetValueForHex( hexString.charAt( 0 ) ) << 28 )
		);
	} else if( hexString.length() == 4 ) {
		return( 
			GetValueForHex( hexString.charAt( 3 ) ) + 
			( GetValueForHex( hexString.charAt( 2 ) ) << 4 ) +
			( GetValueForHex( hexString.charAt( 1 ) ) << 8 ) + 
			( GetValueForHex( hexString.charAt( 0 ) ) << 12 )
		);
	} else if( hexString.length() == 2 ) {
		return( 
			GetValueForHex( hexString.charAt( 1 ) ) + 
			( GetValueForHex( hexString.charAt( 0 ) ) << 4 )
		);
	}
}

// Get the numeric value for a HEX character
byte GetValueForHex( char c ) {
	if(c >= '0' && c <= '9') {
		return (byte)(c - '0');
	} else {
		return (byte)(c-'A'+10);
	}
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

	Serial << F( "[" ) << millis() << F( "] ") << F( "WiFi Modul - Alte Profile loeschen..." );

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

	Serial << F( "[" ) << millis() << F( "] " ) << F( "Datei 'config.ini' oeffnen..." );

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

		wifiClient.print( request );

		Serial << F( "[" ) << millis() << F( "] ") << F( "Gesendeter HTTP GET Request..." ) << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     GET ") << Page << F(" HTTP/1.1") << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     Host: ") << Server << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     Connection: close") << endl;

		HttpReadResponse( State , Content , WIFI_IDLE_TIMEOUT );
		
		Serial << F( "[" ) << millis() << F( "] ") << F( "Verbindung zum Server schliessen..." );
		wifiClient.close();
		Serial << F( "OK" ) << endl;
	} else {
		Serial << F( "FEHLER (VERBINDUNGSAUFBAU" );
		return;
	}
}

// HTTP POST Request durchführen
void HttpPostRequest( char *Server , char *Page , String Data , uint8_t &State , String &Content ){

	// GET /index.html HTTP/1.1
	String request = "POST ";
	request += Page;
	request += " HTTP/1.1\r\n";
	// Host: pro-kitchen.net

	request += "Host: ";
	request += Server;
	request += "\r\n";

	// Connection: close
	request += "Connection: close\r\n";
	// Content Type
	request += "Content-type: application/x-www-form-urlencoded\r\n";

	// Content Length
	request += "Content-length: ";
 	request += Data.length();
	request += "\r\n\r\n";

	// Content
	//request += Data;

	// Abschließen
	//request += "\r\n\r\n";

	Serial << F( "[" ) << millis() << F( "] ") << F( "HTTP POST Request an der Server '" ) << Server << F( "' senden" ) << endl;
	
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

		wifiClient.print( request );
		wifiClient.print( Data );
		wifiClient.print( "\r\n\r\n" );

		Serial << F( "[" ) << millis() << F( "] ") << F( "Gesendeter HTTP POST Request..." ) << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     POST ") << Page << F(" HTTP/1.1") << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     Host: ") << Server << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     Connection: close") << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     Content-type: application/x-www-form-urlencoded") << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     Content-length: ") << Data.length() << endl;
		Serial << F( "[" ) << millis() << F( "] ") << F( "     " ) << Data << endl;

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
	Serial << F( "[" ) << millis() << F( "] ") << F( "Watchdog zuruecksetzen..." );
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
