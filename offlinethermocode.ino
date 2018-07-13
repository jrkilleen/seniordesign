//UCF Summer 2018 Group 10 Senior Design Code
//Code for thermostat to be modified for Outlet and Switch
#include <LiquidCrystal.h> // (THERMOSTAT ONLY)
#include <OneWire.h> //(THERMOSTAT ONLY)
#include <DallasTemperature.h> //(THERMOSTAT ONLY)
#include <ArduinoJson.h>;
//Define Pins
#define ONE_WIRE_BUS 33 //PUT THERMOSTAT DATA PIN HERE
LiquidCrystal lcd(22,24,29,30,31,32); //DEFINE LCD PINS HERE IN ORDER OF (RS, E, DB4,DB5,DB6,DB7) (THERMOSTAT ONLY)

//Declare Variables
float currentTemp; //Temp value read from the thermometer (THERMOSTAT ONLY)
int setTemp; //Temperature set by the user (THERMOSTAT ONLY)
int relayFlag;
long lastTempCheck; //c the last time the temp was checked
long lastEntry; //holds the last time the RTC was checked
long lastMinute; //last time power data was sent;
long lastRelay; // last time relay status was chcked
float minutePower; // holds the total power for the current minute
long timeStamp; //the minute the power data is sent for
long waitTime; //remainer in s of time recieved from server, so that all power is forn a single minute on the minute
long delayTime; //wait time converted to millis for delay() purposes
int espMode;
int isOn;
boolean toggle;



//Setup a oneWire instance to communicate with any OneWire Devices
OneWire oneWire(ONE_WIRE_BUS);

//Json Object variables
char *buffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *serialbuffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *outbuffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

//Pass our oneWIre reference to Dallas Temperature
DallasTemperature sensors(&oneWire);


void setup() {
  init_ESP();
  delay(5000);
  Serial.begin(9600);
  toggle = false;
  pinMode(2, INPUT); //temp up button
  pinMode(3, INPUT); //temp down button
  pinMode(18, INPUT);
  pinMode(45, OUTPUT); //LED
  pinMode(46, OUTPUT);//OTHER LED
  pinMode(25, OUTPUT); //ESP RESET
  pinMode(36, INPUT); //esp gpio2
  pinMode(34, OUTPUT);
  relayFlag = 0;
  digitalWrite(34, LOW);
  attachInterrupt(0, temp_Up, RISING); //button interrupts occur on rising edge
  attachInterrupt(1, temp_Down, RISING);
  attachInterrupt(2, switch_Mode,RISING);
  sensors.begin(); //Start up the thermometer library (thermo only???)
  //cofigure the LCD (THERMOSTAT ONLY)\
  //First line for displaying the temp being read
  lcd.begin(16,2);
  lcd.print("CurrentTemp:");
  setTemp=75; //On boot system defaults to 75 deg
  update_Current_Temp();
  //delay(20000);
  //Serial.println("Post Delay");
  Serial.flush();
  ready_To_Send();
  //get_Relay();
  //Get current time, calculate start and don't do anything till then;
  get_Time();
  delayTime = waitTime * 1000;
  delay(delayTime);
  //Initalize time dependent variables
  lastTempCheck = millis();
  lastRelay = millis();
  lastEntry = millis();
  lastMinute = millis();

}

void loop() {
  // put your main code here, to run repeatedly:
  if(millis()- lastEntry > 1000)
  {
    if (millis() - lastMinute > 60000)
    {
    send_Data();
    lastMinute = millis();
    timeStamp = timeStamp + 60;;
    }
    else
    {
      do_Reads();
    }
  }

   if(millis() - lastRelay > 2500)
  {
    handle_Relay();
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
  currentTemp = sensors.getTempFByIndex(0);
  lcd.print(sensors.getTempFByIndex(0));//Get the temp in Farenheit from the sensor and print it 
  lcd.setCursor(0,1);
  lcd.print("SetTemp:");
  lcd.setCursor(8,1);
  lcd.print(setTemp);
  lcd.print("      ");
  if ((currentTemp - setTemp) > 1) //2 or more deg hotter than user wants
  {
    isOn=1;
     //digitalWrite(45, HIGH);   // turn the LED on (HIGH is the voltage level)
     //digitalWrite(46, LOW);

  }
  else if (currentTemp - setTemp < -1) 
  {
    isOn=1;
    //digitalWrite(45, LOW);   // turn the LED on (HIGH is the voltage level)
    //digitalWrite(46, HIGH);

  }
  else
  {
    isOn = 0;
    //digitalWrite(45, LOW);
    //digitalWrite(46, LOW);
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
  float secondPower;
  long sampleTracker;
  float multCurrent;
  float modCurrent;
  
  readsDone = 0;
  totalPower = 0;      
  sampleTracker = millis();
  while(millis() - sampleTracker < 16)
  {
  voltageValue = analogRead(8);
  currentLowValue = analogRead(9);
  currentHighValue = analogRead(10);
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
  multVolt = voltageValue - 512;
  modVolt = multVolt * .4176904296875;
  totalPower = totalPower + (modVolt * modCurrent);
  readsDone = readsDone+1;
  }
  //avgPower = totalPower/readsDone;
  secondPower = totalPower * 62.5;
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
  minutePower = random(1,100); //THERMO ONLY!!!!
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
    StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = "currentTime";
  String message;
  root.printTo(message);
  message.toCharArray(outbuffer, 100);
  Serial.println(outbuffer);
  int timeSet = 0;
  
  while( timeSet != 1)
  {
    //Serial.println("in get time loop!");
    if (Serial.available() > 0)
    {
      DynamicJsonBuffer jsonBuffer(512);
      JsonObject& root = jsonBuffer.parseObject(Serial);
      const char* type = root["type"];

      if(root.success())
      {
        //digitalWrite(46, HIGH);
        if(strcmp(type, "currentTime") == 0)
        {
          long rawTime = root["currentTime"];
          waitTime = 60 - (rawTime % 60);
          timeStamp = (rawTime + waitTime) - 60;
          timeSet = 1;
        }
      }
    }
  }
}

void ready_To_Send(){
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = "readytosend";
  String message;
  int isReady = 0;
  //Serial.println("ready_To_Send Starts!");
  while(isReady == 0)
  {
  //Serial.println("in ready to send loop!");
  root.printTo(message);
  message.toCharArray(outbuffer, 100);
  Serial.println(outbuffer);

  for(int x = 0; x<10; x++){
  if (Serial.available() > 0)
    {
      DynamicJsonBuffer jsonBuffer(512);
      JsonObject& root = jsonBuffer.parseObject(Serial);
      const char* type = root["type"];

      if(root.success())
      {
        if(strcmp(type, "readytosend") == 0)
        { 
          espMode = root["mode"];
          isReady = 1;
        }
      }
    }
    delay(1000);
  }
  }
}

void switch_Mode(){
  if (espMode == 0)
  {
    espMode = 1;
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = "restart";
  root["mode"] = espMode;
  String message;
  root.printTo(message);
  message.toCharArray(outbuffer, 100);
  Serial.println(outbuffer);
  }
  if (espMode == 1)
  {
    espMode = 0;
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = "restart";
  root["mode"] = espMode;
  String message;
  root.printTo(message);
  message.toCharArray(outbuffer, 100);
  Serial.println(outbuffer);
  }
}




void handle_Relay(){
  if (relayFlag == 0)
  {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = "relayUpdate";
  String message;
  int isReady = 0;
  root.printTo(message);
  message.toCharArray(outbuffer, 100);
  Serial.println(outbuffer);
  relayFlag = 1;
  }
  if (relayFlag = 1)
  {
      if (Serial.available() > 0)
    {
      DynamicJsonBuffer jsonBuffer(512);
      JsonObject& root = jsonBuffer.parseObject(Serial);
      const char* type = root["type"];

      if(root.success())
      {
        digitalWrite(46,toggle);
        toggle = !toggle;
        if(strcmp(type, "relayUpdate") == 0)
        {
          const char* relay = root["relay"];
          if(strcmp(relay,"1") == 0)
          {
           digitalWrite(34,LOW);
          }
          if(strcmp(relay,"0") == 0)
          {
           digitalWrite(34,HIGH);
          }
        }
        relayFlag = 0;
      }
      relayFlag = 0;

    }
  }
  lastRelay = millis();
}




void init_ESP(){
  pinMode(35, OUTPUT); //ESP RESET
  digitalWrite(35,HIGH);
  delay(1000);
  digitalWrite(35, LOW);
  delay(1000);
  digitalWrite(35, HIGH);
  delay(1000);
}

