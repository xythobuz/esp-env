/*
 * ui.h
 *
 * ESP8266 / ESP32 Environmental Sensor
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <xythobuz@xythobuz.de> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you
 * think this stuff is worth it, you can buy me a beer in return.   Thomas Buck
 * ----------------------------------------------------------------------------
 */

#ifndef __UI_H__
#define __UI_H__

enum bathroom_light_states {
    BATH_LIGHT_OFF,
    BATH_LIGHT_NONE,
    BATH_LIGHT_BIG,
    BATH_LIGHT_SMALL,
};

struct ui_status {
    bool light_corner;
    bool light_workspace;
    bool light_kitchen;
    bool sound_amplifier;
    bool light_bench;
    bool light_pc;
    bool light_amp;
    bool light_box;
    enum bathroom_light_states bathroom_lights;
};

extern struct ui_status ui_status;

enum ui_state {
    UI_INIT = 0,
    UI_WIFI_CONNECT,
    UI_WIFI_CONNECTING,
    UI_WIFI_CONNECTED,
    UI_READY,
    UI_UPDATE,
};

void ui_init(void);
void ui_run(void);

void ui_progress(enum ui_state state);

#endif // __UI_H__
