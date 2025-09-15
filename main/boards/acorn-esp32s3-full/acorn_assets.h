#pragma once
#include <lvgl.h>
#include <map>
#include <string>

// ===== GIF 资源声明 =====
// 空闲状态 GIF
extern const lv_img_dsc_t idle_1;
extern const lv_img_dsc_t idle_2;
extern const lv_img_dsc_t idle_blink;
extern const lv_img_dsc_t sleep_loop;

// 监听状态 GIF
extern const lv_img_dsc_t listen_start;
extern const lv_img_dsc_t listen_loop;
extern const lv_img_dsc_t listen_end;

// 说话状态 GIF
extern const lv_img_dsc_t talk_start;
extern const lv_img_dsc_t talk_loop;
extern const lv_img_dsc_t talk_end;

// 触摸状态 GIF
extern const lv_img_dsc_t touch_start;
extern const lv_img_dsc_t touch_loop;
extern const lv_img_dsc_t touch_end;

// 低电量状态 GIF
extern const lv_img_dsc_t lowbat_start;
extern const lv_img_dsc_t lowbat_loop;
extern const lv_img_dsc_t lowbat_end;

// 开关状态 GIF
extern const lv_img_dsc_t switch_on;
extern const lv_img_dsc_t switch_off;

// ===== 图标资源声明 =====
// 电池图标
extern const lv_image_dsc_t battery_full;
extern const lv_image_dsc_t battery_low;

// 音量图标
extern const lv_image_dsc_t volume_high;
extern const lv_image_dsc_t volume_low;
extern const lv_image_dsc_t volume_mute;

// WiFi 图标
extern const lv_image_dsc_t wifi_connected;
extern const lv_image_dsc_t wifi_not_connected;
extern const lv_image_dsc_t wifi_error;

// 蜂窝网络图标
extern const lv_image_dsc_t cellular_connected;
extern const lv_image_dsc_t cellular_not_connected;
extern const lv_image_dsc_t cellular_connected_error;

// 绑定状态图标
extern const lv_image_dsc_t bind_success;
extern const lv_image_dsc_t bind_error;

// 网络加载图标
extern const lv_image_dsc_t network_loading;

namespace AcornAssets {
    // 资源获取函数
    const lv_img_dsc_t* GetGif(const char* name);
    const lv_image_dsc_t* GetIcon(const char* name);
    
    // 资源映射表
    extern std::map<std::string, const lv_img_dsc_t*> gif_map;
    extern std::map<std::string, const lv_image_dsc_t*> icon_map;
    
    // 初始化资源映射表
    void InitializeAssets();
}
