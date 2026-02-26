SET(SOURCES ui_comp_circulararc.c
    ui_comp.c
    ui_mainScreen.c
    ui_calendarScreen.c
    ui_timeSettings.c
    ui_MenuScreen.c
    ui_settingsScreen.c
    ui_alarmScreen.c
    ui_RadioScreen.c
    ui_MusicScreen.c
    ui_WifiScreen.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_bgone_png.c
    ui_img_im_4_png.c
    ui_img_angleright_png.c
    ui_img_alarm_png.c
    ui_img_battery_0_png.c
    ui_img_angleleft_png.c
    ui_img_set_png.c
    ui_img_ok_png.c
    ui_img_radio_png.c
    ui_img_calendar_png.c
    ui_img_499794662.c
    ui_img_1725651404.c
    ui_img_clock_png.c
    ui_img_wifi_icon_png.c
    ui_img_music_png.c
    ui_img_inc_png.c
    ui_img_dec_png.c
    ui_img_sound_png.c
    ui_img_mute_png.c
    fonts/ui_font_Font2.c
    fonts/ui_font_FontTwo.c)

add_library(ui ${SOURCES})
