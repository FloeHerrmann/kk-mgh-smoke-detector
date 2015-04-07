#include "PanstampHelper.h"

#define PANSTAMP_HELPER_TIMEOUT 5000

PanstampHelper::PanstampHelper(void) {
}

void PanstampHelper::init(){
	Serial2.begin( 57600 );
}

void PanstampHelper::clearBuffer(){
	while( Serial2.available() > 0 ) Serial2.read();
}

bool PanstampHelper::switchToCommandMode(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "+++" );

	while( (millis()-start) < PANSTAMP_HELPER_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK-Command mode") != -1 ) return true;
		}
	}
	return false;
}

bool PanstampHelper::switchToDataMode(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "ATO\r" );

	while( (millis()-start) < PANSTAMP_HELPER_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK-Data mode") != -1 ) return true;
		}
	}
	return false;
}

bool PanstampHelper::setAddress( String value ){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	String command = "ATDA=";
	command = command + value;
	command = command + "\r";

	Serial2.print( command );

	while( (millis()-start) < PANSTAMP_HELPER_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK") != -1 ) return true;
		}
	}
	return false;
}

bool PanstampHelper::setSynchronizationWord( String value ){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";
	
	String command = "ATSW=";
	command = command + value;
	command = command + "\r";

	Serial2.print( command );

	while( (millis()-start) < PANSTAMP_HELPER_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK") != -1 ) return true;
		}
	}
	return false;
}

bool PanstampHelper::attention(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "AT\r" );

	while( (millis()-start) < PANSTAMP_HELPER_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK") != -1 ) return true;
		}
	}
	return false;
}

bool PanstampHelper::reset(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "ATZ\r" );

	while( (millis()-start) < PANSTAMP_HELPER_TIMEOUT ) {
		if( Serial2.available() > 0 ) {
			response += (char)Serial2.read();
			if( response.indexOf( "OK") != -1 ) return true;
		}
	}
	return false;
}

String PanstampHelper::synchronizationWord(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "ATSW?\r" );
	while( (millis()-start) < PANSTAMP_HELPER_TIMEOUT ) {
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

String PanstampHelper::firmwareVersion(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "ATFV?\r" );
	while( (millis()-start) < PANSTAMP_HELPER_TIMEOUT ) {
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

String PanstampHelper::hardwareVersion(){
	this->clearBuffer();

	uint64_t start = millis();
	String response = "";

	Serial2.print( "ATHV?\r" );
	while( (millis()-start) < PANSTAMP_HELPER_TIMEOUT ) {
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

String PanstampHelper::address(){

	uint64_t start = millis();
	String response = "";

	this->clearBuffer();
	Serial2.print( "ATDA?\r" );

	while( (millis()-start) < PANSTAMP_HELPER_TIMEOUT ) {
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

PanstampHelper panstampHelper;