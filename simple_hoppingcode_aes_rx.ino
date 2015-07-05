#include <avr/eeprom.h>
#include <LiquidCrystal.h>
#include <VirtualWire.h>
#include <AESLib.h>
const int SAFEWINDOW = 50;
long lastcount;
char *controller;
unsigned char msg[16];
uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
#pragma pack 1
//uint8_t key[16];
typedef struct {
    unsigned long serial;
    long counter;
    char command;
    long extra;
    int code;
    char termin;
} PackedData;
typedef struct {
    long counter;
    unsigned long serial;
    uint8_t skey[16];
} Settings;
typedef struct {
    long counter;
    unsigned long serial;
    int magic;
} PairingData;
// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
Settings StoredSettings;
int errorcounter=0;
int adc_key_in;
// read the buttons
int read_LCD_buttons()
{
    adc_key_in = analogRead(0); // read the value from the sensor
    if (adc_key_in < 50) return 1;// btnRIGHT;
    if (adc_key_in < 250) return 2;// btnUP;
    if (adc_key_in < 450) return 3;//btnDOWN;
    if (adc_key_in < 650) return 4;//btnLEFT;
    if (adc_key_in < 850) return 5;//btnSELECT;
    return 0;//btnNONE; // when all others fail, return this...
}
void setup()
{
    Serial.begin(9600);
    lcd.begin(16, 2); // start the library
    //memcpy (&StoredSettings.skey, &key[0], sizeof(key));
    //eeprom_write_block((const void*)&StoredSettings, (void*)0, sizeof(StoredSettings));
    delay(1000);               
    eeprom_read_block((void*)&StoredSettings, (void*)0, sizeof(StoredSettings));
    //memcpy (&key[0],&StoredSettings.skey, sizeof(key));
    Serial.print("EEPROMKEY:");
    for(int k=0;k<16;k++)
    Serial.print(StoredSettings.skey[k],HEX);
    Serial.println("");
    lcd.setCursor(0,1);
    lcd.print(StoredSettings.counter); // print a simple message
    lcd.setCursor(0,0);
    lcd.print("SN: "); // print a simple message
    lcd.setCursor(4,0);
    lcd.print(StoredSettings.serial);
    pinMode(2, OUTPUT);
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_set_rx_pin(A5);
    vw_setup(4000); // Bits per sec
    vw_rx_start();
}
void pairing(void)
{
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
    lcd.setCursor(0,0);
    lcd.print(" Pairing! "); 
    long start =millis();
    int ledState = LOW;
    long previousMillis = 0; // will store last time LED was updated
    do{
        //do pairing process and store serial and counter
        if (vw_get_message(buf, &buflen)) // Non-blocking
        {
        lcd.clear();
      
        PairingData * pairData = (PairingData *)buf; 
           
            if(pairData->magic==555){
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(pairData->serial);
                lcd.setCursor(0,1);
                lcd.print(pairData->counter);
                StoredSettings.counter=pairData->counter;
                StoredSettings.serial=pairData->serial;
                eeprom_write_block((const void*)&StoredSettings, (void*)0, sizeof(StoredSettings));
                delay(2000);
                lcd.clear();              
                lcd.setCursor(0,0);
                lcd.print(" Done!");
                lcd.setCursor(0,1);
                lcd.print(" Synced");
                delay(2000);
                break;
            }
            else
            {
                lcd.clear();            
                lcd.setCursor(0,0);
                lcd.print(" Error!");
                lcd.setCursor(0,1);
                lcd.print("Try again!");
            }
        }
        delay(100);
        //end of pairing
        unsigned long currentMillis = millis();
        if(currentMillis - previousMillis > 100) {
            previousMillis = currentMillis;
            if (ledState == LOW)
            ledState = HIGH;
            else
            ledState = LOW;
            digitalWrite(2, ledState);
        }
    } while ((millis()-start)<10000); //for 5 seconds
}
void loop()
{


   if (read_LCD_buttons()!=0)
    {
   pairing();
    }
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
    if (vw_get_message(buf, &buflen)) // Non-blocking
    {
        Serial.print("Encrypted:");
        memcpy(msg, buf, buflen);
        for(int k=0;k<16;k++)
        Serial.print(msg[k],HEX);
        Serial.println("");
        Serial.print("KEY:");
        for(int k=0;k<buflen;k++)
        Serial.print(key[k],HEX);
        Serial.println("");
        aes128_dec_single(key, msg);
        PackedData * sdData = (PackedData *)msg;
        Serial.print("Test Decrypt:");
        Serial.print(sdData->serial);
        Serial.println("");
        
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(sdData->serial);
        lcd.setCursor(0,1);
        lcd.print(sdData->counter);
        
        digitalWrite(2,1); //for red led
        long currentcounter;
        if(sdData->code==555){
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(sdData->serial);
            lcd.setCursor(0,1);
            lcd.print(sdData->counter);
            //do the job if the counter is with in safety window
            currentcounter= sdData->counter;
            if((currentcounter-StoredSettings.counter)<SAFEWINDOW && (currentcounter-StoredSettings.counter)>0)
            {
                lcd.setCursor(0,0);
                lcd.print("Access Granted!");
                StoredSettings.counter=sdData->counter;
                eeprom_write_block((const void*)&StoredSettings, (void*)0, sizeof(StoredSettings));
                //do the stuff here for eg running the servo or door lock motor
                char command = sdData->command;
                switch (command) {
                        case 'a':
                              lcd.clear();
                              lcd.setCursor(0,0);
                              lcd.print("OPENING DOOR"); 
                              //do the servo here                              
                              break;
                        case 'b':
                              lcd.clear();
                              lcd.setCursor(0,0);
                              lcd.print("CLOSING DOOR"); 
                              //do the servo here     
                              break;
                        case 'c':
                              lcd.clear();
                              lcd.setCursor(0,0);
                              lcd.print("NEXT THING"); 
                              //do the servo here     
                              break;                }
                
            }
          else
            {
                errorcounter++;
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Needs Pairing!");
                lcd.setCursor(8,1);
                lcd.print(errorcounter);
                lcd.setCursor(12,1);
                lcd.print(StoredSettings.counter);
                lcd.setCursor(0,1);
                lcd.print(currentcounter);               
            } 
        }
        else
        {
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.setCursor(0,0);
            lcd.print(" Error!");
            lcd.setCursor(0,1);
            lcd.print(" Wrong Key??");
        }
        delay(100);
        digitalWrite(2,0);
    }
    delay(100); //just here to slow down a bit

}
