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

//Setup a oneWire instance to communicate with any OneWire Devices
OneWire oneWire(ONE_WIRE_BUS);
//Pass our oneWIre reference to Dallas Temperature
DallasTemperature sensors(&oneWire);


void setup() {
  // put your setup code here, to run once:
  sensors.begin(); //Start up the thermometer library (thermo only???)
  //cofigure the LCD (THERMOSTAT ONLY)\
  //First line for displaying the temp being read
  lcd.begin(16,2);
  lcd.print("CurrentTemp:");
  updateCurrentTemp();
  //second line for reading the temp the user set
  lcd.setCursor(0,1)
  lcd.print("SetTemp:");
  setTemp=75; //On boot system defaults to 75 deg
  lcd.setCursor(8,1);
  lcd.print(setTemp);
  //CONFIGURE BUTTONS HERE
  

}

void loop() {
  // put your main code here, to run repeatedly:

  

}


//FUNCTIONS BELOW

void updateCurrentTemp() //Function to read temp from onewire thermometer and display it on LCD in the proper location
{
  lcd.setCursor(0,12);
  sensors.requestTemperatures();//Send command to get temperature reading
  lcd.print(sensors.getTempFByIndex(0));//Get the temp in Farenheit from the sensor and print it 
}

