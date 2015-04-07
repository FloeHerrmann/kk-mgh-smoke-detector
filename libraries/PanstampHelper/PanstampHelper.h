#ifndef PANSTAMP_H

	#define PANSTAMP_HELPER_H

	#include <Arduino.h>

	class PanstampHelper {
		public:
			PanstampHelper( void );
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

	extern PanstampHelper panstampHelper;

#endif
