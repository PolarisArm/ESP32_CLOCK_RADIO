#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <string.h>
#include "Squareline/ui.h"
#include "esp_heap_caps.h"
#include <LittleFS.h>
#include <Wire.h>
#include "Clock.h"
#include "Alarm.h"

#define LEFT 33
#define RIGHT 25

#define ENTER 26  // ADD THIS - you need a 3rd button for clicking
#define ADC 34
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define DRAW_BUFSIZE ((SCREEN_HEIGHT * SCREEN_WIDTH) / 10)
#define FORMAT_LITTLEFS_IF_FAILED true



uint8_t SEC = 0;
uint8_t MIN = 18;
uint8_t HOUR = 1;
uint8_t Day = 2;
uint8_t Date = 15;
uint8_t Month = 2;
uint8_t Year = 26;
uint8_t AMPM = 0;

uint16_t* drawBuf;
int current_screen = 0;

static lv_timer_t* timer;
static lv_timer_t* clock_timer;

static char Hoursbuf[4];
static char Minsbuf[4];
static char Secsbuf[4];
static char tempBuf[8];



TFT_eSPI tft = TFT_eSPI(SCREEN_HEIGHT,SCREEN_WIDTH);
uint16_t calData[5] = { 364, 3452, 302, 3423, 7 };

lv_obj_t* alarmContainerLabelArr[5];
uint8_t alarmLabel = 0;

Clock clk = Clock();
RTC_CLOCK rtc;
Alarm alarmClock;

// Function Prototypes
void log_print(lv_log_level_t level, const char* buf);
void my_disp_flush (lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap);
void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);

static void event_handler(lv_event_t* e);
static void clock_cb(lv_timer_t* timer);


static void timer_cb(lv_timer_t* timer){

    static int angle = 0;

    if(angle > 100){angle = 0;}

    if(ui_mainScreen){
            
            lv_label_set_text_fmt(ui_tempLabel,"%d°C",angle);
            lv_arc_set_value(ui_tempArc,angle);

        }
   

    angle++;
}


static void delete_label_event_handler(lv_event_t *e){
    lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
    int alarm_to_kill = (int)(intptr_t)lv_obj_get_user_data(obj);
    alarmClock.removeAlarmClock(alarm_to_kill);

    lv_obj_del_async(obj);

    if(alarmLabel > 0){
        alarmLabel--;
        Serial.println("Alarm Label removed");
    }
}


lv_obj_t* AddLabel(const char* name){

    lv_obj_t* label = lv_label_create(ui_alarmContainer);

    lv_obj_set_width(label, 80);
    lv_obj_set_height(label, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(label, -73);
    lv_obj_set_y(label, 1);
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_color(label, lv_color_hex(0x446F00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(label, lv_color_hex(0xF7F5F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_flag(label,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label,delete_label_event_handler,LV_EVENT_CLICKED,NULL);


    return label;

}



void setup() {
    Serial.begin(115200);
    Wire.begin();
    pinMode(ADC,INPUT);
    analogReadResolution(12);
    randomSeed(analogRead(ADC));

    tft.begin();
    tft.initDMA(); // MUST initialize DMA
    tft.setRotation(1);
    tft.setTouch(calData);

    lv_init();

    drawBuf = (uint16_t*)heap_caps_malloc(DRAW_BUFSIZE * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    static lv_display_t* disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_buffers(disp, drawBuf, NULL, DRAW_BUFSIZE * sizeof(uint16_t), LV_DISP_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, my_disp_flush);

    static lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    lv_indev_set_display(indev, disp);
    ui_init();

    lv_obj_add_event_cb(ui_hourroller,event_handler,LV_EVENT_VALUE_CHANGED,NULL);
    lv_obj_add_event_cb(ui_minroller,event_handler,LV_EVENT_VALUE_CHANGED,NULL);
    lv_obj_add_event_cb(ui_secroller,event_handler,LV_EVENT_VALUE_CHANGED,NULL);
    lv_obj_add_event_cb(ui_dateroller,event_handler,LV_EVENT_VALUE_CHANGED,NULL);
    lv_obj_add_event_cb(ui_monroller,event_handler,LV_EVENT_VALUE_CHANGED,NULL);
    lv_obj_add_event_cb(ui_yearroller,event_handler,LV_EVENT_VALUE_CHANGED,NULL);
    lv_obj_add_event_cb(ui_DateTimeOkBtn,event_handler,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(ui_alarmSet,event_handler,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(ui_alarmReset,event_handler,LV_EVENT_CLICKED,NULL);

    timer = lv_timer_create(timer_cb, 1000, NULL);
    clock_timer = lv_timer_create(clock_cb, 1000, NULL);

    lv_calendar_set_today_date(ui_calendar,2026,2,14);
    lv_calendar_set_showed_date(ui_calendar,2026,2);
    lv_obj_set_style_bg_color(ui_calendar,lv_color_hex(0x282828),LV_PART_MAIN);
    lv_obj_set_style_border_color(ui_calendar,lv_color_hex(0x444444),LV_PART_MAIN);
    lv_obj_set_style_border_width(ui_calendar,2,LV_PART_MAIN);
    lv_obj_set_style_radius(ui_calendar,10,LV_PART_MAIN);

    lv_obj_t* calendar_btnm = lv_calendar_get_btnmatrix(ui_calendar);
    lv_obj_set_style_bg_opa(calendar_btnm,LV_OPA_TRANSP,LV_PART_ITEMS);
    lv_obj_set_style_text_color(calendar_btnm,lv_color_white(),LV_PART_ITEMS);
    lv_obj_set_style_bg_color(calendar_btnm, lv_palette_main(LV_PALETTE_BLUE), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(calendar_btnm, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(calendar_btnm, lv_palette_main(LV_PALETTE_GREEN), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(calendar_btnm, 2, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(calendar_btnm, lv_color_hex(0x888888), LV_PART_ITEMS | LV_STATE_DISABLED);
   // initializeClock();
    Serial.println("Setup complete!");
}


void loop() {
    

    
   
    static uint32_t last_tick = millis();
    if(millis() - last_tick > 5) {
        lv_tick_inc(millis() - last_tick);
        last_tick = millis();
        lv_timer_handler();
    }
}


void log_print(lv_log_level_t level, const char* buf){
    Serial.println(buf);
}

void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    uint16_t x, y;

    if(tft.getTouch(&x, &y)) {

        // Map raw touch to LVGL landscape resolution
        data->point.x = x;
        data->point.y = y;

        if(data->point.x >= SCREEN_WIDTH)  data->point.x = SCREEN_WIDTH - 1;
        if(data->point.y >= SCREEN_HEIGHT) data->point.y = SCREEN_HEIGHT - 1;

        Serial.printf("Mapped: %d %d\n", data->point.x, data->point.y);

        data->state = LV_INDEV_STATE_PR;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
    }
}


static void event_handler(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target_obj(e);

    if(code == LV_EVENT_VALUE_CHANGED){
        char buf[32];

        if(obj == ui_hourroller){
            lv_roller_get_selected_str(obj,buf,sizeof(buf));
            LV_LOG_USER("Selected Data roller 1: %s \n",buf);
        }
        if(obj == ui_minroller){
            lv_roller_get_selected_str(obj,buf,sizeof(buf));
            LV_LOG_USER("Selected Data roller 2: %s \n",buf);
        }

        if(obj == ui_secroller){
            lv_roller_get_selected_str(obj,buf,sizeof(buf));
            LV_LOG_USER("Selected Data roller 3: %s \n",buf);
        }

        if(obj == ui_dateroller){
            lv_roller_get_selected_str(obj,buf,sizeof(buf));
            LV_LOG_USER("Selected Data roller 4: %s \n",buf);
        }

        if(obj == ui_monroller){
            lv_roller_get_selected_str(obj,buf,sizeof(buf));
            LV_LOG_USER("Selected Data roller 5: %s \n",buf);
        }

        if(obj == ui_yearroller){
            lv_roller_get_selected_str(obj,buf,sizeof(buf));
            LV_LOG_USER("Selected Data roller 6: %s \n",buf);
        }

       
    }

    if(code == LV_EVENT_CLICKED){

        if(obj == ui_DateTimeOkBtn){
            char bufAMPM[8];
            char bufsec[8];
            char bufmin[8];
            char bufhour[8];
            char bufdate[8];
            char bufmon[8];
            char bufyear[8];


            lv_dropdown_get_selected_str(ui_ampm,bufAMPM,sizeof(bufAMPM));
            lv_roller_get_selected_str(ui_dateroller,bufdate,sizeof(bufdate));
            lv_roller_get_selected_str(ui_monroller,bufmon,sizeof(bufmon));
            lv_roller_get_selected_str(ui_yearroller,bufyear,sizeof(bufyear));
            lv_roller_get_selected_str(ui_hourroller,bufhour,sizeof(bufhour));
            lv_roller_get_selected_str(ui_minroller,bufmin,sizeof(bufmin));
            lv_roller_get_selected_str(ui_secroller,bufsec,sizeof(bufsec));

           // int AMPM; //= atoi(bufAMPM);
            if(strcasecmp(bufAMPM,"AM") == 0){
                rtc.amorpm = 0;
            }else{
                rtc.amorpm = 1;
            }

            rtc.date = atoi(bufdate);
            rtc.month = atoi(bufmon);
            rtc.year = Century + atoi(bufyear);
            rtc.hr  = atoi(bufhour);
            rtc.min = atoi(bufmin);
            rtc.sec = atoi(bufsec);
            
            rtc.day = clk.getDayCode(rtc.date,rtc.month,rtc.year);
            
            LV_LOG_USER("%d %d %d %d %d %d %d %d\n",rtc.amorpm,rtc.date,rtc.day,rtc.month,rtc.year,rtc.hr,rtc.min,rtc.sec);
            
            clk.setClock(rtc);


        }

        if(obj == ui_alarmSet){

            char alarmHourBuf[4];
            char alarmMinBuf[4];
            char alarmSecBuf[4];
            char alarmAMORPMBuf[4];
            char Buf[10];


            lv_roller_get_selected_str(ui_alarmHour,alarmHourBuf,sizeof(alarmHourBuf));
            lv_roller_get_selected_str(ui_alarmMinute,alarmMinBuf,sizeof(alarmMinBuf));
            lv_roller_get_selected_str(ui_alarmSecond,alarmSecBuf,sizeof(alarmSecBuf));
            lv_dropdown_get_selected_str(ui_ampm2,alarmAMORPMBuf,sizeof(alarmAMORPMBuf));

            uint8_t amorpm = 0;

            if(strcasecmp(alarmAMORPMBuf,"AM") == 0){
                amorpm = 0;
            }else{
                amorpm = 32;
            }

            Serial.printf("Alarm: %d %d %d %d\n",atoi(alarmHourBuf),atoi(alarmMinBuf),atoi(alarmSecBuf),amorpm);
            sprintf(Buf,"%d:%d:%d",atoi(alarmHourBuf),atoi(alarmMinBuf),atoi(alarmSecBuf));

            int idx = alarmClock.setAlarmClock(atoi(alarmHourBuf),atoi(alarmMinBuf),atoi(alarmSecBuf),amorpm); 

            if(idx == -1){Serial.println("Alarm is Full");}
            
            if(idx >= 0 && alarmLabel < 5){
                alarmContainerLabelArr[alarmLabel] = AddLabel(Buf);

                // Store index in label user data so delete knows which alarm to remove
                lv_obj_set_user_data(alarmContainerLabelArr[alarmLabel],(void*)(intptr_t)idx);
                alarmLabel++;
            }
            



        }

        if(obj == ui_alarmReset){
            alarmClock.resetAllAlarmClock();

            for(int i = 0; i < alarmLabel; i++){
                if(alarmContainerLabelArr[i] != NULL){
                    lv_obj_del_async(alarmContainerLabelArr[i]);
                    alarmContainerLabelArr[i] = NULL;
                }
            }

            alarmLabel = 0;
        }
    }

}

int al = 0;

static void clock_cb(lv_timer_t* timer){
    
    static int lastAMPMchange = -1;
    static int lastDatechange = -1;
       
    clk.read(rtc);
    int alarmCheck = alarmClock.checkAlarmClock(rtc);

    if(alarmCheck >= 0){al = 3;}
    if(al >= 0){Serial.println("ALARM"); al--;}
    int isPMORAM = rtc.amorpm;

    if(isPMORAM){
        Serial.printf("%d %d %d PM %d %d %d %d %d\n",rtc.sec,rtc.min,rtc.hr,rtc.day,rtc.date,rtc.month,rtc.year,isPMORAM);
    }else{
        Serial.printf("%d %d %d AM %d %d %d %d\n",rtc.sec,rtc.min,rtc.hr,rtc.day,rtc.date,rtc.month,rtc.year);

    }

    if(ui_mainScreen){
    
        lv_label_set_text_fmt(ui_Hours,"%02d",rtc.hr);
        lv_label_set_text_fmt(ui_Minutes,"%02d",rtc.min);
        lv_label_set_text_fmt(ui_Seconds,"%02d",rtc.sec);
        if(isPMORAM != lastAMPMchange){
            if(isPMORAM){
            lv_label_set_text_fmt(ui_amPmLabel,"%s","PM");

            }else{
            lv_label_set_text_fmt(ui_amPmLabel,"%s","AM");
            }        
            
            lastAMPMchange = isPMORAM;

        }

        if(rtc.date != lastDatechange){
        
            
            lv_label_set_text_fmt(ui_day,"%s",clk.getDay(rtc));
            lv_label_set_text_fmt(ui_monthDateYear,"%s %d,%d",clk.getMonth(rtc),rtc.date,rtc.year);

            
            lastDatechange = rtc.date;
        }
        

    }  

    
}



void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    // Swap bytes for RGB565 (needed for LVGL 9 + TFT_eSPI)
    lv_draw_sw_rgb565_swap(pixelmap, w * h);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    // Use DMA to push colors - returns instantly while transfer happens in background
    tft.pushPixelsDMA((uint16_t*)pixelmap, w * h);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}