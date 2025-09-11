#pragma once
#include <lvgl.h>
#include "acorn_assets.h"
#include <map>
#include <string>

namespace AcornAssets {

const lv_image_dsc_t* GetGif(const char* name) {
    static std::map<std::string, const lv_image_dsc_t*> gif_map;
    
    if (gif_map.empty()) {
        // 初始化映射表 - 使用实际的文件变量名作为键
        gif_map["idle_status_1"] = &idle_status_1;
        gif_map["idle_status_2"] = &idle_status_2;
        gif_map["idle_status_blink"] = &idle_status_blink;
        gif_map["listen_start"] = &listen_start;
        gif_map["listen_end"] = &listen_end;
        gif_map["talk_loop"] = &talk_loop;
        gif_map["talk_start"] = &talk_start;
        gif_map["sleep_loop"] = &sleep_loop;
        gif_map["switch_on"] = &switch_on;
        gif_map["switch_off"] = &switch_off;
    }
    
    auto it = gif_map.find(name);
    return (it != gif_map.end()) ? it->second : nullptr;
}

const lv_image_dsc_t* GetStatusIcon(const char* name) {
    static std::map<std::string, const lv_image_dsc_t*> status_map;
    
    if (status_map.empty()) {
        // 初始化映射表
        status_map["volume_high"] = &volume_high;
        status_map["volume_low"] = &volume_low;
        status_map["volume_mute"] = &volume_mute;
        status_map["battery_low"] = &battery_low;
        status_map["battery_full"] = &battery_full;
    }
    
    auto it = status_map.find(name);
    return (it != status_map.end()) ? it->second : nullptr;
}

const lv_image_dsc_t* GetBottomIcon(const char* name) {
    // 暂时返回 nullptr，因为还没有底部图标资源
    return nullptr;
}

}
