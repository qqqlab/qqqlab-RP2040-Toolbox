/*
Minimal Servo / PWM library for RP2040

NOTE: no checking is done on overlapping slices - user should take care not to assign different frequencies to two pins in the same slice.

*/

#include <RP2040_PWM.h>

PWM motor;
PWM servo;

float motor_pwm = 125;
float servo_pwm = 1000;

void setup() {
  motor.begin(1,2000,125,250); //pin 1 (slice 0): Oneshot motor ESC 2000Hz pulse 125-250 us
  servo.begin(2,50,1000,2000); //pin 2 (slice 1): regular servo 50Hz pulse 1000-2000 us 
}

void loop() {
  motor_pwm++;
  if(motor_pwm > motor.get_max_us()) motor_pwm = motor.get_min_us();
  motor.writeMicroseconds(motor_pwm);

  servo_pwm++;
  if(servo_pwm > servo.get_max_us()) servo_pwm = servo.get_min_us();
  servo.writeMicroseconds(servo_pwm);

  delay(3);
}