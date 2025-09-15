#pragma once
#include <lvgl.h>
#include "acorn_assets.h"
#include <map>
#include <string>

// 只包含头文件，不包含 .c 文件
// 编译器会自动链接对应的 .c 文件

namespace AcornAssets {
    // 资源映射表
    std::map<std::string, const lv_img_dsc_t*> gif_map;
    std::map<std::string, const lv_image_dsc_t*> icon_map;
    
    // 初始化资源映射表
    void InitializeAssets() {
        // GIF 资源映射
        gif_map["idle_1"] = &idle_1;
        gif_map["idle_2"] = &idle_2;
        gif_map["idle_blink"] = &idle_blink;
        gif_map["sleep_loop"] = &sleep_loop;
        gif_map["listen_start"] = &listen_start;
        gif_map["listen_loop"] = &listen_loop;
        gif_map["listen_end"] = &listen_end;
        gif_map["talk_start"] = &talk_start;
        gif_map["talk_loop"] = &talk_loop;
        gif_map["talk_end"] = &talk_end;
        gif_map["touch_start"] = &touch_start;
        gif_map["touch_loop"] = &touch_loop;
        gif_map["touch_end"] = &touch_end;
        gif_map["lowbat_start"] = &lowbat_start;
        gif_map["lowbat_loop"] = &lowbat_loop;
        gif_map["lowbat_end"] = &lowbat_end;
        gif_map["switch_on"] = &switch_on;
        gif_map["switch_off"] = &switch_off;
        
        // 图标资源映射
        icon_map["battery_full"] = &battery_full;
        icon_map["battery_low"] = &battery_low;
        icon_map["volume_high"] = &volume_high;
        icon_map["volume_low"] = &volume_low;
        icon_map["volume_mute"] = &volume_mute;
        icon_map["wifi_connected"] = &wifi_connected;
        icon_map["wifi_not_connected"] = &wifi_not_connected;
        icon_map["wifi_error"] = &wifi_error;
        icon_map["cellular_connected"] = &cellular_connected;
        icon_map["cellular_not_connected"] = &cellular_not_connected;
        icon_map["cellular_connected_error"] = &cellular_connected_error;
        icon_map["bind_success"] = &bind_success;
        icon_map["bind_error"] = &bind_error;
        icon_map["network_loading"] = &network_loading;
    }
    
    // 资源获取函数
    const lv_img_dsc_t* GetGif(const char* name) {
        auto it = gif_map.find(name);
        return (it != gif_map.end()) ? it->second : nullptr;
    }
    
    const lv_image_dsc_t* GetIcon(const char* name) {
        auto it = icon_map.find(name);
        return (it != icon_map.end()) ? it->second : nullptr;
    }
}
