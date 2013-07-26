// *********************************************************************************
//  
//  Catears Project Mark 3 - Using compass as input data
//  by: Lambert Le
//  Licence: GPL, http://www.gnu.org/licenses/
//
//  Credit
//  Interface with HMC5883
//  by: Jordan McConnell
//  License: OSHW 1.0, http://freedomdefined.org/OSHW
//
// *********************************************************************************

#include <Wire.h> //I2C Arduino Library
#include <Servo.h> 

#define address 0x1E //0011110b, I2C 7bit address of HMC5883
Servo myservo;
int servoPin = 5;
int posMin = 100;
int posMax = 180;
int posChange = 5;
int positionRatioCalibration = 600;

int targetAngle = posMin;
int currentAngle = posMin;
int adjustRate = 1;

int x,y,z; //triple axis data

//Set the variable "targetAngle" to the new value based on the compass data
void setAngle(int pos)
{
  int mappedPos = map(pos, 0, positionRatioCalibration, posMin, posMax);
  targetAngle = mappedPos;
  Serial.print("\t\t set: ");
  Serial.print(mappedPos);
}

//Set the currentAngle to APPROACH the targetAngle. This is used to smooth the movement of the servo motors. It avoids the motors from jerking
void applyAngle()
{
  //(currentAngle+5) is used to avoid unecessary adjustement of the servo motor. So if the targetAngle is close enough to the currentAngle, then there is no need to apply a new angle. The currentAngle is unchanged.
  if ((currentAngle+5) < targetAngle)  
    currentAngle = currentAngle + adjustRate;
  else if ((currentAngle-5) > targetAngle)
    currentAngle = currentAngle - adjustRate;
  myservo.write(currentAngle);  //After re-adjusting the new angle, set the servo motor
  Serial.print("\t\t cur: ");
  Serial.print(currentAngle);
}

void setup(){
  //Initialize Serial and I2C communications
  pinMode(13, OUTPUT);   
  for(int i=0; i<4; i++)
  {
    digitalWrite(13, HIGH);
    delay(50);
    digitalWrite(13, LOW);
    delay(50);
  }
  
  pinMode(12, INPUT);
  digitalWrite(12, HIGH);
  if(digitalRead(12)==LOW)  //Only start serial if pin12 is low. Thus, if you plugged pin12 to GND. Serial.print slow down the servo motor update, only activate it in debug environment
    Serial.begin(9600);
  Serial.println("Start");
  Wire.begin();
  myservo.attach(servoPin);
  
  //Put the HMC5883 IC into the correct operating mode
  Wire.beginTransmission(address); //open communication with HMC5883
  Wire.write(0x02); //select mode register
  Wire.write(0x00); //continuous measurement mode
  Wire.endTransmission();
  Serial.println("Setup completed");
}

void loop(){
  //Tell the HMC5883 where to begin reading data
  Wire.beginTransmission(address);
  Wire.write(0x03); //select register 3, X MSB register
  Wire.endTransmission();
 
 //Read data from each axis, 2 registers per axis
  Wire.requestFrom(address, 6);
  if(6<=Wire.available()){
    x = Wire.read()<<8; //X msb
    x |= Wire.read(); //X lsb
    z = Wire.read()<<8; //Z msb
    z |= Wire.read(); //Z lsb
    y = Wire.read()<<8; //Y msb
    y |= Wire.read(); //Y lsb
  }
  
  //Print out values of each axis
  Serial.print("\t\t x: ");
  Serial.print(x);
  Serial.print("\t\t y: ");
  Serial.print(y);
  Serial.print("\t\t z: ");
  Serial.print(z);
  Serial.print("\t\t t: ");
  Serial.print(abs(x+y+z));
  
  setAngle(abs(x+y+z)%positionRatioCalibration);  
  applyAngle();
  
  digitalWrite(13, HIGH);
  //delay(1);
  digitalWrite(13, LOW);
  Serial.print("\n");
}

