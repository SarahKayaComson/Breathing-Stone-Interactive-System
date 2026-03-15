// Breathing Stone: Pulse Sensor & LED/Exciter Sync Test
int pulsePin = 0;                 
int blinkPin = 13;                
int fadePin = 5;                  

volatile int BPM;                   
volatile int Signal;                
volatile int IBI = 600;             
volatile boolean Pulse = false;     
volatile boolean QS = false;        

void setup(){
  pinMode(blinkPin,OUTPUT);         
  pinMode(fadePin,OUTPUT);          
  Serial.begin(115200);             
}

void loop(){
  if (QS == true){     
    fadeRate = 255;                  
    Serial.print("BPM: ");
    Serial.println(BPM);   
    QS = false;                      
  }
  ledFadeToBeat();                      
  delay(20);                             
}

void ledFadeToBeat(){
    // LED Fading logic for breathing effect
    // To be integrated with Max/MSP via Serial
}
