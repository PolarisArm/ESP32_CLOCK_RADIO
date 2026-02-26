#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <Wire.h>

#define PM_BIT 0x20
#define AM_BIT 0x00
#define MODE_12H 0x40
#define MODE_24H 0x00
#define DS3231_ADDR 0x68  
#define MASK_BIT 0x1F //0001 1111
#define MASK_BIT_24H 0x3F // 0011 1111 -> AM/PM Bits gets used for 24Hr mode
#define Century 2000

struct RTC_CLOCK{
    uint8_t hr;
    uint8_t min;
    uint8_t sec;
    uint8_t amorpm;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    int     year;

};

class Clock{
    private:
        uint8_t MIN;
        uint8_t HR;        
        uint8_t SEC; 
        uint8_t AMORPM;

        uint8_t DAY; 

        uint8_t DD;
        uint8_t MM; 
        uint8_t YY;
        const char* DAY_ARR[8] = {"","Sat","Sun","Mon","Tues","Wed","Thu","Fri"};
        const char* MON_ARR[13] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    public:
    Clock(){}
    // uint8_t SEC = 1, uint8_t MIN = 1, uint8_t HR = 1, uint8_t DAY = 1,uint8_t DD = 1, uint8_t MM = 2, uint8_t YY = 26, uint8_t AMORPM = 1
    void setClock(RTC_CLOCK &rtc){
        
        this-> SEC = rtc.sec;
        this-> MIN = rtc.min;
        this-> HR =  rtc.hr;
        this-> DAY = rtc.day;
        this-> DD = rtc.date;
        this-> MM = rtc.month;
        this-> YY = rtc.year % 100;
        this-> AMORPM = rtc.amorpm;

        UpdateClock();

    }

    void UpdateClock(){
        uint8_t ampm;
        if(AMORPM == 0){
            ampm = AM_BIT;
        }else{
            ampm = PM_BIT;
        }
        byte hourWrite = DecToBcd(HR) & 0x1F;
        // 0x1F = 0001 1111 AND it with Hour it will clear the control bit. So Nothing goes there accidentally.

        hourWrite |= MODE_12H;
        
        Wire.beginTransmission(DS3231_ADDR);
        Wire.write(0);
        Wire.write(DecToBcd(SEC));
        Wire.write(DecToBcd(MIN));
        Wire.write(hourWrite | ampm);
        Wire.write(DecToBcd(DAY));
        Wire.write(DecToBcd(DD));
        Wire.write(DecToBcd(MM));
        Wire.write(DecToBcd(YY));
        Wire.endTransmission();

    }

    void read(RTC_CLOCK &rtc){
        Wire.beginTransmission(DS3231_ADDR);
        Wire.write(0);
        Wire.endTransmission();

        Wire.requestFrom(DS3231_ADDR,7);

        if(Wire.available() >= 7){
            rtc.sec = BcdToDec(Wire.read());
            rtc.min = BcdToDec(Wire.read());
            byte hoursReg = Wire.read();
            rtc.day = BcdToDec(Wire.read());
            rtc.date = BcdToDec(Wire.read());
            rtc.month = BcdToDec(Wire.read() & MASK_BIT); // 0x1F = 0001 1111 AND it with Month it will clear the control bit. So Nothing goes there accidentally.
            rtc.year =  Century + BcdToDec(Wire.read());

            int is12hrMode = hoursReg & MODE_12H;

            if(is12hrMode){
                rtc.amorpm = hoursReg & PM_BIT;
                rtc.hr = BcdToDec(hoursReg & MASK_BIT);
            }else{
                rtc.amorpm = 0;
                rtc.hr = BcdToDec(hoursReg & MASK_BIT_24H);
            }
        }
    }

    const char* getDay(RTC_CLOCK &rtc){

        return DAY_ARR[getDayCode(rtc.date,rtc.month,rtc.year)];
    }

    const char* getMonth(RTC_CLOCK &rtc){

        return MON_ARR[rtc.month];

    }



    byte BcdToDec(byte data){
        return(((data/16)*10)+(data%16));
    }

    byte DecToBcd(byte data){
        return(((data/10)*16))+(data%10);
    }



    int getDayCode(uint8_t date, uint8_t mon, int year){
        int monthcode[] = {0,1,4,4,0,2,5,0,3,6,1,4,6};

        int k = year % 100;          // Last two digit of year
        int j = year / 100;         // First two digit of century. For 2016 it is 20
        int cc = (3 - (j%4)) * 2;  // century code 
        int d = (date+monthcode[mon]+k+(k/4)+cc) % 7;
        if((year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) && mon < 3){
            d = (d-1 + 7)%7;
        }


        return d+1;
    }



};


#endif