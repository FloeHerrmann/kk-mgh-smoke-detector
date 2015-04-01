#ifndef PANSTAMP_H

	#define PANSTAMP_H

	#include <Arduino.h>

	class Panstamp {
		public:
			Panstamp( void );
			void init();
			void clearBuffer();
			bool switchToCommandMode();
			bool switchToDataMode();
			bool setAddress( String value );
			bool setSynchronizationWord( String value );
			bool attention();
			bool reset();
			String synchronizationWord();
			String firmwareVersion();
			String hardwareVersion();
			String address();
		private:
	};

	extern Panstamp panstamp;

#endif
