#pragma once
#include <lvgl.h>

// 直接声明全局变量（C 兼容方式）
// GIF 资源
extern const lv_image_dsc_t idle_status_1;
extern const lv_image_dsc_t idle_status_2;
extern const lv_image_dsc_t idle_status_blink;
extern const lv_image_dsc_t listen_start;
extern const lv_image_dsc_t listen_end;
extern const lv_image_dsc_t talk_loop;
extern const lv_image_dsc_t talk_start;
extern const lv_image_dsc_t sleep_loop;
extern const lv_image_dsc_t switch_on;
extern const lv_image_dsc_t switch_off;

// 状态图标资源
extern const lv_image_dsc_t volume_high;
extern const lv_image_dsc_t volume_low;
extern const lv_image_dsc_t volume_mute;
extern const lv_image_dsc_t battery_low;
extern const lv_image_dsc_t battery_full;

namespace AcornAssets {
    const lv_image_dsc_t* GetGif(const char* name);
    const lv_image_dsc_t* GetStatusIcon(const char* name);
    const lv_image_dsc_t* GetBottomIcon(const char* name);
}
