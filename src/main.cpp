#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <movingAvg.h>

//データ分析デバック用
//#define DEBUG_TELEPLOT

#define MIC_PIN         32 
#define RESET_BTN_PIN   23
#define L_LED_PIN       18
#define R_LED_PIN       19

#define BASE_VALUE      1800
#define L_VALUE         80
#define H_VALUE         1000
#define M_COUNT         7

/*
//arduino NANO初期値 ADCの分解能がESP32より悪いため実用でない
#define MIC_PIN         A0 
#define BASE_VALUE      380
#define L_VALUE         10
#define H_VALUE         50
#define M_COUNT         10
*/

//グローバル変数

LiquidCrystal_I2C lcd(0x27,16,2);  // I2C addr, 16x2 character

int count;
int next_c;
bool flag;
bool flag_h;
unsigned long t;
double cps_max;

movingAvg pt_mav(10); 

bool L_LED;
bool R_LED;
bool reset_f;


//グローバル変数の初期化とLCD表示の初期化
void reset_score(){
    count = 0;
    next_c = M_COUNT;
    flag = true;
    flag_h = false;
    t = millis();
    cps_max = 0.0f;

    pt_mav.reset();

    L_LED = HIGH;
    R_LED = HIGH;
    reset_f = false;

    lcd.setCursor(0, 0); //LCD Row 0 and Column 0
    lcd.print("Count/s:  ----   ");
    lcd.setCursor(0, 1); //LCD Row 0 and Column 1
    lcd.print("MAX_C/s:  ----   ");

}

//LED表示のアップデート(カウントされると交互点灯)
void led_alternating(){
    if(L_LED){
        L_LED = LOW;
        R_LED = HIGH;
    }else{
        L_LED = HIGH;
        R_LED = LOW;
    }
}

//リセットスイッチが押された時の割り込み
void push_reset(){
   reset_f = true;
}

void setup() {
    Serial.begin(115200);

    //リセットスイッチ初期化
    pinMode(RESET_BTN_PIN, INPUT_PULLUP);
    attachInterrupt(RESET_BTN_PIN, push_reset, RISING);
    
    //LED初期化
    pinMode(L_LED_PIN,OUTPUT);
    pinMode(R_LED_PIN,OUTPUT);

    //lcd1602初期化 
    lcd.init();
    lcd.backlight();

    //移動平均クラスの初期化
    pt_mav.begin();
    
    //リセット(変数初期化・lcd初期化)
    reset_score();

}
  
void loop() {
    
    //リセットボタン押された
    if(reset_f){
        reset_score();
        delay(500); //チャタリング防止用Wait
    }
    
    digitalWrite(L_LED_PIN,L_LED);
    digitalWrite(R_LED_PIN,R_LED);

    int micin = 0;
    char s[100];  //sprintf用
    
    //5回サンプリングの平均値を利用。ADC実測値をBASE_LINEより反転し正の数値を取るように補正
    for(int i=0;i<5;i++){
      micin += abs(analogRead(MIC_PIN) - BASE_VALUE);
      delayMicroseconds(1500);
    }
    micin = micin / 5;

#ifdef DEBUG_TELEPLOT
    Serial.printf(">micin:%d\n", micin);
#endif

    if(flag){
        if(micin > L_VALUE){
            count++;
            //LED表示のアップデート
            led_alternating();

            if(t > 0){
                unsigned int pass_t = millis() - t;
                unsigned int pass_t_ave = pt_mav.reading(pass_t);

                double c_p_sec = 1000.0 / double(pass_t_ave);
                lcd.setCursor(10, 0);
                sprintf(s,"%0.2f  ", c_p_sec);
                lcd.print(s);  

                //カウンタが20を超えたら最大値の保持開始
                if ( count > 20 ){
                    if(c_p_sec > cps_max){
                        cps_max = c_p_sec;
                        lcd.setCursor(10, 1);
                        sprintf(s,"%0.2f  ", cps_max);
                        lcd.print(s);  
                    } 
                } 
                
                #ifdef DEBUG_TELEPLOT
                    Serial.printf(">pass_t_ave:%d\n", pass_t_ave);
                    Serial.printf(">pass_t:%d\n", pass_t);
                #endif
                t = millis();

            }
            flag = false;
        }
    }else{
        //H_VALUEを超えたらカウント
        if(micin > H_VALUE){
            if(!flag_h) flag = true;
            flag_h = true;
            
        }
        //L_VALUEを下回るまでカウントを止める
        if(flag_h && (micin < L_VALUE)){
            flag_h = false;
        }
    }
    
#ifdef DEBUG_TELEPLOT
    Serial.printf(">count:%d\n", count);
#endif

}

