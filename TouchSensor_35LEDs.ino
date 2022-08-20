//為方便梳理線，並非以順序的方式對應LED0 ~ LED34順序，分別為以下順序對應，可自行修改
#include "Wire.h"

#define MCCVI_MPR121_A_ADD 0x5A //觸控I2C地址對應LED0 ~ LED11
#define MCCVI_MPR121_B_ADD 0x5B //觸控I2C地址對應LED12 ~ LED23
#define MCCVI_MPR121_C_ADD 0x5C //觸控I2C地址對應LED24 ~ LED35
#define MCCVI_MPR121_D_ADD 0x5D //未用，保留

#define MCCVI_IRQ_A 7  //MPR121_A用中斷腳
#define MCCVI_IRQ_B 8  //MPR121_B用中斷腳
#define MCCVI_IRQ_C 9  //MPR121_C用中斷腳
#define MCCVI_IRQ_D 10 //MPR121_D用中斷腳，保留

#define MCCVI_MPR121_TOUCH_THRESHOLD    12
#define MCCVI_MPR121_RELEASE_THRESHOLD  6

int mccvi_last_a_stat = 0;
int mccvi_current_a_stat = 0;
int mccvi_last_b_stat = 0;
int mccvi_current_b_stat = 0;
int mccvi_last_c_stat = 0;
int mccvi_current_c_stat = 0;

unsigned long MCCVI_LEDs_Timers[35] = {0};
byte MCCVI_LEDs[35]{
 52,//LED0
 50,//LED1
 48,//LED2
 46,//LED3
 44,//LED4
 42,//LED5
 40,//LED6
 38,//LED7
 36,//LED8
 34,//LED9
 32,//LED10
 30,//LED11
 28,//LED12
 26,//LED13
 24,//LED14
 22,//LED15
 53,//LED16
 51,//LED17
 49,//LED18
 47,//LED19
 45,//LED20
 43,//LED21
 41,//LED22
 39,//LED23
 37,//LED24
 35,//LED25
 33,//LED26
 31,//LED27
 29,//LED28
 27,//LED29
 25,//LED30
 23,//LED31
 6,//LED32
 5,//LED33
 4,//LED34
};

void mccvi_mpr121writereg8(int addr, byte reg, byte val){
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

byte mccvi_mpr121readreg8(int addr, byte reg){
  byte p_buff = 0;
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  delayMicroseconds(10);
  Wire.requestFrom(addr, 1);
  while(Wire.available() < 1);
  p_buff = Wire.read();
  return p_buff;
}

int mccvi_mpr121readreg16(int addr, byte reg){
  int p_buff[3] = {0};
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  delayMicroseconds(10);
  Wire.requestFrom(addr, 1);
  while(Wire.available() < 1);
  p_buff[0] = Wire.read();
  Wire.beginTransmission(addr);
  Wire.write(reg+1);
  Wire.endTransmission(false);
  delayMicroseconds(10);
  Wire.requestFrom(addr, 1);
  while(Wire.available() < 1);
  p_buff[1] = Wire.read();
  p_buff[2] = (p_buff[1] << 8) | p_buff[0];
  return p_buff[2];
}

void mccvi_mpr121init(int addr){
  mccvi_mpr121writereg8(addr, 0x80, 0x63);
  delay(10);
  mccvi_mpr121writereg8(addr, 0x5E, 0x00);
  if(mccvi_mpr121readreg8(addr, 0x5D)!= 0x24){
    Serial.println("初始化失敗");
  }
  for(byte y = 0; y < 12; y++){
    mccvi_mpr121writereg8(addr, (0x41 + (2 * y)), MCCVI_MPR121_TOUCH_THRESHOLD);
    mccvi_mpr121writereg8(addr, (0x42 + (2 * y)), MCCVI_MPR121_RELEASE_THRESHOLD);
  }
  mccvi_mpr121writereg8(addr, 0x2B, 0x01);
  mccvi_mpr121writereg8(addr, 0x2C, 0x01);
  mccvi_mpr121writereg8(addr, 0x2D, 0x0E);
  mccvi_mpr121writereg8(addr, 0x2E, 0x00);
  mccvi_mpr121writereg8(addr, 0x2F, 0x01);
  mccvi_mpr121writereg8(addr, 0x30, 0x05);
  mccvi_mpr121writereg8(addr, 0x31, 0x01);
  mccvi_mpr121writereg8(addr, 0x32, 0x00);
  mccvi_mpr121writereg8(addr, 0x33, 0x00);
  mccvi_mpr121writereg8(addr, 0x34, 0x00);
  mccvi_mpr121writereg8(addr, 0x35, 0x00);
  mccvi_mpr121writereg8(addr, 0x5B, 0x00);
  mccvi_mpr121writereg8(addr, 0x5C, 0x10);
  mccvi_mpr121writereg8(addr, 0x5D, 0x20);
  mccvi_mpr121writereg8(addr, 0x5E, 0x8C);
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  for(byte y = 0; y < 35; y++){
    pinMode(MCCVI_LEDs[y],HIGH);
  }
  mccvi_mpr121init(MCCVI_MPR121_A_ADD);
  mccvi_mpr121init(MCCVI_MPR121_B_ADD);
  mccvi_mpr121init(MCCVI_MPR121_C_ADD);
}

void loop() {
  unsigned long mccvi_nowtime = 0;
  mccvi_current_a_stat = mccvi_mpr121readreg16(MCCVI_MPR121_A_ADD, 0x00);
  mccvi_current_b_stat = mccvi_mpr121readreg16(MCCVI_MPR121_B_ADD, 0x00);
  mccvi_current_c_stat = mccvi_mpr121readreg16(MCCVI_MPR121_C_ADD, 0x00);
  mccvi_nowtime = millis();
  for(byte y = 0; y < 12; y++){
    if ((mccvi_current_a_stat & _BV(y)) && !(mccvi_last_a_stat & _BV(y)) ){ //a touched
      digitalWrite(MCCVI_LEDs[y], HIGH);
    }
    if (!(mccvi_current_a_stat & _BV(y)) && (mccvi_last_a_stat & _BV(y)) ){ //a released
      MCCVI_LEDs_Timers[y] = millis();
    }
    if ((mccvi_current_b_stat & _BV(y)) && !(mccvi_last_b_stat & _BV(y)) ){ //b touched
      digitalWrite(MCCVI_LEDs[y+12], HIGH);
    }
    if (!(mccvi_current_b_stat & _BV(y)) && (mccvi_last_b_stat & _BV(y)) ){ //b released
      MCCVI_LEDs_Timers[y+12] = millis();
    }
    if ((mccvi_current_c_stat & _BV(y)) && !(mccvi_last_c_stat & _BV(y)) ){ //c touched
      if(y < 11){ //lack 36s LED
        digitalWrite(MCCVI_LEDs[y+24], HIGH);
      }
    }
    if (!(mccvi_current_c_stat & _BV(y)) && (mccvi_last_c_stat & _BV(y)) ){ //c released
      if(y < 11){ //lack 36s LED
        MCCVI_LEDs_Timers[y+24] = millis();
      }
    }
    if(mccvi_nowtime - MCCVI_LEDs_Timers[y] > 3000){
      digitalWrite(MCCVI_LEDs[y], LOW);
    }
    if(mccvi_nowtime - MCCVI_LEDs_Timers[y+12] > 3000){
      digitalWrite(MCCVI_LEDs[y+12], LOW);
    }
    if(mccvi_nowtime - MCCVI_LEDs_Timers[y+24] > 3000){
      digitalWrite(MCCVI_LEDs[y+24], LOW);
    }
  }
  mccvi_last_a_stat = mccvi_current_a_stat;
  mccvi_last_b_stat = mccvi_current_b_stat;
  mccvi_last_c_stat = mccvi_current_c_stat;
}
