#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <Keypad.h>

// Define I2C Address - change if required
const int i2c_addr = 0x26;
LiquidCrystal_I2C lcd(0x26, 20, 4);

//keeps data for the keypad rows and cols
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

// Define a struct to represent a menu item
struct MenuItem {
  const char* name;
  int* intValue; // Pointer to an integer value (if applicable)
  bool* boolValue; // Pointer to a boolean value (if applicable)
  void (*action)(); // Pointer to the associated action function
};

//Setting values
int gamemode = 0; // This creates a modifiable character array. //TODO
int timeToPlant = 10; // The amount of time you have to plant the bomb (10mins)
int timeToDefuse = 10; // The amount of time you have to defuse the bomb (10mins)
int codeLength = 4; // Length of the code (4)
int mistakeTime = 0; // Removes time from counter as a punchment for doing it wrong (0sec)
int delayForNumbers = 2; // The amount of delay for the next number to be showed in code gamemodes (2sec)
int buzzerVolume = 255; // Volume of the buzzer (255)
bool autoCheck = true; // Auto checks if the code is right in the end so you dont need to press # to check it manually (True)
bool lockedMenu = false; // Locked Menu locks the menu ingame so player wont press it by mistake to unlock press * and # at the same time (False)
bool backLight = true; // Backlight for the LCD Display idk why you would turn it off but here you go (True)
bool lights = true; // Truns on or off a LED light above the keypad (True)

//data tracking
String pad; // Stores the data of the keypad
String bombcode; // The code that the bomb gen
String displaycode; // The number that will be displayed in the connor in code gamemodes
char keypressed; // The lastest key that have been pressed
bool isLocked = false; // the toggle for the menu being locked or not locked while playing
unsigned long currentMillis; // The current time from the boot up on the bomb
int plantTime = timeToPlant*60; // The amount of time you have to plant the bomb in sec (timeToPlant*60)
int defuseTime = timeToDefuse*60; // The amount of time you have to defuse the bomb in sec (timeToDefuse*60)
unsigned long previousMillis = 0; // time thingy
bool loadingAnimationForCode = false; // Loading animation for the code in code gamemodes
bool isPaused = true; // Stops everything related to games

int gameState = 0;

//Menu stages
bool editMode = false; // Flag to indicate if you're in edit mode
bool scrollingEnabled = true; // Flag to indicate if scrolling is enabled
int editingSetting = 0; // Value to keep track of what setting is being changed

//Yes make code not break and look pretty
void loadingAnimation(int x, int y);
void displayText(int x, int y, const char* text, int duration);

// Define function for menu item actions
void startAction();
void resetGameAction();
void infoAction();
void settingsAction();
void presetsAction();

void backAction();
void changeGamemode();
void changingValueAction(const char* text, int action, int min, int max, const char* units, int id);
void saveAsPresetAction();
void autoCheckAction();
void lockedMenuAction();
void lightsAction();
void backLightAction();
void useLastProfileAction();
void supportAction();
void saveSettings();
void factoryResetAction();
void gameover(const char* top, const char* mid, int time, const char* bot);
void userPreset();
void setTimeCodes(int gamemode, int plantTime, int defuseTime, int codeLength, int delayForNumbers, int mistake);
void DebugLog();

// Define the main menu and settings menu
MenuItem mainMenu[] = {
  {"Start", nullptr, nullptr, startAction},
  {"Info", nullptr, nullptr, infoAction},
  {"Settings", nullptr, nullptr, settingsAction},
  {"Presets", nullptr, nullptr, presetsAction},
  {"Support", nullptr, nullptr, supportAction}
};

MenuItem settingMenu[] = {
  {"Back/Save", nullptr, nullptr, backAction},
  {"Gamemodes", &gamemode, nullptr, []() { changingValueAction("Gamemodes", gamemode, 0, 3, "", 7); }},
  {"Time to plant", &timeToPlant, nullptr, []() { changingValueAction("Time to plant", timeToPlant, 0, 60, " Mins", 1); }},
  {"Time to defuse", &timeToDefuse, nullptr, []() { changingValueAction("Time to defuse", timeToDefuse, 0, 60, " Mins", 2); }},
  {"Code length", &codeLength, nullptr, []() { changingValueAction("Code Length", codeLength, 0, 20, "", 3); }},
  {"Buzzer Volume", &buzzerVolume, nullptr, []() { changingValueAction("Buzzer Volume", buzzerVolume, 0, 255, "", 4); }},
  {"Mistake", &mistakeTime, nullptr, []() { changingValueAction("Mistake punishment", mistakeTime, 0, 60, " Sec", 5); }},
  {"Delay for numbers", &delayForNumbers, nullptr, []() { changingValueAction("Delay for numbers", delayForNumbers, 0, 10, " Sec", 6); }},
  {"Auto check", nullptr, &autoCheck, autoCheckAction},
  {"Locked menu", nullptr, &lockedMenu, lockedMenuAction},
  {"Lights", nullptr, &lights, lightsAction},
  {"Back light", nullptr, &backLight, backLightAction},
  {"Support", nullptr, nullptr, supportAction}, // No value for this item
  {"Save as default", nullptr, nullptr, saveSettings},
  {"Factory Reset", nullptr, nullptr, factoryResetAction},
};

MenuItem presetMenu[] = {
  {"Back", nullptr, nullptr, backAction},
  {"User Preset", nullptr, nullptr, userPreset},
  {"Speedy Code 10/5", nullptr, nullptr, []() { setTimeCodes(0, 10, 5, 4, 3, 5); }},
  {"Code/15/10", nullptr, nullptr, []() { setTimeCodes(0, 15, 10, 6, 4, 10); }},
  {"Code 10/10", nullptr, nullptr, []() { setTimeCodes(0, 10, 10, 6, 3, 8); }},
  {"Code/20/10", nullptr, nullptr, []() { setTimeCodes(0, 20, 10, 8, 3, 15); }},
};

int selectedMenuItem = 0;
int menuStartIndex = 0;
const MenuItem* currentMenu = mainMenu;
int maxVisibleItems = 4;
int menuInUse=0;
int menuMaxLength = sizeof(mainMenu) / sizeof(mainMenu[0])-1;

const int selectButton = 21;
const int upButton = 20;
const int downButton = 19;
const int whiteLed = 22;
const int ledPin = 4;
const int buzzerPin = 5;

//Bomb timer

void displayTimer(int minutes, int seconds) {
  lcd.setCursor(13, 1);
  lcd.print("Timer");
  lcd.setCursor(13, 2);

  // Display minutes
  if (minutes < 10)
    lcd.print("0");
  lcd.print(minutes);
  lcd.print(":");

  // Display seconds
  if (seconds < 10)
    lcd.print("0");
  lcd.print(seconds);
}

void BombTime(int defuseTime) {
  int minutes = defuseTime / 60;
  int seconds = defuseTime % 60;
  displayTimer(minutes, seconds);
}

void PlantTime(int plantTime) {
  int minutes = plantTime / 60;
  int seconds = plantTime % 60;
  displayTimer(minutes, seconds);
}

//Code gamemode

void getBombCode() {
  bombcode = "";
  for (int i = 0; i < codeLength; i++) {
      bombcode += String(random(0, 9));
  }
}

void displayCode() {
  static unsigned long displayDelayStart = 0;

  // Check if it's time to update the display code
  if (millis() - displayDelayStart >= (delayForNumbers + 1) * 1000) {
    displaycode = bombcode[pad.length()]; // Accessing the character at the specified index
    {
      loadingAnimationForCode = false;
      lcd.setCursor(0, 0);
      lcd.print("     ");
      lcd.setCursor(0, 0);
      lcd.print(displaycode);
    }
    // Reset the delay timer
    displayDelayStart = millis();
  }
}

void readKeypad() {
  keypressed = myKeypad.getKey(); // Detect keypad press

  if (gameState != 0) {
    if (keypressed != NO_KEY) {  // Check if a valid key is pressed
      String konv = String(keypressed);
      loadingAnimationForCode = true;

      digitalWrite(ledPin, HIGH);
      analogWrite(buzzerPin, buzzerVolume);
      delay(100);
      digitalWrite(ledPin, LOW);
      analogWrite(buzzerPin, 0);

      if (keypressed == '*') {
        // Remove the last character from the pad (if it's not empty)
        if (pad.length() > 0) {
          pad = pad.substring(0, pad.length() - 1);
          lcd.setCursor(0, 3);
          lcd.print("                ");  // Clear the line
          lcd.setCursor(0, 3);
          lcd.print(pad);
        }
      } else if (keypressed != '#') {
        // Check if the pad length is less than the code length
        if (static_cast<int>(pad.length()) < codeLength) {
          pad += konv;
          lcd.setCursor(0, 3);
          lcd.print(pad);
        }
      }
    }
  }
}

//Main gamemode functions

void blinking(int blinkInterval) {
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;
  static bool ledState = false;

  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    analogWrite(buzzerPin, ledState ? buzzerVolume : 0);
  }
}

int lerp(int start, int end, float t) {
  return start + t * (end - start);
}

void timer() {
  if (defuseTime <=0) {
    lcd.clear();
    gameover("Game over!", "Bomb blow up", defuseTime, "Attackers wins");
  }
  if (plantTime <=0) {
    lcd.clear();
    gameover("Game over!", "Time ran out", plantTime, "Defender wins");
  }

  //Checks if the bomb is planted and if the game is over
  if (gameState == 2 && defuseTime > 0) {
    unsigned long currentMillis = millis();
    if (defuseTime <= 60) {
    int blinkInterval = lerp(0, 1000, defuseTime / 60.0);
    blinking(blinkInterval);
    } else {
      blinking(1000);
    }

    if (currentMillis - previousMillis >= 1000) {
      previousMillis = currentMillis;
      defuseTime -= 1;
      BombTime(defuseTime);
    }
  } else {
    unsigned long currentMillis = millis();

    if (gameState == 1 && plantTime > 0) {
      if (currentMillis - previousMillis >= 1000) {
        previousMillis = currentMillis;
        plantTime -= 1;
        PlantTime(plantTime);
      }
    }
  }
}

void loadingAnimation(int x, int y) {
  static unsigned long previousMillis = 0;
  static int numberOfDots = 0;
  const int maxDots = 3;

  if (millis() - previousMillis >= 333) {
    lcd.setCursor(x, y);
    for (int i = 0; i < numberOfDots; i++) {
      lcd.print('.');
    }
    lcd.print("     "); // Clear any remaining dots

    numberOfDots = (numberOfDots + 1) % (maxDots + 1); // Cycle through 0 to maxDots
    previousMillis = millis(); // Reset the delay timer
  }
}

void displayText(int x, int y, const char* text, int duration) {
  unsigned long startTime = millis();
  lcd.setCursor(x, y);
  lcd.print(text);

  // Calculate the end time based on the duration
  unsigned long endTime = startTime + duration;

  while (millis() < endTime) {
    timer();
    readKeypad();  // Continue reading keypad during the delay
  }

  int textLength = strlen(text);
  lcd.setCursor(x, y);
  for (int i = 0; i < textLength; ++i) {
    lcd.print(" ");
  }
}

void gameover(const char* top,const char* mid,int time, const char* bot) {

  isLocked = false;
  loadingAnimationForCode = false;
  editMode = true;
  isPaused = true;
  scrollingEnabled = false;

  //Stop led and buzzer
  digitalWrite(ledPin, LOW);
  analogWrite(buzzerPin, 0);

  //LCD Screen
  lcd.setCursor((20 - strlen(top)) / 2, 0);
  lcd.print(top);
  lcd.setCursor((20 - strlen(mid)) / 2, 1);
  lcd.print(mid);
  int minutes = time / 60;
  int seconds = time % 60;
  lcd.setCursor(7, 2);
  if (minutes < 10) {
    lcd.print("0");
  }
  lcd.print(minutes);
  lcd.print(":");

  if (seconds < 10) {
    lcd.print("0");
  }
  lcd.print(seconds);
  lcd.print("s");
  lcd.setCursor((20 - strlen(bot)) / 2, 3);
  lcd.print(bot);
}

//Save and loading
void saveSettingsToEEPROM() {
  EEPROM.put(0 * sizeof(int), gamemode);
  EEPROM.put(1 * sizeof(int), timeToPlant);
  EEPROM.put(2 * sizeof(int), timeToDefuse);
  EEPROM.put(3 * sizeof(int), codeLength);
  EEPROM.put(4 * sizeof(int), mistakeTime);
  EEPROM.put(5 * sizeof(int), delayForNumbers);
  EEPROM.put(6 * sizeof(int), buzzerVolume);
  EEPROM.put(7 * sizeof(int), autoCheck);
  EEPROM.put(8 * sizeof(int), lockedMenu);
  EEPROM.put(9 * sizeof(int), backLight);
  EEPROM.put(10 * sizeof(int), lights);
}

void loadSettingsFromEEPROM() {
  EEPROM.get(0 * sizeof(int), gamemode);
  EEPROM.get(1 * sizeof(int), timeToPlant);
  EEPROM.get(2 * sizeof(int), timeToDefuse);
  EEPROM.get(3 * sizeof(int), codeLength);
  EEPROM.get(4 * sizeof(int), mistakeTime);
  EEPROM.get(5 * sizeof(int), delayForNumbers);
  EEPROM.get(6 * sizeof(int), buzzerVolume);
  EEPROM.get(7 * sizeof(int), autoCheck);
  EEPROM.get(8 * sizeof(int), lockedMenu);
  EEPROM.get(9 * sizeof(int), backLight);
  EEPROM.get(10 * sizeof(int), lights);
}

//Update menus

void updateMainMenu() {
  lcd.clear();
  int numItems = sizeof(mainMenu) / sizeof(mainMenu[0]);
  
  for (int i = menuStartIndex; i < min(menuStartIndex + maxVisibleItems, numItems); i++) {
    lcd.setCursor(0, i - menuStartIndex);
    if (i == selectedMenuItem) {
      lcd.print('>');
    } else {
      lcd.print(' ');
    }
    lcd.print(mainMenu[i].name);
  }
}

void updateSettingMenu() {
  lcd.clear();
  int numItems = sizeof(settingMenu) / sizeof(settingMenu[0]);
  
  for (int i = menuStartIndex; i < min(menuStartIndex + maxVisibleItems, numItems); i++) {
    lcd.setCursor(0, i - menuStartIndex);
    if (i == selectedMenuItem) {
      lcd.print('>');
    } else {
      lcd.print(' ');
    }
    lcd.print(settingMenu[i].name);

    // Display associated values (int or bool) if available
    if (settingMenu[i].intValue != nullptr) {
      lcd.setCursor(17, i - menuStartIndex);
      lcd.print(*settingMenu[i].intValue);
    } else if (settingMenu[i].boolValue != nullptr) {
      lcd.setCursor(17, i - menuStartIndex);
      lcd.print((*settingMenu[i].boolValue) ? "On" : "Off");
    }
  }
}

void updatePresetMenu() {
  lcd.clear();
  int numItems = sizeof(presetMenu) / sizeof(presetMenu[0]);
  
  for (int i = menuStartIndex; i < min(menuStartIndex + maxVisibleItems, numItems); i++) {
    lcd.setCursor(0, i - menuStartIndex);
    if (i == selectedMenuItem) {
      lcd.print('>');
    } else {
      lcd.print(' ');
    }
    lcd.print(presetMenu[i].name);
  }
}

void updateMenu() {
  switch (menuInUse) {
    case 0: {
      updateMainMenu();
      break;
    }
    case 1: {
      updateSettingMenu();
      break;
    }
    case 2: {
      updatePresetMenu();
      break;
    }
  }
}

//Setup
void setup() {
  //data things to start up
  Serial.begin(9600);
  loadSettingsFromEEPROM();
  // Gets random number from the noise of the analog port
  randomSeed(analogRead(0) + millis());;
  gameState = 0;

  //display
  lcd.init();
  if (backLight==true) {
    lcd.backlight();
  }
  lcd.clear();

  //pin modes
  pinMode(selectButton, INPUT_PULLUP);
  pinMode(upButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  pinMode(whiteLed, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  //lights
  if (lights==true) {
    digitalWrite(whiteLed, HIGH);
  }

  //startup animation
  lcd.setCursor(3, 1);
  lcd.print("Odense Airsoft");
  lcd.setCursor(4, 2);
  lcd.print("CSGO Bomb V1");
  delay(2000);
  gameState = 0;
  updateMenu();
}

void changeValueMenu(const char* name, int& value, int minVal, int maxVal, const char* valueInfo, int settingNumber) {
  scrollingEnabled = false;
  editMode = true;
  editingSetting = settingNumber;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(name);
  
  lcd.setCursor(0, 1);
  lcd.print("Value: ");
  lcd.print(value);
  lcd.print(valueInfo);
  
  lcd.setCursor(0, 3);
  lcd.print("Press knob to save");
}

void loop() {
  readKeypad(); // Reads keypad for inputs

  //Unlock locked menu
  if (keypressed == '*' || keypressed == '#') {
    isLocked = false;
  }

  //Loading animation for the code
  if (loadingAnimationForCode == true) {
    loadingAnimation(0,0);
  }

  //Checks if code is right
  if (isPaused == false) {
    if (gameState != 0) {
      lcd.setCursor(0, 3);
      lcd.print(pad);

      displayCode();
      timer();

      if ((autoCheck && keypressed != '#') || keypressed == '#') {
        if (pad.length() == bombcode.length()) {  // Compare only when the input length matches bomb code length
          if (pad == bombcode) {
            // Correct code
            if (gameState == 2) {
              gameState = 0;
              // Bomb defused
              pad="";
              mainMenu[0] = {"Start", nullptr, nullptr, startAction};
              mainMenu[1] = {"Info", nullptr, nullptr, infoAction};
              mainMenu[2] = {"Setting", nullptr, nullptr, settingsAction};
              mainMenu[3] = {"Presets", nullptr, nullptr, presetsAction};
              mainMenu[4] = {"Support", nullptr, nullptr, supportAction};
              
              lcd.clear();
              gameover("Game over!", "Bomb defused", defuseTime, "Defenders wins");
            } else {
              // Bomb planted
              lcd.setCursor(0, 0);
              lcd.print("                    ");  // Clear the pad string
              lcd.setCursor(0, 3);
              pad = ("                    ");  // Clear the pad string
              lcd.print(pad);
              pad="";
              gameState = 2;
              displayText(0,1,"Bomb planted", 3000);
              gameState = 2;
            }
          } else {
            // Incorrect code
            if (gameState == 2) {
              defuseTime -= mistakeTime;
              defuseTime = max(defuseTime, 0);
            } else {
              plantTime -= mistakeTime;
              plantTime = max(plantTime, 0);
            }
            lcd.setCursor(0, 3);
            lcd.print("                    ");  // Clear the line
            pad = "";  // Clear the pad
            displayText(0,1, "Wrong Code", 3000);
          }
        }
      }
    }
  }

  //Menu button handleing
  if (isLocked == false) {
    if (digitalRead(upButton) == LOW) {
        if (selectedMenuItem > 0 && scrollingEnabled) {
          selectedMenuItem--;
          if (selectedMenuItem < menuStartIndex) {
            menuStartIndex--;
          }
          delay(100);
          updateMenu();
      } else if (editMode) {
        switch(editingSetting){
        case 1:
          {
            timeToPlant = min(timeToPlant + 1, 60); // Decrement and limit the value
            changeValueMenu("Time to plant", timeToPlant, 0, 60, " Mins", 1);
            break;
          };
        case 2:
        {
          timeToDefuse = min(timeToDefuse + 1, 60); // Decrement and limit the value
          changeValueMenu("Time to defuse", timeToDefuse, 0, 60, " Mins", 2);
          break;
        };
        case 3:
        {
          codeLength = min(codeLength + 1, 20); // Decrement and limit the value
          changeValueMenu("Code Length", codeLength, 0, 20, " ", 3);
          break;
        };
        case 4:
        {
          buzzerVolume = min(buzzerVolume + 1, 255); // Decrement and limit the value
          changeValueMenu("Buzzer Volume", buzzerVolume, 0, 255, " ", 4);
          analogWrite(buzzerPin, buzzerVolume);
          delay(100);
          analogWrite(buzzerPin, 0);
          break;
        };
        case 5:
        {
          mistakeTime = min(mistakeTime + 1, 30); // Decrement and limit the value
          changeValueMenu("Mistake punishment", mistakeTime, 0, 30, " Sec", 5);
          break;
        };
        case 6:
        {
          delayForNumbers = min(delayForNumbers + 1, 30); // Decrement and limit the value
          changeValueMenu("Delay for numbers", delayForNumbers, 0, 30, " Sec", 6);
          break;
        };
        case 7:
        {
          gamemode = min(gamemode + 1, 3); // Decrement and limit the value
          changeValueMenu("Gamemodes", gamemode, 0, 3, "", 7);
          break;
        };
        }
        delay(100);
      }
    }
  
    if (digitalRead(downButton) == LOW) {
      
      if (selectedMenuItem < menuMaxLength) {
        if (currentMenu[selectedMenuItem + 1].name != NULL && scrollingEnabled) {
          selectedMenuItem++;
          if (selectedMenuItem >= menuStartIndex + maxVisibleItems) {
            menuStartIndex++;
          }
          delay(100);
          updateMenu();
        } else if (editMode) {
          switch(editingSetting){
          case 1:
            {
              timeToPlant = max(timeToPlant - 1, 0); // Decrement and limit the value
              changeValueMenu("Time to plant", timeToPlant, 0, 60, " Mins", 1);
              break;
            };
          case 2:
          {
            timeToDefuse = max(timeToDefuse - 1, 0); // Decrement and limit the value
            changeValueMenu("Time to defuse", timeToDefuse, 0, 60, " Mins", 2);
            break;
          };
          case 3:
          {
            codeLength = max(codeLength - 1, 0); // Decrement and limit the value
            changeValueMenu("Code Length", codeLength, 0, 20, " ", 3);
            break;
          };
          case 4:
          {
            buzzerVolume = max(buzzerVolume - 1, 0); // Decrement and limit the value
            changeValueMenu("Buzzer Volume", buzzerVolume, 0, 255, " ", 4);
            analogWrite(buzzerPin, buzzerVolume);
            delay(100);
            analogWrite(buzzerPin, 0);
            break;
          };
          case 5:
          {
            mistakeTime = max(mistakeTime - 1, 0); // Decrement and limit the value
            changeValueMenu("Mistake punishment", mistakeTime, 0, 30, " Sec", 5);
            break;
          };
          case 6:
          {
            delayForNumbers = max(delayForNumbers - 1, 0); // Decrement and limit the value
            changeValueMenu("Delay for numbers", delayForNumbers, 0, 30, " Sec", 6);
            break;
          };
          case 7:
          {
            gamemode = max(gamemode - 1, 0); // Decrement and limit the value
            changeValueMenu("Gamemodes", gamemode, 0, 3, "", 7);
            break;
          };
          }
          delay(100);
        }
      }
    }
  
    if (digitalRead(selectButton) == LOW) {
      const MenuItem selectedItem = currentMenu[selectedMenuItem];
      updateMenu();

      
      if (editMode) {
        // Handle saving the edited value
        scrollingEnabled = true;
        editMode = false;
        isPaused = true;
        editingSetting= 0 ;
        loadingAnimationForCode = false;
        updateMenu();
      } else {
        selectedItem.action(); // Call the associated action function
      }
      
      digitalWrite(ledPin, HIGH);
      analogWrite(buzzerPin, buzzerVolume);
      delay(100);
      digitalWrite(ledPin, LOW);
      analogWrite(buzzerPin, 0);
      
      delay(200);
    }
  }
}

void startAction() {
  Serial.println("Start action");
  pad="";
  if (gameState == 0) {
    defuseTime = timeToDefuse*60;
    plantTime = timeToPlant*60;
    editMode = true;
    DebugLog();

    if (gamemode == 0) {
      getBombCode();
      
      lcd.clear();
      scrollingEnabled = false;
      gameState = 1;
      Serial.println("CS Code");

    }
  } else {
    Serial.println("Game is started");
    lcd.clear();
    editMode = true;
  }

  mainMenu[0] = {"Continue", nullptr, nullptr, startAction};
  mainMenu[1] = {"Restart", nullptr, nullptr, resetGameAction};
  mainMenu[2] = {"Info", nullptr, nullptr, infoAction};
  mainMenu[3] = {"Setting", nullptr, nullptr, settingsAction};
  mainMenu[4] = {"Presets", nullptr, nullptr, presetsAction};
  mainMenu[5] = {"Support", nullptr, nullptr, supportAction};

  isPaused = false;

  if (lockedMenu == true) {
    isLocked = true;
  }
}

void resetGameAction() {
  Serial.println("Reset game action");
  mainMenu[0] = {"Start", nullptr, nullptr, startAction};
  mainMenu[1] = {"Info", nullptr, nullptr, infoAction};
  mainMenu[2] = {"Setting", nullptr, nullptr, settingsAction};
  mainMenu[3] = {"Presets", nullptr, nullptr, presetsAction};
  mainMenu[4] = {"Support", nullptr, nullptr, supportAction};
  gameState = 0;
  defuseTime = timeToDefuse*60;
  plantTime = timeToPlant*60;
  updateMenu(); // Update the menu display
}

void infoAction() {
  Serial.println("Info action");
  scrollingEnabled= false ;
  editMode= true ;
  lcd.setCursor(0,0);
  lcd.print("Gamemode: ");
  switch (gamemode)
  {
  case 0: {
    lcd.print("CSGO Code");
    break;
  }
  case 1: {
    lcd.print("CSGO Hold");
    break;
  }
  case 2: {
    lcd.print("Defense Code");
    break;
  }
  case 3: {
    lcd.print("Defense Hold");
    break;
  }
  default:
    break;
  }
  lcd.setCursor(0,1);
  lcd.print("Time to plant: ");
  lcd.print(timeToPlant);
  lcd.setCursor(0,2);
  lcd.print("Time to Defuse: ");
  lcd.print(timeToDefuse);
  lcd.setCursor(0,3);
  lcd.print("Code Length: ");
  lcd.print(codeLength);
}

void settingsAction() {
  Serial.println("Settings action");
  currentMenu = settingMenu; // Change the menu to settingMenu
  menuInUse = 1;
  menuMaxLength = sizeof(settingMenu) / sizeof(settingMenu[0])-1;
  selectedMenuItem = 0; // Reset the selected menu item
  menuStartIndex = 0; // Reset the menu start index
  updateSettingMenu(); // Update the menu display
}

void presetsAction() {
  Serial.println("Presets action");
  currentMenu = presetMenu; // Change the menu to settingMenu
  menuInUse = 2;
  menuMaxLength = sizeof(presetMenu) / sizeof(presetMenu[0])-1;
  selectedMenuItem = 0; // Reset the selected menu item
  menuStartIndex = 0; // Reset the menu start index
  updatePresetMenu(); // Update the menu display
}

//Setting menu

void backAction() {
  Serial.println("Back action");
  currentMenu = mainMenu; // Change back to the main menu
  menuInUse = 0;
  menuMaxLength = sizeof(mainMenu) / sizeof(mainMenu[0])-1;
  selectedMenuItem = 0; // Reset the selected menu item
  menuStartIndex = 0; // Reset the menu start index
  updateMainMenu(); // Update the menu display
}

void saveSettings() {
  saveSettingsToEEPROM();
  backAction();
}

void changingValueAction(const char* text, int action, int min, int max, const char* units, int id) {
  Serial.println(text);
  changeValueMenu(text, action, min, max, units, id);
}

void autoCheckAction() {
  Serial.println("Auto check action");
  if (autoCheck == false) {
    autoCheck = true;
  }
  else {
    autoCheck = false;
  }
  updateMenu();
}

void lockedMenuAction() {
  Serial.println("Locked menu action");
  if (lockedMenu == false) {
    lockedMenu = true;
  }
  else {
    lockedMenu = false;
  }
  updateMenu();
}

void lightsAction() {
  Serial.println("Lights action");
  if (lights == false) {
    lights = true;
    digitalWrite(whiteLed, HIGH); // Turn on the backlight
  }
  else {
    lights = false;
    digitalWrite(whiteLed, LOW); // Turn off the backlight
  }
  updateMenu(); // Corrected function name
}

void backLightAction() {
  Serial.println("Back light action");
  if (backLight == false) {
    backLight = true;
    lcd.backlight(); // Turn on the backlight
  }
  else {
    backLight = false;
    lcd.noBacklight(); // Turn off the backlight
  }
  updateMenu(); // Corrected function name
}

void supportAction() {
  Serial.println("Support action");
  scrollingEnabled= false ;
  editMode= true ;
  lcd.setCursor(0,0);
  lcd.print("User manual for bombGithub.com/lasse005/                    CSBomb             ");
}

void factoryResetAction() {
  timeToPlant = 10; // Initialize with a default value
  timeToDefuse = 10; // Initialize with a default value
  codeLength = 8;
  buzzerVolume = 5;
  mistakeTime = 2;
  delayForNumbers = 2;
  lockedMenu = false;
  backLight = true;
  saveSettingsToEEPROM();
  backAction();
}

//Presets
void setTimeCodes(int gamemode, int plantTime, int defuseTime, int codeLength, int delayForNumbers, int mistake) {
    backAction();
    timeToDefuse = defuseTime;
    timeToPlant = plantTime;
}

void userPreset() {
  backAction();
  EEPROM.get(0 * sizeof(int), gamemode);
  EEPROM.get(1 * sizeof(int), timeToPlant);
  EEPROM.get(2 * sizeof(int), timeToDefuse);
  EEPROM.get(3 * sizeof(int), codeLength);
  EEPROM.get(4 * sizeof(int), mistakeTime);
  EEPROM.get(5 * sizeof(int), delayForNumbers);
}

//Debuging
void DebugLog() {
  Serial.print("Gamemode ");
  Serial.println(gamemode);
  Serial.print("IsPaused ");
  Serial.println(isPaused);
  Serial.print("Edit Mode ");
  Serial.println(editMode);
  Serial.print("GameState ");
  Serial.println(gameState);
}