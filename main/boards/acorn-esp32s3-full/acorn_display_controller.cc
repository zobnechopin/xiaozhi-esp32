#include "acorn_display_controller.h"
#include "acorn_assets.h"  // 添加这个包含
#include <esp_log.h>
#include <random>
#include <cstring>
#include <functional>

#define TAG "AcornDisplayController"

AcornDisplayController::AcornDisplayController(SpiLcdDisplay* display)
    : display_(display), current_state_(DeviceDisplayState::IDLE), 
      previous_state_(DeviceDisplayState::IDLE), timeout_timer_(nullptr), 
      overlay_timer_(nullptr) {
    
    SetupStateMapping();
    
    // 创建超时定时器
    esp_timer_create_args_t timer_args = {
        .callback = TimeoutCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "display_timeout",
        .skip_unhandled_events = false,
    };
    esp_timer_create(&timer_args, &timeout_timer_);
    
    // 初始设置为IDLE状态
    SetDeviceState(DeviceDisplayState::IDLE);
}

AcornDisplayController::~AcornDisplayController() {
    if (timeout_timer_) {
        esp_timer_stop(timeout_timer_);
        esp_timer_delete(timeout_timer_);
    }
    
    // 添加覆盖定时器的清理
    if (overlay_timer_) {
        esp_timer_stop(overlay_timer_);
        esp_timer_delete(overlay_timer_);
    }
}

void AcornDisplayController::SetupStateMapping() {
    // 状态到显示配置的映射
    state_configs_[DeviceDisplayState::IDLE] = {
        .gif_names = {"idle_1", "sleep_loop"},
        .status_icon = nullptr,
        .bottom_content = nullptr,
        .bottom_text = nullptr,
        .auto_return_to_idle = true,
        .timeout_ms = 3000,  // 3 秒后重新随机选择
        .random_gif = true
    };
    
    state_configs_[DeviceDisplayState::LISTENING] = {
        .gif_names = {"listen_start"},
        .status_icon = "microphone_active",
        .bottom_content = nullptr,
        .bottom_text = "正在聆听...",
        .auto_return_to_idle = false,
        .timeout_ms = 0,
        .random_gif = false
    };
    
    state_configs_[DeviceDisplayState::THINKING] = {
        .gif_names = {"listen_end"},
        .status_icon = "brain_active",
        .bottom_content = nullptr,
        .bottom_text = "思考中...",
        .auto_return_to_idle = true,
        .timeout_ms = 10000,
        .random_gif = false
    };
    
    state_configs_[DeviceDisplayState::SPEAKING] = {
        .gif_names = {"talk_loop"},
        .status_icon = "speaker_active",
        .bottom_content = nullptr,
        .bottom_text = nullptr,
        .auto_return_to_idle = false,
        .timeout_ms = 0,
        .random_gif = false
    };
    
    state_configs_[DeviceDisplayState::VOLUME_UP] = {
        .gif_names = {"volume_up_animation"},
        .status_icon = "volume_high",
        .bottom_content = nullptr,
        .bottom_text = "音量+",
        .auto_return_to_idle = true,
        .timeout_ms = 2000,
        .random_gif = false
    };
    
    state_configs_[DeviceDisplayState::VOLUME_DOWN] = {
        .gif_names = {"volume_down_animation"},
        .status_icon = "volume_low",
        .bottom_content = nullptr,
        .bottom_text = "音量-",
        .auto_return_to_idle = true,
        .timeout_ms = 2000,
        .random_gif = false
    };
    
    state_configs_[DeviceDisplayState::WIFI_CONNECTING] = {
        .gif_names = {"wifi_connecting_animation"},
        .status_icon = "wifi_disconnected",
        .bottom_content = nullptr,
        .bottom_text = "连接中...",
        .auto_return_to_idle = false,
        .timeout_ms = 0,
        .random_gif = false
    };
    
    state_configs_[DeviceDisplayState::WIFI_CONNECTED] = {
        .gif_names = {"notification_gif"},
        .status_icon = "wifi_connected",
        .bottom_content = nullptr,
        .bottom_text = "已连接",
        .auto_return_to_idle = true,
        .timeout_ms = 3000,
        .random_gif = false
    };
    
    state_configs_[DeviceDisplayState::BATTERY_LOW] = {
        .gif_names = {"error_animation"},
        .status_icon = "battery_low",
        .bottom_content = nullptr,
        .bottom_text = "电量不足",
        .auto_return_to_idle = true,
        .timeout_ms = 5000,
        .random_gif = false
    };
    
    state_configs_[DeviceDisplayState::ERROR] = {
        .gif_names = {"error_animation"},
        .status_icon = "info_icon",
        .bottom_content = nullptr,
        .bottom_text = "系统错误",
        .auto_return_to_idle = true,
        .timeout_ms = 5000,
        .random_gif = false
    };
    
    // 表情名称到状态的映射（兼容原有接口）
    emotion_mapping_["neutral"] = DeviceDisplayState::IDLE;
    emotion_mapping_["happy"] = DeviceDisplayState::IDLE;
    emotion_mapping_["listening"] = DeviceDisplayState::LISTENING;
    emotion_mapping_["thinking"] = DeviceDisplayState::THINKING;
    emotion_mapping_["speaking"] = DeviceDisplayState::SPEAKING;
    emotion_mapping_["sleepy"] = DeviceDisplayState::IDLE;
}

void AcornDisplayController::SetDeviceState(DeviceDisplayState state) {
    if (current_state_ == state) return;
    
    ESP_LOGI(TAG, "State change: %d -> %d", (int)current_state_, (int)state);
    
    // 停止之前的定时器
    if (timeout_timer_) {
        esp_timer_stop(timeout_timer_);
    }
    
    previous_state_ = current_state_;
    current_state_ = state;
    
    // 应用新状态的显示配置
    auto it = state_configs_.find(state);
    if (it != state_configs_.end()) {
        ApplyDisplayConfig(it->second);
        
        // 如果需要自动返回IDLE，启动定时器
        if (it->second.auto_return_to_idle && it->second.timeout_ms > 0) {
            esp_timer_start_once(timeout_timer_, it->second.timeout_ms * 1000);
        }
    }
}

void AcornDisplayController::ApplyDisplayConfig(const DisplayConfig& config) {
    // 现在只负责启动定时器，显示更新交给UpdateDisplay
    UpdateDisplay();
    
    // 处理自动返回IDLE的定时器
    if (config.auto_return_to_idle && config.timeout_ms > 0) {
        esp_timer_stop(timeout_timer_);
        esp_timer_start_once(timeout_timer_, config.timeout_ms * 1000);
    }
}

void AcornDisplayController::SetEmotion(const char* emotion) {
    auto it = emotion_mapping_.find(emotion);
    if (it != emotion_mapping_.end()) {
        SetDeviceState(it->second);
    } else {
        ESP_LOGW(TAG, "Unknown emotion: %s", emotion);
    }
}

void AcornDisplayController::ShowTemporary(const char* gif_name, const char* status_icon, 
                                          const char* bottom_content, const char* bottom_text,
                                          int timeout_ms) {
    DisplayConfig temp_config = {
        .gif_names = {gif_name},
        .status_icon = status_icon,
        .bottom_content = bottom_content,
        .bottom_text = bottom_text,
        .auto_return_to_idle = true,
        .timeout_ms = timeout_ms,
        .random_gif = false
    };
    
    previous_state_ = current_state_;
    ApplyDisplayConfig(temp_config);
    
    if (timeout_ms > 0) {
        esp_timer_start_once(timeout_timer_, timeout_ms * 1000);
    }
}

void AcornDisplayController::ForceIdle() {
    ESP_LOGI(TAG, "ForceIdle called, current state: %d", (int)current_state_);
    
    // 强制刷新 IDLE 状态，即使当前已经是 IDLE
    if (current_state_ == DeviceDisplayState::IDLE) {
        // 直接更新显示和启动定时器
        UpdateDisplay();
        
        // 手动启动 IDLE 循环定时器
        auto it = state_configs_.find(DeviceDisplayState::IDLE);
        if (it != state_configs_.end() && it->second.auto_return_to_idle && it->second.timeout_ms > 0) {
            ESP_LOGI(TAG, "Starting IDLE timer for %d ms", it->second.timeout_ms);
            esp_timer_stop(timeout_timer_);  // 先停止之前的定时器
            esp_timer_start_once(timeout_timer_, it->second.timeout_ms * 1000);
        }
    } else {
        // 如果不是 IDLE 状态，使用正常的状态切换
        SetDeviceState(DeviceDisplayState::IDLE);
    }
}

void AcornDisplayController::TimeoutCallback(void* arg) {
    auto* controller = static_cast<AcornDisplayController*>(arg);
    ESP_LOGI(TAG, "TimeoutCallback triggered, current_state: %d", (int)controller->current_state_);
    
    if (controller->current_state_ == DeviceDisplayState::IDLE) {
        // 如果当前已经是 IDLE 状态，直接刷新显示（重新随机选择）
        ESP_LOGI(TAG, "IDLE timeout, refreshing GIF");
        controller->UpdateDisplay();
        
        // 重新启动定时器
        auto it = controller->state_configs_.find(DeviceDisplayState::IDLE);
        if (it != controller->state_configs_.end() && it->second.auto_return_to_idle) {
            ESP_LOGI(TAG, "Restarting IDLE timer for %d ms", it->second.timeout_ms);
            esp_timer_start_once(controller->timeout_timer_, it->second.timeout_ms * 1000);
        } else {
            ESP_LOGE(TAG, "Failed to restart timer - config not found or auto_return_to_idle is false");
        }
    } else {
        // 其他状态正常返回 IDLE
        ESP_LOGI(TAG, "Returning to IDLE from state: %d", (int)controller->current_state_);
        controller->SetDeviceState(DeviceDisplayState::IDLE);
    }
}

// === 独立区域控制方法实现 ===

void AcornDisplayController::SetStatusIcon(const char* icon_name) {
    if (icon_name && strlen(icon_name) > 0) {
        current_status_override_ = icon_name;
    } else {
        current_status_override_.clear();
    }
    UpdateDisplay();
}

void AcornDisplayController::SetGifAnimation(const char* gif_name) {
    if (gif_name && strlen(gif_name) > 0) {
        current_gif_override_ = gif_name;
    } else {
        current_gif_override_.clear();
    }
    UpdateDisplay();
}

void AcornDisplayController::SetBottomText(const char* text) {
    if (text && strlen(text) > 0) {
        current_bottom_text_override_ = text;
        current_bottom_icon_override_.clear();  // 文字和图标互斥
    } else {
        current_bottom_text_override_.clear();
    }
    UpdateDisplay();
}

void AcornDisplayController::SetBottomIcon(const char* icon_name) {
    if (icon_name && strlen(icon_name) > 0) {
        current_bottom_icon_override_ = icon_name;
        current_bottom_text_override_.clear();  // 文字和图标互斥
    } else {
        current_bottom_icon_override_.clear();
    }
    UpdateDisplay();
}

// === 清除操作实现 ===

void AcornDisplayController::ClearStatusIcon() {
    current_status_override_.clear();
    UpdateDisplay();
}

void AcornDisplayController::ClearBottomArea() {
    current_bottom_text_override_.clear();
    current_bottom_icon_override_.clear();
    UpdateDisplay();
}

void AcornDisplayController::ClearAll() {
    current_status_override_.clear();
    current_gif_override_.clear();
    current_bottom_text_override_.clear();
    current_bottom_icon_override_.clear();
    
    // 停止覆盖定时器
    if (overlay_timer_) {
        esp_timer_stop(overlay_timer_);
    }
    
    UpdateDisplay();
}

// === 临时覆盖实现 ===

void AcornDisplayController::SetStatusOverlay(const char* icon_name, int duration_ms) {
    SetStatusIcon(icon_name);
    
    if (duration_ms > 0) {
        // 创建覆盖定时器（如果还没有）
        if (!overlay_timer_) {
            esp_timer_create_args_t overlay_timer_args = {
                .callback = OverlayTimeoutCallback,
                .arg = this,
                .dispatch_method = ESP_TIMER_TASK,
                .name = "overlay_timeout",
                .skip_unhandled_events = false,
            };
            esp_timer_create(&overlay_timer_args, &overlay_timer_);
        }
        
        // 启动定时器
        esp_timer_stop(overlay_timer_);  // 先停止之前的定时器
        esp_timer_start_once(overlay_timer_, duration_ms * 1000);  // 转换为微秒
    }
}

void AcornDisplayController::SetBottomOverlay(const char* text, int duration_ms) {
    SetBottomText(text);
    
    if (duration_ms > 0) {
        // 复用同一个覆盖定时器
        if (!overlay_timer_) {
            esp_timer_create_args_t overlay_timer_args = {
                .callback = OverlayTimeoutCallback,
                .arg = this,
                .dispatch_method = ESP_TIMER_TASK,
                .name = "overlay_timeout",
                .skip_unhandled_events = false,
            };
            esp_timer_create(&overlay_timer_args, &overlay_timer_);
        }
        
        esp_timer_stop(overlay_timer_);
        esp_timer_start_once(overlay_timer_, duration_ms * 1000);
    }
}

// === 组合控制实现 ===

void AcornDisplayController::SetCustomDisplay(const char* gif_name, 
                                              const char* status_icon,
                                              const char* bottom_text,
                                              const char* bottom_icon) {
    // 设置各个组件（nullptr表示不改变该区域）
    if (gif_name) {
        SetGifAnimation(gif_name);
    }
    if (status_icon) {
        SetStatusIcon(status_icon);
    }
    if (bottom_text) {
        SetBottomText(bottom_text);
    } else if (bottom_icon) {
        SetBottomIcon(bottom_icon);
    }
}

// === 统一更新显示方法 ===

// 把 UpdateDisplay() 实现移回这里
void AcornDisplayController::UpdateDisplay() {
    if (!display_) return;
    
    ESP_LOGI(TAG, "Updating display for state: %d", (int)current_state_);
    
    // 获取当前状态的配置
    auto it = state_configs_.find(current_state_);
    if (it == state_configs_.end()) {
        ESP_LOGW(TAG, "No config found for state: %d", (int)current_state_);
        return;
    }
    
    const DisplayConfig& config = it->second;
    
    // 更新中央 GIF - 直接切换，无过渡效果
    std::string gif_name = GetCurrentGif(config);
    if (!gif_name.empty()) {
        const lv_image_dsc_t* gif = AcornAssets::GetGif(gif_name.c_str());
        if (gif) {
            display_->SetCustomGif(gif);  // 直接设置，不用过渡
            ESP_LOGI(TAG, "Set center GIF: %s", gif_name.c_str());
        }
    }
    
    // TODO: 状态图标和底部内容需要等 SpiLcdDisplay 添加相应方法
}

// 新增方法：带淡入淡出效果的 GIF 切换
// void AcornDisplayController::SetGifWithFadeTransition(const lv_image_dsc_t* gif, const char* gif_name) {
//     // 创建回调函数，捕获参数
//     fade_out_callback_ = [this, gif, gif_name]() {
//         // 切换 GIF
//         display_->SetCustomGif(gif);
//         ESP_LOGI(TAG, "Set center GIF: %s", gif_name);
        
//         // 获取 GIF 控件并创建淡入动画
//         lv_obj_t* gif_obj = display_->GetGifWidget();
//         if (gif_obj) {
//             lv_anim_t fade_in;
//             lv_anim_init(&fade_in);
//             lv_anim_set_var(&fade_in, gif_obj);
//             lv_anim_set_values(&fade_in, LV_OPA_TRANSP, LV_OPA_COVER);
//             lv_anim_set_time(&fade_in, 200);  // 200ms 淡入
//             lv_anim_set_path_cb(&fade_in, lv_anim_path_ease_in_out);
//             lv_anim_set_exec_cb(&fade_in, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
//             lv_anim_start(&fade_in);
//         }
//     };
    
//     lv_obj_t* gif_obj = display_->GetGifWidget();
    
//     if (gif_obj) {
//         // 创建淡出动画
//         lv_anim_t fade_out;
//         lv_anim_init(&fade_out);
//         lv_anim_set_var(&fade_out, gif_obj);
//         lv_anim_set_values(&fade_out, lv_obj_get_style_opa(gif_obj, 0), LV_OPA_TRANSP);
//         lv_anim_set_time(&fade_out, 200);  // 200ms 淡出
//         lv_anim_set_path_cb(&fade_out, lv_anim_path_ease_in_out);
//         lv_anim_set_exec_cb(&fade_out, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
//         lv_anim_set_ready_cb(&fade_out, [](lv_anim_t* a) {
//             // 淡出完成后调用存储的回调
//             auto* controller = static_cast<AcornDisplayController*>(a->user_data);
//             controller->OnFadeOutComplete();
//         });
//         lv_anim_set_user_data(&fade_out, this);
//         lv_anim_start(&fade_out);
//     } else {
//         // 如果无法获取控件，直接切换
//         display_->SetCustomGif(gif);
//         ESP_LOGI(TAG, "Set center GIF: %s", gif_name);
//     }
// }

// void AcornDisplayController::OnFadeOutComplete() {
//     // 执行存储的回调
//     if (fade_out_callback_) {
//         fade_out_callback_();
//         fade_out_callback_ = nullptr;  // 清除回调
//     }
// }

// === 新的定时器回调 ===

void AcornDisplayController::OverlayTimeoutCallback(void* arg) {
    auto* controller = static_cast<AcornDisplayController*>(arg);
    if (controller) {
        ESP_LOGI(TAG, "Overlay timeout, clearing overlays");
        
        // 清除所有覆盖（但保持状态）
        controller->current_status_override_.clear();
        controller->current_bottom_text_override_.clear();
        controller->current_bottom_icon_override_.clear();
        
        // 重新应用当前状态
        controller->UpdateDisplay();
    }
}

// 在 AcornDisplayController 类中添加这些私有方法的实现

std::string AcornDisplayController::GetCurrentGif(const DisplayConfig& config) {
    if (HasGifOverride()) {
        return current_gif_override_;
    }
    
    if (config.gif_names.empty()) {
        return "";
    }
    
    if (config.random_gif && config.gif_names.size() > 1) {
        // 避免连续选择相同的 GIF
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::string last_gif_name = "";  // 记录上次选择的 GIF
        
        std::uniform_int_distribution<> dis(0, config.gif_names.size() - 1);
        int index = dis(gen);
        std::string selected_gif = std::string(config.gif_names[index]);
        
        // 如果选中的 GIF 和上次相同，强制选择另一个
        if (selected_gif == last_gif_name && config.gif_names.size() > 1) {
            index = (index + 1) % config.gif_names.size();  // 选择下一个
            selected_gif = std::string(config.gif_names[index]);
        }
        
        last_gif_name = selected_gif;
        ESP_LOGI(TAG, "Random GIF selected: %s (index: %d, total: %d)", 
                 selected_gif.c_str(), index, config.gif_names.size());
        return selected_gif;
    }
    
    // 使用第一个 GIF
    ESP_LOGI(TAG, "Using first GIF: %s", config.gif_names[0]);
    return std::string(config.gif_names[0]);
}

std::string AcornDisplayController::GetCurrentStatusIcon(const DisplayConfig& config) {
    if (HasStatusOverride()) {
        return current_status_override_;
    }
    
    return config.status_icon ? std::string(config.status_icon) : "";
}

std::string AcornDisplayController::GetCurrentBottomText(const DisplayConfig& config) {
    if (HasBottomTextOverride()) {
        return current_bottom_text_override_;
    }
    
    return config.bottom_text ? std::string(config.bottom_text) : "";
}

std::string AcornDisplayController::GetCurrentBottomIcon(const DisplayConfig& config) {
    if (HasBottomIconOverride()) {
        return current_bottom_icon_override_;
    }
    
    return config.bottom_content ? std::string(config.bottom_content) : "";
}

// 添加这些辅助方法
bool AcornDisplayController::HasGifOverride() const {
    return !current_gif_override_.empty();
}

bool AcornDisplayController::HasBottomTextOverride() const {
    return !current_bottom_text_override_.empty();
}

bool AcornDisplayController::HasBottomIconOverride() const {
    return !current_bottom_icon_override_.empty();
}

bool AcornDisplayController::HasVolumeOverride() const {
    return current_status_override_ == "volume_mute" || 
           current_status_override_ == "volume_low" || 
           current_status_override_ == "volume_high";
}