#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define I2C Address - change if required
const int i2c_addr = 0x26;
LiquidCrystal_I2C lcd(0x26,20,4);

//Pins
const int led_pin = 4;
const int buzzer_pin = 5;
const int selectButton = 21;
const int upButton = 19;
const int downButton = 20;

//Values
int codeLength = 8;
int bombtime = 300; // set the time the defenders have to defuse the bomb
int planttime = 600; // set the time the attackers have to plant the bomb
int buzzerVolume = 255;  // Initial volume (0 to 255)

//data tracking
String pad;
String bombcode;
String displaycode;
char keypressed;
bool isTimerRunning = false;
bool isPlanted = false;
unsigned long previousMillis = 0; // Store the last time when the LED was updated
const long interval = 1000;       // Interval in milliseconds
unsigned long currentMillis = millis();
bool isBlinking = false;

const byte numRows = 4;
const byte numCols = 3;
char keymap[numRows][numCols] =
{
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
//------------------------------------------------------------
byte rowPins[numRows] = {7, 12, 11, 9};
byte colPins[numCols] = {8, 6, 10};

Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols); //mapping keypad

void BombTime(int bombtime) {
  int minutes = floor(bombtime/60);
  int seconds = bombtime%60;

  String minuteText = (minutes < 10) ? "0" + String(minutes) : String(minutes);
  String secondText = (seconds < 10) ? "0" + String(seconds) : String(seconds);
  String timerText = minuteText + ":" + secondText;

  lcd.setCursor(13, 1);
  lcd.print("Timer");
  lcd.setCursor(13, 2);
  lcd.print(timerText);
  bombtime -= 1;
}
void PlantTime(int planttime) {
  int minutes = floor(planttime/60);
  int seconds = planttime%60;

  String minuteText = (minutes < 10) ? "0" + String(minutes) : String(minutes);
  String secondText = (seconds < 10) ? "0" + String(seconds) : String(seconds);
  String timerText = minuteText + ":" + secondText;
  
  lcd.setCursor(13, 1);
  lcd.print("Timer");
  lcd.setCursor(13, 2);
  lcd.print(timerText);
  planttime -= 1;
}

void getBombCode() {
  bombcode = "";
  for (int i = 0; i < codeLength; i++) {
      bombcode += String(random(0, 9));
  }
}

void blink(int onTime, int offTime) {
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;
  static bool ledState = false;

  if (ledState) {
    if (currentMillis - previousMillis >= onTime) {
      previousMillis = currentMillis;
      ledState = false;
      digitalWrite(led_pin, LOW);
      analogWrite(buzzer_pin, 0);  // Turn off buzzer
    }
  } else {
    if (currentMillis - previousMillis >= offTime) {
      previousMillis = currentMillis;
      ledState = true;
      digitalWrite(led_pin, HIGH);
      analogWrite(buzzer_pin, buzzerVolume);  // Set buzzer volume
    }
  }
}

void blinking() {
  if (bombtime > 0) {
    // Call the blinking functions outside the if condition
    if (bombtime > 60) {
      isBlinking = true;
      blink(100,900);
    } else if (bombtime <= 60 && bombtime > 40) {
      isBlinking = true;
      blink(100,650);
    } else if (bombtime <= 40 && bombtime > 20) {
      isBlinking = true;
      blink(100,400);
    } else if (bombtime <= 20 && bombtime > 10) {
      isBlinking = true;
      blink(100,150);
    } else if (bombtime <= 10 && bombtime > 1) {
      isBlinking = true;
      blink(100,50);
    } else if (bombtime <= 1) {
      isBlinking = true;
      blink(100,25);
    } else {
      isBlinking = false;
      digitalWrite(led_pin, LOW);
    }
  }
}

void starttimer() {
  if (bombtime > 0) {
    unsigned long currentMillis = millis(); // Get the current time

    if (currentMillis - previousMillis >= interval) {
      // One second has elapsed, update the timer
      previousMillis = currentMillis;

      bombtime -= 1;            // Removes one from the timer
      BombTime(bombtime);
      blinking();
      
      Serial.println(bombtime); // Prints the time
    }

    // Call the blinking functions outside the if condition
    blinking();
  }
}

void displayCode() {
  static unsigned long displayDelayStart = 0;
  const unsigned long displayDelayDuration = 1000; // 1 sec in milliseconds

  if (millis() - displayDelayStart >= displayDelayDuration) {
    displaycode = bombcode[pad.length()]; // Accessing the character at the specified index

    lcd.setCursor(0, 0);
    lcd.print(displaycode);

    displayDelayStart = millis(); // Reset the delay timer
  }
}

void animatedLoading(int x, int y, int numDots, int delayTime) {
  lcd.setCursor(x, y);
  
  for (int i = 0; i < numDots; i++) {
    lcd.print(".");
    delay(delayTime);
  }
  
  lcd.setCursor(x, y);
  for (int i = 0; i < numDots; i++) {
    lcd.print(" ");
    delay(delayTime);
    lcd.print(".");
    delay(delayTime);
  }
}

void readKeypad() {
  keypressed = myKeypad.getKey(); // Detect keypad press
  
  if (keypressed != NO_KEY) {  // Check if a valid key is pressed
    String konv = String(keypressed);
    pad += konv;

    // Check the entered code along the way
    if (pad.length() > 0 && pad != bombcode.substring(0, pad.length())) {
      lcd.setCursor(0, 1);
      lcd.print("Wrong code  ");
      
      pad = "";  // Clear the pad string

      
    }
  }
}


void setup() {
  Serial.begin(9600);
  // Gets random number from the noise of the analog port
  randomSeed(analogRead(0) + millis());;
  //

  getBombCode();

  pinMode(led_pin, OUTPUT);
  pinMode(buzzer_pin, OUTPUT);
  analogWrite(buzzer_pin, buzzerVolume);  // Set initial volume
  pinMode(selectButton, INPUT_PULLUP);
  pinMode(upButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);

  lcd.backlight();
  lcd.init();

  // Print on first row
  
  lcd.setCursor(3, 1);
  lcd.print("Odense Airsoft");
  lcd.setCursor(8, 2);
  lcd.print("CSGO");
  delay(500);
  lcd.clear();
  isTimerRunning=true;
  starttimer();
}

void loop() {
  unsigned long currentMillis = millis();

  if (isTimerRunning == true) {
    starttimer();
    displayCode();
  } else {
    digitalWrite(led_pin, LOW);
    digitalWrite(buzzer_pin, LOW);
  }

  readKeypad();

  if (keypressed == '*') {
    pad = "";
    lcd.clear();
    getBombCode();
    isTimerRunning=true;
    bombtime=(60);
  }

  lcd.setCursor(0, 3);
  lcd.print(pad);

  if (pad.length() == bombcode.length()) {
    if (pad == bombcode) {
      lcd.setCursor(0, 1);
      lcd.print("Bomb defused");
      isTimerRunning=false;
    }
    delay(1000);
    pad = "";  // Clear the pad string
    Serial.println(bombcode);
  }
}