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
long lastTempCheck; //holds the last time the temp was checked
long lastEntry; //holds the last time the RTC was checked'

//Setup a oneWire instance to communicate with any OneWire Devices
OneWire oneWire(ONE_WIRE_BUS);


//Pass our oneWIre reference to Dallas Temperature
DallasTemperature sensors(&oneWire);


void setup() {
  pinMode(2, INPUT); //temp up button
  pinMode(3, INPUT); //temp down button
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  attachInterrupt(2, temp_Up, FALLING);
  attachInterrupt(3, temp_Down, FALLING);
  // put your setup code here, to run once:
  sensors.begin(); //Start up the thermometer library (thermo only???)
  //cofigure the LCD (THERMOSTAT ONLY)\
  //First line for displaying the temp being read
  lcd.begin(16,2);
  lcd.print("CurrentTemp:");
  update_Current_Temp();
  //second line for reading the temp the user set
  lcd.setCursor(0,1)
  lcd.print("SetTemp:");
  setTemp=75; //On boot system defaults to 75 deg
  lcd.setCursor(8,1);
  lcd.print(setTemp);
  //CONFIGURE BUTTONS HERE


  //Initalize time dependent variables
  lastTempCheck = millis();
  lastEntry = millis();

}

void loop() {
  // put your main code here, to run repeatedly:
  if(millis()- lastEntry > 1000)
  {
    if (//minute value has changed)
    {
    //Check Rtc and possibly send data
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

  if (currentTemp - setTemp > 1) //2 or more deg hotter than user wants
  {
    //turn on fan
  }
  else if (currentTemp - setTemp < -1) 
  {
    //turn on heat
  }
  else
  {
    //turn both off
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
    
  }
  else
  {
    useCurrent = currentLowValue;
    multCurrent = (useCurrent * 5)/1024;
    modCurrent = ((multCurrent - 2.5)*2)/3.525;;

  }
  multVolt = (voltageValue*5)/1024;
  modVoltage = (((multVolt - 2.5) *2)*(35.84));
  modPower  = modCurrent * modVoltage;
  totalPower = totalPower + (modVoltage * modCurrent);
  readsDone = readsDone+1;
  }
  avgPower = totalPower/readsDone;
  lastEntry = millis();
}

void temp_Up(){ //runs when temp up button is pressed
  if(setTemp <86)
  {
    setTemp = setTemp +1; //if set temp is below max increase by 1
  }
  lcd.setCursor(8,1); //set cursor and print new set temp
  lcd.print(setTemp);
  update_Current_Temp(); //update temp will toggle accesories as needed
}


void temp_Down(){ //runs when temp down button is pressed
  if(setTemp >64)
  {
    setTemp = setTemp -1;
  }
  lcd.setCursor(8,1); //set cursor and print new set temp
  lcd.print(setTemp);
  update_Current_Temp();  //update temp will toggle accesories as needed
}
