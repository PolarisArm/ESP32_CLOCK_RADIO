#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>
#include "Clock.h"

#define MAX_ALARMS 5

struct AlarmEntry{
        uint8_t hour;
        uint8_t min;
        uint8_t sec;
        uint8_t amorpm;
        bool lastMatch = false;
        bool active = false;
};

class Alarm{
    private:
       
        AlarmEntry Alarms[MAX_ALARMS];
        bool repeatAlarm = false;
    public:

    Alarm()//uint8_t hour, uint8_t min, uint8_t sec,uint8_t amorpm)
    {
        for(int i = 0; i < MAX_ALARMS; i++){
            Alarms[i].lastMatch = false;
            Alarms[i].active = false;
        }
    }

    int setAlarmClock(uint8_t hour, uint8_t min, uint8_t sec, uint8_t amorpm){

        for(int i = 0; i < MAX_ALARMS; i++){ // This will set one alarm and then return
            if(Alarms[i].active == false){
                Alarms[i].hour = hour;
                Alarms[i].min  = min;
                Alarms[i].sec  = sec;
                Alarms[i].amorpm = amorpm;
                Alarms[i].active = true;
                Alarms[i].lastMatch = false;
                return i;
            }
        }

        return -1; // If full it will return -1
        
    }

    int checkAlarmClock(RTC_CLOCK &rtc){

        for(int i = 0; i < MAX_ALARMS; i++){

            if(Alarms[i].active == true)
            {
                bool match = (Alarms[i].hour == rtc.hr && 
                              Alarms[i].min == rtc.min && 
                              Alarms[i].sec == rtc.sec&& 
                              Alarms[i].amorpm == rtc.amorpm);

                if(repeatAlarm == false){
                    if(match && !Alarms[i].lastMatch){
                        Alarms[i].lastMatch = true;
                        return i;
                    }

                if(!match) Alarms[i].lastMatch = false;

                }else{
                    if(match){return i;}
                }
            }
           
        
        }
        
        return -1;
    }

    void removeAlarmClock(uint8_t index){
        if(index < 0 || index >= MAX_ALARMS)
            return;
        
        Alarms[index].active = false;
        Alarms[index].lastMatch = false;
        
    }

    void resetAllAlarmClock(){
        for(int i = 0; i < MAX_ALARMS; i++){
            Alarms[i].lastMatch = false;
            Alarms[i].active = false;
        }
    }

    void setAlarmRepeat(bool repeatAlarm){

        this->repeatAlarm = repeatAlarm;

    }
};


#endif