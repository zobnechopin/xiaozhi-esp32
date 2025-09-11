#include "acorn_display_controller.h"
#include "acorn_assets.h"  // 添加这个包含
#include <esp_log.h>
#include <random>
#include <cstring>

#define TAG "AcornDisplayController"

AcornDisplayController::AcornDisplayController(AcornDisplay* display)
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
    state_configs_ = {
        {DeviceDisplayState::IDLE, {
            .gif_names = {"idle_status_1", "idle_status_2", "idle_status_blink"},
            .status_icon = nullptr,
            .bottom_content = nullptr,
            .bottom_text = nullptr,
            .auto_return_to_idle = false,
            .timeout_ms = 0,
            .random_gif = true  // 启用随机选择
        }},
        
        {DeviceDisplayState::LISTENING, {
            .gif_names = {"listen_start"},
            .status_icon = "microphone_active",
            .bottom_content = nullptr,
            .bottom_text = "正在聆听...",
            .auto_return_to_idle = false,
            .timeout_ms = 0,
            .random_gif = false
        }},
        
        {DeviceDisplayState::THINKING, {
            .gif_names = {"listen_end"},
            .status_icon = "brain_active",
            .bottom_content = nullptr,
            .bottom_text = "思考中...",
            .auto_return_to_idle = true,
            .timeout_ms = 10000,
            .random_gif = false
        }},
        
        {DeviceDisplayState::SPEAKING, {
            .gif_names = {"talk_loop"},
            .status_icon = "speaker_active",
            .bottom_content = nullptr,
            .bottom_text = nullptr,
            .auto_return_to_idle = false,
            .timeout_ms = 0,
            .random_gif = false
        }},
        
        {DeviceDisplayState::VOLUME_UP, {
            .gif_names = {"volume_up_animation"},
            .status_icon = "volume_high",
            .bottom_content = nullptr,
            .bottom_text = "音量+",
            .auto_return_to_idle = true,
            .timeout_ms = 2000,
            .random_gif = false
        }},
        
        {DeviceDisplayState::VOLUME_DOWN, {
            .gif_names = {"volume_down_animation"},
            .status_icon = "volume_low",
            .bottom_content = nullptr,
            .bottom_text = "音量-",
            .auto_return_to_idle = true,
            .timeout_ms = 2000,
            .random_gif = false
        }},
        
        {DeviceDisplayState::WIFI_CONNECTING, {
            .gif_names = {"wifi_connecting_animation"},
            .status_icon = "wifi_disconnected",
            .bottom_content = nullptr,
            .bottom_text = "连接中...",
            .auto_return_to_idle = false,
            .timeout_ms = 0,
            .random_gif = false
        }},
        
        {DeviceDisplayState::WIFI_CONNECTED, {
            .gif_names = {"notification_gif"},
            .status_icon = "wifi_connected",
            .bottom_content = nullptr,
            .bottom_text = "已连接",
            .auto_return_to_idle = true,
            .timeout_ms = 3000,
            .random_gif = false
        }},
        
        {DeviceDisplayState::BATTERY_LOW, {
            .gif_names = {"error_animation"},
            .status_icon = "battery_low",
            .bottom_content = nullptr,
            .bottom_text = "电量不足",
            .auto_return_to_idle = true,
            .timeout_ms = 5000,
            .random_gif = false
        }},
        
        {DeviceDisplayState::ERROR, {
            .gif_names = {"error_animation"},
            .status_icon = "info_icon",
            .bottom_content = nullptr,
            .bottom_text = "系统错误",
            .auto_return_to_idle = true,
            .timeout_ms = 5000,
            .random_gif = false
        }},
    };
    
    // 表情名称到状态的映射（兼容原有接口）
    emotion_mapping_ = {
        {"neutral", DeviceDisplayState::IDLE},
        {"happy", DeviceDisplayState::IDLE},
        {"listening", DeviceDisplayState::LISTENING},
        {"thinking", DeviceDisplayState::THINKING},
        {"speaking", DeviceDisplayState::SPEAKING},
        {"sleepy", DeviceDisplayState::IDLE},
    };
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
    SetDeviceState(DeviceDisplayState::IDLE);
}

void AcornDisplayController::TimeoutCallback(void* arg) {
    auto* controller = static_cast<AcornDisplayController*>(arg);
    controller->SetDeviceState(DeviceDisplayState::IDLE);
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
    
    // 更新中央 GIF
    std::string gif_name = GetCurrentGif(config);
    if (!gif_name.empty()) {
        const lv_image_dsc_t* gif = AcornAssets::GetGif(gif_name.c_str());
        if (gif) {
            display_->SetCenterGif(gif_name.c_str());  // 使用 AcornDisplay 的方法
            ESP_LOGI(TAG, "Set center GIF: %s", gif_name.c_str());
        }
    }
    
    // 更新状态图标
    std::string status_icon = GetCurrentStatusIcon(config);
    if (!status_icon.empty()) {
        display_->SetStatusIcon(status_icon.c_str());  // 使用 AcornDisplay 的方法
    }
    
    // 更新底部内容
    std::string bottom_text = GetCurrentBottomText(config);
    if (!bottom_text.empty()) {
        display_->SetBottomText(bottom_text.c_str());  // 使用 AcornDisplay 的方法
    }
}

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
        // 随机选择一个 GIF
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, config.gif_names.size() - 1);
        int index = dis(gen);
        return std::string(config.gif_names[index]);
    } else {
        // 使用第一个 GIF
        return std::string(config.gif_names[0]);
    }
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