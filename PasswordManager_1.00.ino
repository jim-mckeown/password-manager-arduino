/******************************************************************
*  
*  Send password associated with each button combination to keyboard
*  Apply power with unlock button combination pressed to enter mode 1 
*  Use a serial terminal such as putty to enter mode 2 (editPW)
*  
******************************************************************/

#include "Keyboard.h"
#include <EEPROM.h>

String rev = "1.00";
String releaseDate = "13 June 2023";
String author = "Jim McKeown";
uint8_t unlock = 10; // unlock button value 
int eeStrOffset = 64; // size of passwords in bytes, including null terminator
int pwLength = EEPROM.length() / eeStrOffset; // maximum number of passwords

uint8_t mode = 0; // mode 0: locked (default on power-up)
                  // mode 1: read buttons (after unlock code entered)
                  // mode 2: edit passwords ('editPW' from USB serial port) 

uint8_t id = 0; // password index

uint8_t gpioValue = 0; // value read from 4 buttons (0 - 15)
uint8_t gpioLast = 0;  // previous value read from 4 buttons
bool textSent = true;
int readCount = 0;
int readMax = 5000;

void setup()
{
  //configure pin 2 - 5 as input and enable the internal pull-up resistor
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
    
  Keyboard.begin(); // setup usb keyboard
  Serial.begin(57600); // setup usb serial port
  delay(1000);

  gpioValue = getButtons(); // read buttons
  if(gpioValue == unlock) mode = 1; // unlock if unlock buttons pressed on power-up
}

void loop()
{
  if(mode == 0 || mode == 1) // normal button to password mode
  {
    gpioValue = getButtons(); // check for button press
    if(gpioValue == gpioLast)
    {
      readCount++;
      if(readCount > readMax)
      {
        readCount = readMax; // avoid overflow, limit readCount variable
        if(gpioValue != 0) 
        {
          if(textSent == false)
          {
            //eeReadString(gpioValue); // send text at gpioValue index in eeprom
            if(mode == 0)
            {
              // just send button number to keyboard if device is locked
              Keyboard.print(gpioValue); // send button value to keyboard
            }
            else
            {
              Keyboard.print(eeReadString(gpioValue * eeStrOffset)); // send password to keyboard
            } 
            textSent = true; // set textSent flag         
          }
        }
        else
        {
          textSent = false; // clear text sent flag
        }
      }
    }
    else
    {
      readCount = 0;
      gpioLast = gpioValue;
    }
  }
  
  if(mode == 2) // edit passwords
  {
    Serial.println("Ready to edit password");
    Serial.print("Please type in the ID # from 1 to ");
    Serial.print(pwLength - 1);
    Serial.println(" you want to save this password as...");
    id = readnumber();
    if (id > 0 && id < pwLength) 
    {
      Serial.print("Editing password for ID #");
      Serial.println(id);
      Serial.print("Enter password for ID "); 
      Serial.print(id); 
      Serial.print(" max "); 
      Serial.print(eeStrOffset - 1); 
      Serial.println(" characters:");
      String pw = readStr();
      Serial.print("Password = "); Serial.println(pw);
      int eeAddress = id * eeStrOffset;
      eeWriteString(eeAddress, pw);
    }
  }

  // check serial port for text commands
  if(Serial.available() != 0) // check for serial input
  {
    String strCmd = Serial.readString();
    if(strCmd.startsWith("editPW"))
    {
      mode = 2;
    }
    if(strCmd.startsWith("?"))
    {
      about();
    }
  }
} 

/***************************************************************
 * Send information on firmware, fingerprint sensor, eeprom
 * to serial port.
 * 
 **************************************************************/
void about()
{
  Serial.println("Password Manager");
  Serial.print("Version "); Serial.println(rev); 
  Serial.print("Release date: "); Serial.println(releaseDate); 
  Serial.print("Author: "); Serial.println(author);    
  Serial.print("Maximum password length = "); Serial.println(eeStrOffset - 1);
  Serial.print("Maximum number of passwords = "); Serial.println(pwLength - 1);  
  Serial.println("Enter 'editPW' to edit passwords.");
  Serial.println("Enter '? to see this information with no mode change.");
}

uint8_t readnumber(void) {
  uint8_t num = -1;
  while (! Serial.available());
  num = Serial.parseInt();
  Serial.readString(); // clear serial buffer
  return num;
}

String readStr(void) 
{
  while (! Serial.available());
  {
    delay(1);
  }
  String strRead = Serial.readString();
  return strRead;
}

void eeWriteString(int eeAddr, String strWrite)
{
  int strLen = strWrite.length() + 1;
  char charArray[strLen];
  strWrite.toCharArray(charArray, strLen);
  for(int i = 0;i < strLen;i++)
  {
    EEPROM.write(eeAddr + i, charArray[i]);
  }
}

String eeReadString(int eeAddr)
{
  char charArray[eeStrOffset];
  for(int i = 0;i < eeStrOffset;i++)
  {
    charArray[i] = EEPROM.read(eeAddr + i);
    if(charArray[i] == 0)
    {
      break;
    }
  }
  return String(charArray);
}

//read the pushbutton value into a variable. buttons are inverted.
uint8_t getButtons()
{
  uint8_t buttons = 0;
  if(digitalRead(2) == 0) buttons += 1;
  if(digitalRead(3) == 0) buttons += 2;
  if(digitalRead(4) == 0) buttons += 4;
  if(digitalRead(5) == 0) buttons += 8;
  return buttons;  
}
