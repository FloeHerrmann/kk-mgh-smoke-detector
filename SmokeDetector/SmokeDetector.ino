//#include "EEPROM.h"
#include "product.h"
#include "regtable.h"
#include "cc1101.h"
#include "panstamp.h"

#define LED        4       // Connect led to digital pin 4
#define RFCHANNEL        0       // Let's use channel 0
#define SYNCWORD1        0x64    // Synchronization word, high byte
#define SYNCWORD0        0x32    // Synchronization word, low byte
#define SOURCE_ADDR      5       // Sender address

#define WDT_4S 8
#define WDT_8S 9

#define STX "\x02"
#define ETX "\x03"
#define ACK "\x06"

#define RESPONSE_TIMEOUT 5000
#define SEND_CYCLE 300000

CC1101 cc1101;

long startTime = 999999;
bool sendData = false;
bool alarmActive = false;

String receivedInfo = "";

void setup() {

	pinMode( LED , OUTPUT );
	digitalWrite( LED , LOW );

	panstamp.init();

	cc1101.init();
	cc1101.setChannel( RFCHANNEL , false );
	cc1101.setSyncWord( SYNCWORD1 , SYNCWORD0 , false );
	cc1101.setDevAddress(SOURCE_ADDR, false);

	Serial.begin( 9600 );
	while( !Serial );

	sendData = true;
}

void loop() {

	//delay( 1100 );
	if( Serial.available() > 0 ) {
		bool readResponse = true;
		long readTime = millis();
		while( readResponse ) {
			if( Serial.available() > 0 ) {
				byte currentChar = Serial.read();
				if( currentChar == 3 ) {
					Serial.print( ACK );
					delay( 100 );
					Serial.print( ACK );
					Serial.flush();
					readResponse = false;
					break;
				}
				if( currentChar != 0 && currentChar != 2 && currentChar != 3 && currentChar != 255 ) receivedInfo += (char)currentChar;
			}
			if( (millis()-readTime) > RESPONSE_TIMEOUT ){
				readResponse = false;
				receivedInfo = "TIMEOUT";
				break;
			}
		}
		ParseReceivedInfo();
	}

	if( (millis() - startTime) >= SEND_CYCLE ) {
		sendData = true;
	}

	if( sendData == true ) {
		
		String response = "";

		CCPACKET packet;
		packet.length = 17;
		packet.data[0] = SOURCE_ADDR;

		GetData( "0969" , response );
		packet.data[1] = HexStringToInt( response.substring( 0 , 2 ) );
		packet.data[2] = HexStringToInt( response.substring( 2 , 4 ) );
		packet.data[3] = HexStringToInt( response.substring( 4 , 6 ) );
		packet.data[4] = HexStringToInt( response.substring( 6 , 8 ) );
		delay( 1000 );

		GetData( "0B72" , response );
		packet.data[5] = HexStringToInt( response.substring( 0 , 2 ) );
		packet.data[6] = HexStringToInt( response.substring( 2 , 4 ) );
		packet.data[7] = HexStringToInt( response.substring( 4 , 6 ) );
		packet.data[8] = HexStringToInt( response.substring( 6 , 8 ) );
		delay( 1000 );
	  
		GetData( "0C73" , response );
		packet.data[9] = HexStringToInt( response.substring( 0 , 2 ) );
		packet.data[10] = HexStringToInt( response.substring( 2 , 4 ) );
		packet.data[11] = HexStringToInt( response.substring( 4 , 6 ) );
		packet.data[12] = HexStringToInt( response.substring( 6 , 8 ) );
		delay( 1000 );
		
		GetData( "0D74" , response );
		packet.data[13] = HexStringToInt( response.substring( 0 , 2 ) );
		packet.data[14] = HexStringToInt( response.substring( 2 , 4 ) );
		packet.data[15] = HexStringToInt( response.substring( 4 , 6 ) );
		packet.data[16] = HexStringToInt( response.substring( 6 , 8 ) );
		delay( 1000 );

		digitalWrite(LED, HIGH);
		cc1101.sendData( packet );
		digitalWrite(LED, LOW);

		sendData = false;
		startTime = millis();
	}
	//panstamp.sleepWell( 2 );
}

void ParseReceivedInfo( ) {

	int mode = -1;

	if( receivedInfo.equals( "8230000000FD" ) && alarmActive == false ) {
		alarmActive = true;
		mode = 7;
	} else if( receivedInfo.equals( "8220200000EE") && alarmActive == false ) {
		alarmActive = true;
		mode = 5;
	} else if( receivedInfo.equals( "8200200000EC") && alarmActive == false ) {
		alarmActive = true;
		mode = 6;
	} else if( receivedInfo.equals( "8228200000F6") && alarmActive == false ) {
		alarmActive = true;
		mode = 4;
	} else if( receivedInfo.equals( "8220040000F0") && alarmActive == false ) {
		alarmActive = true;
		mode = 8;
	} else if( receivedInfo.equals( "8222000004F2") ) {
		mode = 3;
	} else if( receivedInfo.equals( "8222000010EF") ) {
		mode = 2;
	} else if( receivedInfo.equals( "8220010000ED") ) {
		mode = 1;
	} else if( receivedInfo.equals( "8228010000F5") ) {
		mode = 1;
	} else if( receivedInfo.equals( "8220000000EC") && alarmActive == true ) {
		alarmActive = false;
		mode = 0;
	}

	if( mode >= 0 ) {
		CCPACKET packet;
		packet.length = 6;
		packet.data[0] = SOURCE_ADDR;
		packet.data[1] = mode;
		packet.data[2] = mode;
		packet.data[3] = mode;
		packet.data[4] = mode;
		packet.data[5] = mode;
		digitalWrite(LED, HIGH);
		cc1101.sendData( packet );
		digitalWrite(LED, LOW);
	}

	receivedInfo = "";
}

void GetData( String command , String &receivedData ) {
	Serial.print( STX );
	Serial.print( command );
	Serial.print( ETX );
	Serial.flush();
	
	delay( 500 );
	
	receivedData = "";
	bool readResponse = true;
	long startTime = millis();
	while( readResponse ) {
		
		if( Serial.available() > 0 ) {
			byte currentChar = Serial.read();
			if( currentChar == 3 ) {
				Serial.print( ACK );
				delay( 100 );
				Serial.print( ACK );
				Serial.flush();
				readResponse = false;
				break;
			}
			if( currentChar != 6 && currentChar != 0 && currentChar != 2 && currentChar != 3 ) receivedData += (char)currentChar;
		}
		
		if( (millis()-startTime) > RESPONSE_TIMEOUT ){
			readResponse = false;
			receivedData = "";
			return;
		}
	}

	receivedData = receivedData.substring( 2 , receivedData.length() -2 );
}

int HexStringToInt( String hexString ) {
	return GetValueForHex( hexString.charAt( 1 ) ) + ( GetValueForHex( hexString.charAt( 0 ) ) << 4 );
}

byte GetValueForHex( char c ) {
	if( c >= '0' && c <= '9' ) {
		return (byte)( c - '0' );
	} else {
		return (byte)( c - 'A' + 10 );
	}
}
