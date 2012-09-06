/*
Tank Controller - Created by Ben Rigas (ben@rigas.net)
 
 1.0:
 Controls up to 3 MeanWell ELN-48P LED Dimmable Drivers
 Automatically controls brightness of Royal Blue and Cool White LEDs based on time of day
 * Night time
 * Sunrise
 * Noon
 * Sunset
 * Or, custom brightness settings
 
 Future Stuff:
 * Moonlight on/off
 * Pumps on/off
 * Auto-top-off of RO/DI water control
 * Ethernet or Bluetooth communication (iOS and/or web app)
 */

#include <Wire.h>
#include <RTClib.h>

RTC_DS1307 RTC;

#define COOL_WHITE_PIN 11
#define ROYAL_BLUE_PIN 10
#define ROYAL_BLUE_PIN2 9

// TODO implement the gradual increase
#define GRADUAL_STEP_INTERVAL_BRIGHTNESS 0.05
#define GRADUAL_STEP_DURATION_MIN 30

#define BUTTON_PIN 2
//#define BUTTON_LED_PIN 13

float tb = 0.3;

int ledState = HIGH;
int buttonState;
int lastButtonState = LOW;

long lastDebounceTime = 0;
long debounceDelay = 50;

bool justChangedState = false;

// TODO
// refactor this into 'target' brightness for blue and white?
// less code, each 'mode' just sets properties and simple FSM
// makes the brightness adjustments until 'target' is reached

typedef enum LightingState
{ 
	LightingStateOff, 
	LightingStateSunrise, 
	LightingStateNoon, 
	LightingStateSunset, 
	LightingStateNight, 
	LightingStateCustom 
} LightingState_t;

bool SetChannelBrightness(int channel, float brightness);
void DoLightsOff(bool gradually=true);
void DoSunrise(bool gradually=true);
void DoNoon(bool gradually=true);
void DoSunset(bool gradually=true);
void DoNight(bool gradually=true);
void SetCurrentLightingState();

float royalBlueBrightness = 0.0;
float coolWhiteBrightness = 0.0;

LightingState_t currentLightingState = LightingStateSunset;

bool SetChannelBrightness(int channel, float brightness)
{
	bool isValid = true;

	// max/min check
	if (brightness < 0.0 || brightness > 1.0) isValid = false;

	if (isValid)
	{
		// remember brightness values
		switch (channel)
		{
			case ROYAL_BLUE_PIN:
				royalBlueBrightness = brightness;
				analogWrite(channel, 255 * brightness);
                                analogWrite(ROYAL_BLUE_PIN2, 255 * brightness);
				break;
			case COOL_WHITE_PIN:
				coolWhiteBrightness = brightness;
				analogWrite(channel, 255 * brightness);
				break;
			default:
				isValid = false;
				break;
		}
	}

	return isValid;	
}

void DoLightsOff(bool gradually)
{
	if (!gradually)
	{
		SetChannelBrightness(ROYAL_BLUE_PIN, 0.0);
		SetChannelBrightness(COOL_WHITE_PIN, 0.0);
	}
}

void DoSunrise(bool gradually)
{
	if (!gradually)
	{
		SetChannelBrightness(ROYAL_BLUE_PIN, 0.44);
		SetChannelBrightness(COOL_WHITE_PIN, 0.00);
	}
}

void DoSunset(bool gradually)
{
	if (!gradually)
	{
		SetChannelBrightness(ROYAL_BLUE_PIN, 0.50);
		SetChannelBrightness(COOL_WHITE_PIN, 0.20);
	}
}

void DoNoon(bool gradually)
{
	if (!gradually)
	{
		SetChannelBrightness(ROYAL_BLUE_PIN, 0.6);
		SetChannelBrightness(COOL_WHITE_PIN, 0.3);
	}
}

void DoNight(bool gradually)
{
	//DoLightsOff(gradually);

	if (!gradually)
	{
		SetChannelBrightness(ROYAL_BLUE_PIN, 0.22);
		SetChannelBrightness(COOL_WHITE_PIN, 0.0);
	}

	// TODO add moonight controls here? Or leave manual?
}

void DoLightingStateCustom(bool gradually)
{
	if (!gradually)
	{
		SetChannelBrightness(ROYAL_BLUE_PIN, 0.6);
		SetChannelBrightness(COOL_WHITE_PIN, 0.3);
	}
}

void CheckButtons()
{
        

        //Serial.println(millis() - lastButtonStateChange);
	int reading = !digitalRead(BUTTON_PIN);

	// check to see if you just pressed the button 
	// (i.e. the input went from LOW to HIGH),  and you've waited 
	// long enough since the last press to ignore any noise:  

	// If the switch changed, due to noise or pressing:
	if (reading != lastButtonState) {
		// reset the debouncing timer
		lastDebounceTime = millis();
	} 

	if ((millis() - lastDebounceTime) > debounceDelay) {
		// whatever the reading is at, it's been there for longer
		// than the debounce delay, so take it as the actual current state:
		buttonState = reading;
	}

	// set the LED using the state of the button:
	//digitalWrite(ledPin, buttonState);	

	if (buttonState == HIGH && justChangedState == false)
	{
            //Serial.println("BUTTON STATE HIGH");
            //Serial.println(lastButtonState);
		if (currentLightingState != LightingStateCustom)
		{
                        // we want to override the normal setting until the button is
			// pressed again
			currentLightingState = LightingStateCustom;
		}
		else
		{
			// reset lighting state to let normal setting take effect
			currentLightingState = LightingStateOff;
		}
              justChangedState = true;
	}
        else if (buttonState == LOW)
        {
          //Serial.println("LOW");
          justChangedState = false;
        }

        
	// save the reading.  Next time through the loop,
	// it'll be the lastButtonState:
	lastButtonState = reading;
        
}

void SetCurrentLightingState()
{
	LightingState_t state = currentLightingState;
	
	if (currentLightingState != LightingStateCustom)
	{
		DateTime now = RTC.now();
//		
                float blueBright = 0.0;
                float whiteBright = 0.0;
                
                if (now.hour() == 9 || now.hour() == 10) blueBright = 0.22;
                if (now.hour() == 11 || now.hour() == 12) blueBright = 0.3;
                if (now.hour() >= 13 && now.hour() <= 16) blueBright = 0.4;
                if (now.hour() == 17 || now.hour() == 18) blueBright = 0.3;
                if (now.hour() >= 19 && now.hour() < 22) blueBright = 0.22;
                
	        if (now.hour() == 11)
                {
                    whiteBright = (0.2/59) * now.minute();
                }
                if (now.hour() == 12)
                {
                    whiteBright = 0.2 + (0.2/59) * now.minute();
                }
                if (now.hour() == 13)
                {
                    whiteBright = 0.4 + (0.2/59) * now.minute();
                }
                
                if (now.hour() >= 14 && now.hour() <= 18) whiteBright = 0.6;
                
                if (now.hour() == 19)
                {
                    whiteBright = 0.6 - ((0.6/59) * now.minute());
                }
	Serial.println(blueBright, DEC);

		SetChannelBrightness(ROYAL_BLUE_PIN, blueBright);
		SetChannelBrightness(COOL_WHITE_PIN, whiteBright);
		
return;
		if (now.hour() >= 9 && now.hour() < 12)
		{
			state = LightingStateSunrise;
                        //Serial.println("sunrise");
		}
		else if (now.hour() >= 12 && now.hour() < 17)
		{
			state = LightingStateNoon;
		}
		else if (now.hour() >= 17 && now.hour() < 19)
		{
			state = LightingStateSunset;
		}
                else if (now.hour() >= 19 && now.hour() < 22)
                {
                        state = LightingStateNight;
                }
		else
		{
			state = LightingStateOff;
		}
		
	}
	
	currentLightingState = state;
}

void AdjustLighting()
{
	SetCurrentLightingState();
	return;
	switch (currentLightingState)
	{
		case LightingStateOff:
			DoLightsOff(false);
			break;
		case LightingStateSunrise:
			DoSunrise(false);
			break;
		case LightingStateNoon:
			DoNoon(false);
			break;
		case LightingStateSunset:
			DoSunset(false);
			break;
		case LightingStateNight:
			DoNight(false);
			break;
		case LightingStateCustom:
			DoLightingStateCustom(false);
			break;
		default:
			// do nothing
			break;
	}
}

void setup()
{ 
        Serial.begin(57600);
	// declare RoyalBlue and CoolWhite pins as outputs
	pinMode(ROYAL_BLUE_PIN, OUTPUT);
	pinMode(COOL_WHITE_PIN, OUTPUT);

	pinMode(A3, OUTPUT);
	digitalWrite(A3, HIGH);

	pinMode(A2, OUTPUT);
	digitalWrite(A2, LOW);

	pinMode(BUTTON_PIN, INPUT);
	digitalWrite(BUTTON_PIN, HIGH);

	//pinMode(BUTTON_LED_PIN, OUTPUT);

	Wire.begin();
	RTC.begin();

	// set the current time to the time that we compiled
	// NOTE compile and upload at the same time!
	RTC.adjust(DateTime(__DATE__, __TIME__));
} 

void loop()
{ 	
//        DateTime now = RTC.now();
//	Serial.println(now.hour(), DEC);
	CheckButtons();
//Serial.println(currentLightingState);
	AdjustLighting();

	// small delay in loop
	delay(150);                         
}

