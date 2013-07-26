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
//  3-Axis 16-Bit Compass Module (HMC5883) Demo code.
//  2011 Copyright (c)Robosoft Systems, India.  All right reserved.
//  Author: Rohan Soni  ( Code Design and Comments)
//  License: GNU Lesser General Public
//
// *********************************************************************************

#include <Wire.h> //I2C Arduino Library
#include <math.h>
#include <Servo.h> 

//Gyroscope Parameters
#define HMC5883_WriteAddress 0x1E //  i.e 0x3C >> 1
#define HMC5883_ModeRegisterAddress 0x02
#define HMC5883_ContinuousModeCommand 0x00
#define HMC5883_DataOutputXMSBAddress  0x03
int regb=0x01;
int regbdata=0x40;
int outputData[6];

//Servo Par
Servo myservo;
int servoPin = 5;
int posMin = 70;
int posMax = 180;
int targetAngle = posMin;
int loopCount = 0;
#define AVG_CNT_MAX 12
int arrayTargetAngle[AVG_CNT_MAX];
int currentAngle = posMin;
int dividerRatio = 45; //let dividerRatio be "X". Let say the compass rotate by Y degrees, then the servo will move by (360%X)*Y . This is used to amplify the servo movements
int adjustRate = 2; //the rate at which the servo motor can move

//Functions
void setAngle(int);
void applyAngle();

void setup(){
    //Determine if in debug mode
    int debug = 0;
    pinMode(12, INPUT);
    digitalWrite(12, HIGH);
    if(digitalRead(12)==LOW)
      debug = 1;  //Only start DEBUG MODE if pin12 is low. Thus, if you plugged pin12 to GND.
  
    //Start Indicator
    pinMode(13, OUTPUT);   
    for(int i=0; i<4; i++)
    {
      digitalWrite(13, HIGH);
      delay(50);
      digitalWrite(13, LOW);
      delay(50);
    }
    
    //Initialize Serial
    if(debug == 1)  //Serial.print slow down the servo motor update, only activate it in debug environment
      Serial.begin(9600);
    Serial.println("");
    Serial.println("************");
    Serial.println("Start - Debug Mode");
    Serial.println("Initialize Serial");
    
    
    //Initialize SPI
    Serial.println("Initialize SPI");
    Wire.begin();
    
    //Initialize Servo Motors
    Serial.println("Initialize Servo Motors");
    myservo.attach(servoPin);
    if(debug == 2)
    {
      myservo.write(posMin);
      Serial.println("servo-min");
      delay(1000);
      myservo.write(posMax);
      Serial.println("servo-max");
      delay(1000);
    }
    
    //Initialize Averaging Angle Function
    Serial.println("Initialize Averaging Angle Function");
    for(int i=0; i<AVG_CNT_MAX; i++)
    {
       arrayTargetAngle[i] = posMin;
    }
    Serial.println("************");
}

void loop(){
    int i,x,y,z;
    double angle;
 
    Wire.beginTransmission(HMC5883_WriteAddress);
    Wire.write(regb);
    Wire.write(regbdata);
    Wire.endTransmission();
 
    delay(1);
    Wire.beginTransmission(HMC5883_WriteAddress); //Initiate a transmission with HMC5883 (Write address).
    Wire.write(HMC5883_ModeRegisterAddress);       //Place the Mode Register Address in send-buffer.
    Wire.write(HMC5883_ContinuousModeCommand);     //Place the command for Continuous operation Mode in send-buffer.
    Wire.endTransmission();                       //Send the send-buffer to HMC5883 and end the I2C transmission.
    delay(1);
 
    Wire.beginTransmission(HMC5883_WriteAddress);  //Initiate a transmission with HMC5883 (Write address).
    Wire.requestFrom(HMC5883_WriteAddress,6);      //Request 6 bytes of data from the address specified.
 
    digitalWrite(13, HIGH);
    delay(5);
    digitalWrite(13, LOW);
    
    //Read the value of magnetic components X,Y and Z
    if(6 <= Wire.available()) // If the number of bytes available for reading be <=6.
    {
        for(i=0;i<6;i++)
        {
            outputData[i]=Wire.read();  //Store the data in outputData buffer
        }
    }
     
    x=outputData[0] << 8 | outputData[1]; //Combine MSB and LSB of X Data output register
    z=outputData[2] << 8 | outputData[3]; //Combine MSB and LSB of Z Data output register
    y=outputData[4] << 8 | outputData[5]; //Combine MSB and LSB of Y Data output register
     
    angle= atan2((double)y,(double)x) * (180 / 3.14159265) + 180; // angle in degrees
    
    //setAngle(abs(x+y+z)%positionRatioCalibration);  
    setAngle((int)angle%dividerRatio);
    applyAngle();
    
    //Serial.print("\n");
    //Serial.println((int)angle);
    outputArray();
    Serial.print("\n");
}

//Set the variable "targetAngle" to the new value based on the compass data
void setAngle(int pos)
{
    //int mappedPos = map(pos, 0, positionRatioCalibration, posMin, posMax);
    int mappedPos = map(pos, 0, dividerRatio, posMin, posMax);
    
    loopCount++;
    loopCount = loopCount % AVG_CNT_MAX;
    arrayTargetAngle[loopCount] = mappedPos;
    int tempAverage;
    for(int i=0; i<AVG_CNT_MAX; i++)
    {
      tempAverage = tempAverage + arrayTargetAngle[i];
    }
    targetAngle = tempAverage / AVG_CNT_MAX;
    
    Serial.print("\t\t pos: ");
    Serial.print(pos);
    Serial.print(" => mpos: ");
    Serial.print(mappedPos);
    Serial.print(" -> ");
    Serial.print(tempAverage);
    Serial.print("/");    
    Serial.print(AVG_CNT_MAX);
    Serial.print("=");        
    Serial.print(targetAngle);
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

void outputArray()
{
    Serial.print("[");
    for(int i=0; i<AVG_CNT_MAX; i++)
    {
      Serial.print(arrayTargetAngle[i]);
      Serial.print(",");
    }
    Serial.print("]");
}
