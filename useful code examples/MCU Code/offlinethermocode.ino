//UCF Summer 2018 Group 10 Senior Design Code
//Code for thermostat to be modified for Outlet and Switch
#include <LiquidCrystal.h> // (THERMOSTAT ONLY)
#include <OneWire.h> //(THERMOSTAT ONLY)
#include <DallasTemperature.h> //(THERMOSTAT ONLY)

//Define Pins
#define ONE_WIRE_BUS 33 //PUT THERMOSTAT DATA PIN HERE
LiquidCrystal lcd(22,24,29,30,31,32); //DEFINE LCD PINS HERE IN ORDER OF (RS, E, DB4,DB5,DB6,DB7) (THERMOSTAT ONLY)

//Declare Variables
float currentTemp; //Temp value read from the thermometer (THERMOSTAT ONLY)
int setTemp; //Temperature set by the user (THERMOSTAT ONLY)
long lastTempCheck; //c the last time the temp was checked
long lastEntry; //holds the last time the RTC was checked
long lastMinute;
float minutePower; // holds the total power for the current minute
long timeStamp; //the minute the power data is sent for
long waitTime; //remainer in s of time recieved from server, so that all power is forn a single minute on the minute
long delayTime; //wait time converted to millis for delay() purposes


//Setup a oneWire instance to communicate with any OneWire Devices
OneWire oneWire(ONE_WIRE_BUS);

//Json Object variables
char *buffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *serialbuffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *outbuffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

//Pass our oneWIre reference to Dallas Temperature
DallasTemperature sensors(&oneWire);


void setup() {
  pinMode(2, INPUT); //temp up button
  pinMode(3, INPUT); //temp down button
  attachInterrupt(0, temp_Up, RISING); //button interrupts occur on rising edge
  attachInterrupt(1, temp_Down, RISING);
  sensors.begin(); //Start up the thermometer library (thermo only???)
  //cofigure the LCD (THERMOSTAT ONLY)\
  //First line for displaying the temp being read
  lcd.begin(16,2);
  lcd.print("CurrentTemp:");
  setTemp=75; //On boot system defaults to 75 deg
  update_Current_Temp();
  delay(5000);
  ready_To_Semd();

  //Get current time, calculate start and don't do anything till then;
  get_Time();
  delayTime = waitTime * 1000;
  delay(delayTime);
  //Initalize time dependent variables
  lastTempCheck = millis();
  lastEntry = millis();

}

void loop() {
  // put your main code here, to run repeatedly:
  if(millis()- lastEntry > 1000)
  {
    if (millis() - lastMinute > 60000)
    {
    send_Data();
    lastMinute = millis();
    timeStamp = timeStamp+ 60;
    }
    else
    {
      do_Reads();
    }
  }



  
 if (millis() - lastTempCheck > 30000) //
 {
  update_Current_Temp();
 }
}


//FUNCTIONS BELOW
void update_Current_Temp() //Function to read temp from onewire thermometer and display it on LCD in the proper location
{
  lcd.setCursor(12,0);
  sensors.requestTemperatures();//Send command to get temperature reading
  lcd.print(sensors.getTempFByIndex(0));//Get the temp in Farenheit from the sensor and print it 
  lcd.setCursor(0,1);
  lcd.print("SetTemp:");
  lcd.setCursor(8,1);
  lcd.print(setTemp);
  lcd.print("      ");
  if (currentTemp - setTemp > 1) //2 or more deg hotter than user wants
  {
     digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)

  }
  else if (currentTemp - setTemp < -1) 
  {
    digitalWrite(13, LOW);   // turn the LED on (HIGH is the voltage level)

  }
  
  lastTempCheck = millis();
}

void do_Reads(){
  int readsDone = 0; //tracks number of reads done in while loop
  float voltageValue; //Voltage value Read from ADC
  float currentLowValue; //Value for low current (HIGHER GAIN) from adc
  float currentHighValue; //Value for high current (LOWER GAIN) from adc
  float useCurrent; //holds adc current value thata will be used in real current
  float multVolt; //scaled voltage from adc val to 0-5v val
  float modVolt; //real voltage val
  float totalPower; //total power for sample period
  float avgPower; //avg power for sample period
  

  readsDone = 0;
  totalPower = 0;      
  sampleTracker = millis();
  while(millis() - sampleTracker < 16)
  {
  voltageValue = analogRead(A8);
  currentLowValue = analogRead(A9);
  currentHighValue = analogRead(A10);
  if(currentLowValue >983)
  {
    useCurrent = currentHighValue;
    multCurrent = (useCurrent * 5)/1024;
    modCurrent = ((multCurrent - 2.5) * 400)/47;
    
  }
  else
  {
    useCurrent = currentLowValue;
    multCurrent = (useCurrent * 5)/1024;
    modCurrent = ((multCurrent - 2.5)*2)/3.525;

  }
  multVolt = (voltageValue*5)/1024;
  modVoltage = (((multVolt - 2.5) *2)*(35.84));
  modPower  = modCurrent * modVoltage;
  totalPower = totalPower + (modVoltage * modCurrent);
  readsDone = readsDone+1;
  }
  avgPower = totalPower/readsDone;
  minutePower = minutePower + totalPower;
  lastEntry = millis();
}

void temp_Up(){ //runs when temp up button is pressed
  if(setTemp <86)
  {
    setTemp = setTemp +1; //if set temp is below max increase by 1
  }
  update_Current_Temp(); //update temp will toggle accesories as needed
}


void temp_Down(){ //runs when temp down button is pressed
  if(setTemp >64)
  {
    setTemp = setTemp -1;
  }
  update_Current_Temp();  //update temp will toggle accesories as needed
}

void send_Data(){
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& powerData = jsonBuffer.createObject();
  powerData["type"] = "powerUpdate";
  powerData["totalPower"] = minutePower;
  powerData["timeStamp"] = timeStamp;

  String message;

  powerData.printTo(message);
  message.toCharArray(outbuffer, 100);
  Serial.println(outbuffer);

}

void get_Time(){
  int timeSet = 0;

  while( timeSet != 1)
  {
    if (Serial.available() > 0)
    {
      DynamicJsonBuffer jsonBuffer(512);
      JsonObject& root = jsonBuffer.parseObject(Serial);
      const char* type = root["type"];

      if(root.success())
      {
        if(strcmp(type, "currentTime") == 0)
        {
          long rawTime = root["time"];
          waitTime = 60 - (rawTime % 60);
          timeStamp = rawTime + waitTime;
          timeSet = 1;
        }
      }
    }
  }
}

void ready_To_Send(){
  
  int isReady = 0;
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& powerData = jsonBuffer.createObject();
  root["type"] = "readytosend";
  String message;

  while(isReady != 1)
  {
  powerData.printTo(message);
  message.toCharArray(outbuffer, 100);
  Serial.println(outbuffer);
  
  if (Serial.available() > 0)
    {
      DynamicJsonBuffer jsonBuffer(512);
      JsonObject& root = jsonBuffer.parseObject(Serial);
      const char* type = root["type"];

      if(root.success())
      {
        if(strcmp(type, "readytosend") == 0)
        {
          isReady = root["readytosend"];
        }
      }
    }
    delay(5000);
  }
}

