/*
    Baja Control Code

    An arduino sketch which controls the motor and servos based on serial input data

    current sensor - 0 current has value  505-506. 505 to be safe
*/

#include <stdint.h>
typedef __attribute__ ((packed)) struct {
    int16_t gas;
    int16_t turn;
    int16_t rot;
} controlMessage;

controlMessage msg;

#include <Servo.h>
//#include <LowPower.h>

#include "serialctrl.h"

// Low power mode UART
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>

#define SERVO_RIGHTMOST 9
#define SERVO_MIDDLERIGHT 10
#define SERVO_MIDDLELEFT 11
#define SERVO_LEFTMOST 6
#define SERVO_UPPER 5

#define VOLTAGE A0
#define TEMPERATURE A1
#define CURRENT A3
#define CURRENT_MOTOR A4
#define ACCEL_X A7
#define ACCEL_Y A6
#define ACCEL_Z A5

#define PWM 2
#define SLP 3           // 0 means sleep, 1 means active
#define DIR 4           // 0 is backwards, 1 is forwards
#define FLT 7
#define ECHO 8
#define TRIGGER 13
#define SWITCH1 12
#define SWITCH2 22
#define SWITCH3 23

Servo steering;
Servo claw;
Servo armRotation;
Servo armUpDown;
Servo armMain;

void setup() {
    // Start comms with Raspi
    Serial.begin(115200);

    // Set up the motor controller
    pinMode(PWM,OUTPUT);
    pinMode(SLP,OUTPUT);
    pinMode(DIR,OUTPUT);

    // The motor controller starts in sleep mode, and direction is forward.
    digitalWrite(SLP,0);
    digitalWrite(DIR,1);
    analogWrite(PWM,0);

    // Whether there was an overcurrent in the current sensor.
    pinMode(FLT,INPUT);

    // Distance sensor
    pinMode(ECHO,INPUT); // ECHO
    pinMode(TRIGGER, OUTPUT); // TRIGGER
    digitalWrite(TRIGGER,LOW);

    // The Switch with values ABC. These values allow us to read the switch value.
    pinMode(SWITCH1,INPUT_PULLUP);
    pinMode(SWITCH3,INPUT_PULLUP);
    pinMode(SWITCH2,OUTPUT);
    digitalWrite(SWITCH2,0);

    // Move all servos to their "init" locations
    servoInit();

    // Wait for serial command, and beep on acquisition
    while (!Serial.available()) {
        delay(20);
    }

    buzzMotor(2,70,70);

    digitalWrite(SLP,1);
}

void servoInit() {
    // Servos!
    steering.attach(SERVO_RIGHTMOST,1000,2000);
    claw.attach(SERVO_LEFTMOST,1000,2000);
    armMain.attach(SERVO_MIDDLELEFT,640,2070);
    armUpDown.attach(SERVO_MIDDLERIGHT,640,2070);
    armRotation.attach(SERVO_UPPER,640,2070);

    // Initialize servos to a safe setting

    // Steering is weird in that it has a range of
    // 50-142, with middle at 96. This is due to weirdness
    // in the servo library when changing the timing
    steering.write(96);

    // Others are normal
    claw.write(90);
    armMain.write(90);
    armUpDown.write(90);
    armRotation.write(90);
}


void buzzMotor(int number,int timeOn,int timeOff) {
    // buzz the motor
    int slp_state = digitalRead(SLP);
    digitalWrite(SLP,1);

    analogWrite(PWM,5);
    delay(timeOn);
    analogWrite(PWM,0);
    for (int i=1;i<number;i++) {
        delay(timeOff);
        analogWrite(PWM,5);
    delay(timeOn);
    analogWrite(PWM,0);
    }

    digitalWrite(SLP,slp_state);
}
/*
// This is used on low battery to basically kill the chip so that it uses as little current as possible.
// We periodically wake it to complain about battery by buzzing the motor
void lowBatteryMode() {
    steering.detach();
    claw.detach();
    armMain.detach();
    armUpDown.detach();
    armRotation.detach();

    // Controller in Sleep mode
    digitalWrite(SLP,0);

    while (true) {
        
        // Complain about low power
        buzzMotor(3,500,500);

        // Wait 80 seconds
        for (int i=0;i < 10;i++) {
            LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        }

    }
    
}
/*
void sleepUntilSerial() {
    // Set low power mode. Note that servos must be reinitialized after This
    // This mode puts the arduino into sleep mode where everything is turned off
    // except the UART

    steering.detach();
    claw.detach();
    armMain.detach();
    armUpDown.detach();
    armRotation.detach();

    // Controller in Sleep mode
    digitalWrite(SLP,0);

    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();

    // Disable ALL the things
    power_adc_disable();
    power_spi_disable();
    power_timer0_disable();
    power_timer1_disable();
    power_timer2_disable();
    power_timer3_disable();
    power_twi_disable();

    sleep_mode(); // here the device is actually put to sleep!!

    // Whoa, wake up time
    sleep_disable();
    // Power up everything
    power_all_enable();

    // Reinitialize the servos
    servoInit();

    buzzMotor(2,70,70);
}


/*
 Returns the setting of the 3-position switch on the bottom.
 1: A, 2: B, 3: C
*/
int getSwitch() {
    if (digitalRead(SWITCH3)==1) return 1;
    if (digitalRead(SWITCH1)==1) return 2;
    return 3;
}

void resetOutput() {
    analogWrite(PWM,0);
    digitalWrite(DIR,1);

    // Steering is weird in that it has a range of
    // 50-142, with middle at 96. This is due to weirdness
    // in the servo library when changing the timing
    steering.write(96);

    // Others are normal
    claw.write(90);
    armMain.write(90);
    armUpDown.write(90);
    armRotation.write(90);
}


void readSensors() {
    Serial.print(analogRead(VOLTAGE));
    Serial.print(',');
    Serial.print(analogRead(TEMPERATURE));
    Serial.print(',');
    Serial.print(analogRead(CURRENT));
    Serial.print(',');
    Serial.print(analogRead(CURRENT_MOTOR));
    Serial.print(',');
    Serial.print(analogRead(ACCEL_X));
    Serial.print(',');
    Serial.print(analogRead(ACCEL_Y));
    Serial.print(',');
    Serial.print(analogRead(ACCEL_Z));
    Serial.print(',');
    Serial.print(digitalRead(FLT));
    Serial.print(',');
    Serial.println(getSwitch());
}

void runControls() {
    unsigned long prevSignal = millis();
    while (!Serial.available()) {
        if (prevSignal + 300 < millis()) {
            resetOutput();
        }
    }
    long pwm = Serial.parseInt();
    long dir = Serial.parseInt();
    long steer = Serial.parseInt();
    long slp = Serial.parseInt();
    long rot = Serial.parseInt();
    //long ud = Serial.parseInt();
    //long m = Serial.parseInt();
    //long claw = Serial.parseInt();
    
    // Let's limit the outputs to correct values
    pwm = (pwm > 40? 40:pwm);
    pwm = (pwm <0? 0:pwm);
    steer = (steer>142?142:steer);
    steer = (steer<50?50:steer);
    
    rot = (rot>180?180:rot);
    rot = (rot<0?0:rot);
    /*
    ud = (ud>180?180:ud);
    ud = (ud<0?0:ud);
    m = (m>180?180:m);
    m = (m<0?0:m);
    claw = (claw>180?180:claw);
    claw = (claw<0?0:claw);
    */

    analogWrite(PWM,pwm);
    digitalWrite(DIR,dir>0);
    digitalWrite(SLP,slp>0);

    steering.write(steer);
    armRotation.write(rot);
    
    // Let's leave the rest alone for now...

    
}

bool started = false;
// Run in a loop
void loop() {
    /*
    int pos;
   for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    armRotation.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
        if (pos==90) delay(1000);
    }
    delay(1000);
    for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
        armRotation.write(pos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15ms for the servo to reach the position
    }
    //delay(1000);
    */
    /*
    digitalWrite(TRIGGER,HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER,LOW);
    unsigned long duration = pulseIn(ECHO,HIGH,100000);
    float distance = duration/58.2;
    Serial.println(distance);
    delay(200);
    */
    /*
    analogWrite(PWM,20);
    delay(500);
    analogWrite(PWM,0);
    delay(3000);
    */
   //runget(Serial);
   Serial.readBytes((char*)&msg,sizeof(msg));
   if (started) {
       //Serial.print(msg.gas);
        //Serial.print(",");
        //Serial.println(msg.turn);
        //Serial.print(",");
        //Serial.println(msg.rot);

        long steer = msg.turn/13 + 92;
        steer = (steer>167?167:steer);
            steer = (steer<17?17:steer);
        steering.write(steer);

        long rot = msg.rot/8 + 90;
        rot = (rot>180?180:rot);
        rot = (rot<0?0:rot);
        armRotation.write(rot);

        long pwm = msg.gas/25;
        digitalWrite(DIR,pwm>0);
        if (pwm < 0) {
            pwm = -pwm;
        }
        pwm = (pwm > 40? 40:pwm);
        pwm = (pwm <0? 0:pwm);
        analogWrite(PWM,pwm);

   } else {
       if (msg.turn==0 && msg.gas==0) {
           started = true;
           Serial.print("STARTED");
       } else {
           Serial.print("Got garbage: ");
           Serial.print(msg.gas);
            Serial.print(",");
            Serial.print(msg.turn);
            Serial.print(",");
            Serial.println(msg.rot);
       }
   }


   /*
    runControls();
    readSensors();
    */
   /*
   int pos;
   for (pos = 50; pos <= 142; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    steering.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
        if (pos==96) delay(1000);
    }
    delay(1000);
    for (pos = 142; pos >= 50; pos -= 1) { // goes from 180 degrees to 0 degrees
        steering.write(pos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15ms for the servo to reach the position
    }
    delay(1000);
    */
    /*
    Serial.println(rd());
    analogWrite(6,30);
    for (int i=0;i < 10;i++) {
        delay(100);
        Serial.println(rd());
    }
    analogWrite(6,0);
    
    for (int i=0;i < 10; i++) {
        Serial.println(rd());
        delay(1000);
    }

    for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
    if (pos==90) delay(8000);
  }
  delay(1000);
  for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  delay(2000);
  */
}