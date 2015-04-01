#include "Panstamp.h"

#define PANSTAMP_TIMEOUT 5000

Panstamp::Panstamp(void) {
}

void Panstamp::init(){
	Serial2.begin( 57600 );
}

void Panstamp::clearBuffer(){
	while( Serial2.available() > 0 ) Serial2.read();
}

bool Panstamp::switchToCommandMode(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "+++" );

	while( (millis()-start) < PANSTAMP_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK-Command mode") != -1 ) return true;
		}
	}
	return false;
}

bool Panstamp::switchToDataMode(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "ATO\r" );

	while( (millis()-start) < PANSTAMP_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK-Data mode") != -1 ) return true;
		}
	}
	return false;
}

bool Panstamp::setAddress( String value ){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	String command = "ATDA=";
	command = command + value;
	command = command + "\r";

	Serial2.print( command );

	while( (millis()-start) < PANSTAMP_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK") != -1 ) return true;
		}
	}
	return false;
}

bool Panstamp::setSynchronizationWord( String value ){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";
	
	String command = "ATSW=";
	command = command + value;
	command = command + "\r";

	Serial2.print( command );

	while( (millis()-start) < PANSTAMP_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK") != -1 ) return true;
		}
	}
	return false;
}

bool Panstamp::attention(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "AT\r" );

	while( (millis()-start) < PANSTAMP_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK") != -1 ) return true;
		}
	}
	return false;
}

bool Panstamp::reset(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "ATZ\r" );

	while( (millis()-start) < PANSTAMP_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK") != -1 ) return true;
		}
	}
	return false;
}

String Panstamp::synchronizationWord(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "ATSW?\r" );
	while( (millis()-start) < PANSTAMP_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			char c = Serial2.read();
			if( 13 == (int)c && response.length() > 3 ) {
				response.replace( "\r" , "" );
				response.replace( "\n" , "" );
				return response;
			} else {
				response += c;
			}
		}
	}
	return "";
}

String Panstamp::firmwareVersion(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "ATFV?\r" );
	while( (millis()-start) < PANSTAMP_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			char c = Serial2.read();
			if( 13 == (int)c && response.length() > 3 ) {
				response.replace( "\r" , "" );
				response.replace( "\n" , "" );
				return response;
			} else {
				response += c;
			}
		}
	}
	return "";
}

String Panstamp::hardwareVersion(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "ATHV?\r" );
	while( (millis()-start) < PANSTAMP_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			char c = Serial2.read();
			if( 13 == (int)c && response.length() > 3 ) {
				response.replace( "\r" , "" );
				response.replace( "\n" , "" );
				return response;
			} else {
				response += c;
			}
		}
	}
	return "";
}

String Panstamp::address(){

	uint64_t start = millis();
	String response = "";

	this->clearBuffer();
	Serial2.print( "ATDA?\r" );

	while( (millis()-start) < PANSTAMP_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			char c = Serial2.read();
			if( 13 == (int)c && response.length() > 2 ) {
				response.replace( "\r" , "" );
				response.replace( "\n" , "" );
				return response;
			} else {
				response += c;
			}
		}
	}
	return "";
}

Panstamp panstamp;