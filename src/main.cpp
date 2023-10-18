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
int timeToPlant = 10; // The amount of time you have to plant the bomb (10mins)
int timeToDefuse = 10; // The amount of time you have to defuse the bomb (10mins)
int codeLenght = 4; // Lenght of the code (4)
int gamemode = 0; // This creates a modifiable character array. //TODO
int buzzerPitch = 2; // Pitch/audio of the buzzer (5)
int mistakeTime = 0; // Removes time from counter as a punchment for doing it wrong (0sec)
int delayForNumbers = 2; // The amount of delay for the next number to be showed in code gamemodes (2sec)
bool autoCheck = true; // Auto checks if the code is right in the end so you dont need to press # to check it manually (True)
bool lockedMenu = false; // Locked Menu locks the menu ingame so player wont press it by mistake to unlock press * and # at the same time (False)
bool backLight = true; // Backlight for the LCD Display idk why you would turn it off but here you go (True)

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

// Define game states
enum GameState {
  GAME_NOT_STARTED,
  GAME_STARTED,
  BOMB_PLANTED,
  BOMB_DEFUSED
};

GameState gameState = GAME_NOT_STARTED;

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
void backLightAction();
void useLastProfileAction();
void supportAction();
void saveSettings();
void factoryResetAction();

void setTimeCodes(int timeToPlant, int timeToDefuse);

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
  {"Code length", &codeLenght, nullptr, []() { changingValueAction("Code lenght", codeLenght, 0, 20, "", 3); }},
  {"Buzzer Pitch", &buzzerPitch, nullptr, []() { changingValueAction("Buzzer pitch", buzzerPitch, 0, 255, "", 4); }},
  {"Mistake", &mistakeTime, nullptr, []() { changingValueAction("Mistake punishment", mistakeTime, 0, 60, " Sec", 5); }},
  {"Delay for numbers", &delayForNumbers, nullptr, []() { changingValueAction("Delay for numbers", delayForNumbers, 0, 10, "", 6); }},
  {"Auto check", nullptr, &autoCheck, autoCheckAction},
  {"Locked menu", nullptr, &lockedMenu, lockedMenuAction},
  {"Back light", nullptr, &backLight, backLightAction},
  {"Support", nullptr, nullptr, supportAction}, // No value for this item
  {"Save as default", nullptr, nullptr, saveSettings},
  {"Factory Reset", nullptr, nullptr, factoryResetAction},
};

MenuItem presetMenu[] = {
  {"Back", nullptr, nullptr, backAction},
  {"10/5/Code", nullptr, nullptr, []() { setTimeCodes(10, 5); }},
  {"15/10/Code", nullptr, nullptr, []() { setTimeCodes(15, 10); }},
  {"10/10/Code", nullptr, nullptr, []() { setTimeCodes(10, 10); }},
  {"20/10/Code", nullptr, nullptr, []() { setTimeCodes(20, 10); }},
};

int selectedMenuItem = 0;
int menuStartIndex = 0;
const MenuItem* currentMenu = mainMenu;
const int maxVisibleItems = 4;
int menuInUse=0;
int menuMaxLenght = sizeof(mainMenu) / sizeof(mainMenu[0])-1;

const int selectButton = 21;
const int upButton = 20;
const int downButton = 19;
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
  for (int i = 0; i < codeLenght; i++) {
      bombcode += String(random(0, 9));
  }
}

void displayCode() {
  static unsigned long displayDelayStart = 0;

  // Check if it's time to update the display code
  if (millis() - displayDelayStart >= delayForNumbers * 1000) {
    displaycode = bombcode[pad.length()]; // Accessing the character at the specified index

    {
      loadingAnimationForCode = false;
      lcd.setCursor(0, 0);
      lcd.print("   "); // Clear any remaining dots
      lcd.setCursor(0, 0);
      lcd.print(displaycode);
    }

    // Reset the delay timer
    displayDelayStart = millis();
  }
}

void readKeypad() {
  keypressed = myKeypad.getKey(); // Detect keypad press

  if (gameState != GAME_NOT_STARTED || BOMB_DEFUSED) {
    if (keypressed != NO_KEY) {  // Check if a valid key is pressed
      String konv = String(keypressed);
      loadingAnimationForCode = true;

      digitalWrite(ledPin, HIGH);
      analogWrite(buzzerPin, buzzerPitch);
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
        if (pad.length() < codeLenght) {
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
    analogWrite(buzzerPin, ledState ? buzzerPitch : 0);
  }
}

int lerp(int start, int end, float t) {
  return start + t * (end - start);
}

void timer() {
  if (gameState == BOMB_PLANTED && defuseTime > 0) {
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

    if (plantTime > 0) {
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

  if (millis() - previousMillis >= 500) {
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
    displayCode();
    readKeypad();  // Continue reading keypad during the delay
  }

  int textLength = strlen(text);
  lcd.setCursor(x, y);
  for (int i = 0; i < textLength; ++i) {
    lcd.print(" ");
  }
}

//Save and loading

void saveSettingsToEEPROM() {
  EEPROM.put(0, timeToPlant);
  EEPROM.put(sizeof(int), timeToDefuse);
  EEPROM.put(2 * sizeof(int), codeLenght);
  EEPROM.put(3 * sizeof(int), buzzerPitch);
  EEPROM.put(4 * sizeof(int), mistakeTime);
  EEPROM.put(5 * sizeof(int), delayForNumbers);
  EEPROM.get(6 * sizeof(int), autoCheck);
  EEPROM.put(7 * sizeof(int), lockedMenu);
  EEPROM.put(8 * sizeof(int), backLight);
}

void loadSettingsFromEEPROM() {
  EEPROM.get(0, timeToPlant);
  EEPROM.get(sizeof(int), timeToDefuse);
  EEPROM.get(2 * sizeof(int), codeLenght);
  EEPROM.get(3 * sizeof(int), buzzerPitch);
  EEPROM.get(4 * sizeof(int), mistakeTime);
  EEPROM.get(5 * sizeof(int), delayForNumbers);
  EEPROM.get(6 * sizeof(int), autoCheck);
  EEPROM.get(7 * sizeof(int), lockedMenu);
  EEPROM.get(8 * sizeof(int), backLight);
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
  GameState gameState = GAME_NOT_STARTED;

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
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  //startup animation
  lcd.setCursor(3, 1);
  lcd.print("Odense Airsoft");
  lcd.setCursor(4, 2);
  lcd.print("CSGO Bomb V1");
  delay(1000);
  gameState = GAME_NOT_STARTED;
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
  if (gameState != GAME_NOT_STARTED && !isPaused) {
    lcd.setCursor(0, 3);
    lcd.print(pad);

    timer();
    displayCode();

    if ((autoCheck && keypressed != '#') || keypressed == '#') {
      if (pad.length() == bombcode.length()) {  // Compare only when the input length matches bomb code length
        if (pad == bombcode) {
          // Correct code
          if (gameState == BOMB_PLANTED) {
            // Bomb defused
            pad="";
            mainMenu[0] = {"Start", nullptr, nullptr, startAction};
            mainMenu[1] = {"Info", nullptr, nullptr, infoAction};
            mainMenu[2] = {"Setting", nullptr, nullptr, settingsAction};
            mainMenu[3] = {"Presets", nullptr, nullptr, presetsAction};
            mainMenu[4] = {"Support", nullptr, nullptr, supportAction};

            loadingAnimationForCode = false;
            gameState = BOMB_DEFUSED;
            editMode = true;
            scrollingEnabled = false;

            //Stop led and buzzer
            digitalWrite(ledPin, LOW);
            analogWrite(buzzerPin, 0);

            //Display text
            lcd.clear();
            lcd.setCursor(5, 0);
            lcd.print("Game over!");
            lcd.setCursor(4, 1);
            lcd.print("Bomb defused");

            int minutes = defuseTime / 60;
            int seconds = defuseTime % 60;
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
          } else {
            // Bomb planted
            pad="";
            displayText(0,1,"Bomb planted", 5000);
            lcd.setCursor(0, 3);
            pad = ("                    ");  // Clear the pad string
            lcd.setCursor(0, 3);
            lcd.print(pad);
            gameState=BOMB_PLANTED;
          }
        } else {
          // Incorrect code
          if (gameState == BOMB_PLANTED) {
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

  //Menu button handleing
  if (isLocked == false) {
    if (scrollingEnabled != false) {
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
            codeLenght = min(codeLenght + 1, 20); // Decrement and limit the value
            changeValueMenu("Code lenght", codeLenght, 0, 20, " ", 3);
            break;
          };
          case 4:
          {
            buzzerPitch = min(buzzerPitch + 1, 255); // Decrement and limit the value
            changeValueMenu("Buzzer pitch", buzzerPitch, 0, 255, " ", 4);
            analogWrite(buzzerPin, buzzerPitch);
            delay(100);
            analogWrite(buzzerPin, 0);
            break;
          };
          case 5:
          {
            mistakeTime = min(mistakeTime + 1, 30); // Decrement and limit the value
            changeValueMenu("Mistake punishment", mistakeTime, 0, 30, " sec", 5);
            break;
          };
          case 6:
          {
            delayForNumbers = min(delayForNumbers + 1, 30); // Decrement and limit the value
            changeValueMenu("Delay for numbers", delayForNumbers, 0, 30, " sec", 6);
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
        
        if (selectedMenuItem < menuMaxLenght) {
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
              codeLenght = max(codeLenght - 1, 0); // Decrement and limit the value
              changeValueMenu("Code lenght", codeLenght, 0, 20, " ", 3);
              break;
            };
            case 4:
            {
              buzzerPitch = max(buzzerPitch - 1, 0); // Decrement and limit the value
              changeValueMenu("Buzzer pitch", buzzerPitch, 0, 255, " ", 4);
              analogWrite(buzzerPin, buzzerPitch);
              delay(100);
              analogWrite(buzzerPin, 0);
              break;
            };
            case 5:
            {
              mistakeTime = max(mistakeTime - 1, 0); // Decrement and limit the value
              changeValueMenu("Mistake punishment", mistakeTime, 0, 30, " sec", 5);
              break;
            };
            case 6:
            {
              delayForNumbers = max(delayForNumbers - 1, 0); // Decrement and limit the value
              changeValueMenu("Delay for numbers", delayForNumbers, 0, 30, " sec", 6);
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
    }
    
    if (digitalRead(selectButton) == LOW) {
      const MenuItem selectedItem = currentMenu[selectedMenuItem];
      updateMenu();

      
      if (editMode) {
        // Handle saving the edited value
        scrollingEnabled = true;
        editMode = false;
        isPaused = false;
        editingSetting= 0 ;
        updateMenu();
      } else {
        selectedItem.action(); // Call the associated action function
      }
      
      digitalWrite(ledPin, HIGH);
      analogWrite(buzzerPin, buzzerPitch);
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
  if (gameState == GAME_NOT_STARTED || isPaused) {
    defuseTime = timeToDefuse*60;
    plantTime = timeToPlant*60;
    gameState = GAME_STARTED;
    editMode = true;

    if (gamemode == 0) {
      getBombCode();
      
      lcd.clear();
      scrollingEnabled = false;
      gameState = GAME_STARTED;

    }
  } else {
    Serial.println("Game is started");
    isPaused = true;
  }

  mainMenu[0] = {"Continue", nullptr, nullptr, startAction};
  mainMenu[1] = {"Restart", nullptr, nullptr, resetGameAction};
  mainMenu[2] = {"Info", nullptr, nullptr, infoAction};
  mainMenu[3] = {"Setting", nullptr, nullptr, settingsAction};
  mainMenu[4] = {"Presets", nullptr, nullptr, presetsAction};
  mainMenu[5] = {"Support", nullptr, nullptr, supportAction};

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
  gameState = GAME_NOT_STARTED;
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
  lcd.print("Code lenght: ");
  lcd.print(codeLenght);
}

void settingsAction() {
  Serial.println("Settings action");
  currentMenu = settingMenu; // Change the menu to settingMenu
  menuInUse = 1;
  menuMaxLenght = sizeof(settingMenu) / sizeof(settingMenu[0])-1;
  selectedMenuItem = 0; // Reset the selected menu item
  menuStartIndex = 0; // Reset the menu start index
  updateSettingMenu(); // Update the menu display
}

void presetsAction() {
  Serial.println("Presets action");
  currentMenu = presetMenu; // Change the menu to settingMenu
  menuInUse = 2;
  menuMaxLenght = sizeof(presetMenu) / sizeof(presetMenu[0])-1;
  selectedMenuItem = 0; // Reset the selected menu item
  menuStartIndex = 0; // Reset the menu start index
  updatePresetMenu(); // Update the menu display
}

//Setting menu

void backAction() {
  Serial.println("Back action");
  currentMenu = mainMenu; // Change back to the main menu
  menuInUse = 0;
  menuMaxLenght = sizeof(mainMenu) / sizeof(mainMenu[0])-1;
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
  lcd.print("User manual for bombGithub.com/lasse_005                    /CSBomb             ");
}

void factoryResetAction() {
  timeToPlant = 10; // Initialize with a default value
  timeToDefuse = 10; // Initialize with a default value
  codeLenght = 8;
  buzzerPitch = 5;
  mistakeTime = 2;
  delayForNumbers = 2;
  lockedMenu = false;
  backLight = true;
  saveSettingsToEEPROM();
  backAction();
}

//Presets
void setTimeCodes(int plantTime, int defuseTime) {
    backAction();
    timeToDefuse = defuseTime;
    timeToPlant = plantTime;
}