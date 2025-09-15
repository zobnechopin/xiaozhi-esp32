#include "acorn_display_controller.h"
#include "acorn_assets.h"
#include <esp_log.h>
#include <cstring>
#include <random>

#define TAG "AcornDisplayController"

AcornDisplayController::AcornDisplayController(AcornDisplay* display)
    : display_(display), current_state_(DeviceDisplayState::IDLE), 
      previous_state_(DeviceDisplayState::IDLE), timeout_timer_(nullptr), 
      overlay_timer_(nullptr) {
    
    // 初始化资源
    AcornAssets::InitializeAssets();
    
    SetupStateMapping();
    SetupEmotionMapping();
    SetupDeviceStateMapping();  // 新增：设置DeviceState映射
    
    // 创建超时定时器
    esp_timer_create_args_t timer_args = {
        .callback = TimeoutCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "display_timeout",
        .skip_unhandled_events = false,
    };
    esp_timer_create(&timer_args, &timeout_timer_);
    
    // 创建覆盖层定时器
    esp_timer_create_args_t overlay_args = {
        .callback = OverlayTimeoutCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "display_overlay",
        .skip_unhandled_events = false,
    };
    esp_timer_create(&overlay_args, &overlay_timer_);
    
    // 初始设置为IDLE状态
    SetDeviceState(DeviceDisplayState::IDLE);
}

AcornDisplayController::~AcornDisplayController() {
    if (timeout_timer_) {
        esp_timer_stop(timeout_timer_);
        esp_timer_delete(timeout_timer_);
    }
    
    if (overlay_timer_) {
        esp_timer_stop(overlay_timer_);
        esp_timer_delete(overlay_timer_);
    }
}

void AcornDisplayController::SetupStateMapping() {
    // 空闲状态 - 随机播放空闲动画，不显示顶部图标
    state_configs_[DeviceDisplayState::IDLE] = {
        .gif_names = {"idle_1", "idle_2", "idle_blink"},
        .status_icon = nullptr,  // IDLE状态不显示顶部图标
        .bottom_content = nullptr,
        .bottom_text = nullptr,
        .auto_return_to_idle = true,
        .timeout_ms = 10000,  // 增加到10秒，让动画播放更久
        .random_gif = true   // 保持随机，这样动画会循环播放
    };
    
    // 监听状态 - 显示聆听动画
    state_configs_[DeviceDisplayState::LISTENING] = {
        .gif_names = {"listen_start", "listen_loop"},
        .status_icon = "volume_high",
        .bottom_content = nullptr,
        .bottom_text = "正在聆听...",
        .auto_return_to_idle = false,
        .timeout_ms = 0,
        .random_gif = false
    };
    
    // 思考状态 - 显示思考动画
    state_configs_[DeviceDisplayState::THINKING] = {
        .gif_names = {"listen_end"},
        .status_icon = "network_loading",
        .bottom_content = nullptr,
        .bottom_text = "思考中...",
        .auto_return_to_idle = true,
        .timeout_ms = 10000,
        .random_gif = false
    };
    
    // 说话状态 - 显示说话动画
    state_configs_[DeviceDisplayState::SPEAKING] = {
        .gif_names = {"talk_start", "talk_loop"},
        .status_icon = "volume_high",
        .bottom_content = nullptr,
        .bottom_text = nullptr,
        .auto_return_to_idle = false,
        .timeout_ms = 0,
        .random_gif = false
    };
    
    // 音量调节状态
    state_configs_[DeviceDisplayState::VOLUME_UP] = {
        .gif_names = {"touch_start"},
        .status_icon = "volume_high",
        .bottom_content = nullptr,
        .bottom_text = "音量+",
        .auto_return_to_idle = true,
        .timeout_ms = 2000,
        .random_gif = false
    };
    
    state_configs_[DeviceDisplayState::VOLUME_DOWN] = {
        .gif_names = {"touch_start"},
        .status_icon = "volume_low",
        .bottom_content = nullptr,
        .bottom_text = "音量-",
        .auto_return_to_idle = true,
        .timeout_ms = 2000,
        .random_gif = false
    };
    
    // WiFi连接状态
    state_configs_[DeviceDisplayState::WIFI_CONNECTING] = {
        .gif_names = {"touch_loop"},
        .status_icon = "network_loading",
        .bottom_content = nullptr,
        .bottom_text = "连接WiFi...",
        .auto_return_to_idle = false,
        .timeout_ms = 0,
        .random_gif = false
    };
    
    state_configs_[DeviceDisplayState::WIFI_CONNECTED] = {
        .gif_names = {"touch_end"},
        .status_icon = "wifi_connected",
        .bottom_content = nullptr,
        .bottom_text = "WiFi已连接",
        .auto_return_to_idle = true,
        .timeout_ms = 3000,
        .random_gif = false
    };
    
    // 电池状态 - 显示低电量图标
    state_configs_[DeviceDisplayState::BATTERY_LOW] = {
        .gif_names = {"lowbat_start", "lowbat_loop", "lowbat_end"},
        .status_icon = "battery_low",        // 顶部显示电池图标
        .bottom_content = nullptr,           // 底部不显示图标，让文字显示
        .bottom_text = "电量不足",           // 底部显示文字
        .auto_return_to_idle = true,
        .timeout_ms = 5000,
        .random_gif = false
    };
    
    // 错误状态
    state_configs_[DeviceDisplayState::ERROR] = {
        .gif_names = {"switch_off"},
        .status_icon = "bind_error",
        .bottom_content = "bind_error",
        .bottom_text = "出现错误",
        .auto_return_to_idle = true,
        .timeout_ms = 5000,
        .random_gif = false
    };
}

void AcornDisplayController::SetupEmotionMapping() {
    // 情感映射到设备状态
    emotion_mapping_["idle"] = DeviceDisplayState::IDLE;
    emotion_mapping_["listening"] = DeviceDisplayState::LISTENING;
    emotion_mapping_["thinking"] = DeviceDisplayState::THINKING;
    emotion_mapping_["speaking"] = DeviceDisplayState::SPEAKING;
    emotion_mapping_["volume_up"] = DeviceDisplayState::VOLUME_UP;
    emotion_mapping_["volume_down"] = DeviceDisplayState::VOLUME_DOWN;
    emotion_mapping_["wifi_connecting"] = DeviceDisplayState::WIFI_CONNECTING;
    emotion_mapping_["wifi_connected"] = DeviceDisplayState::WIFI_CONNECTED;
    emotion_mapping_["battery_low"] = DeviceDisplayState::BATTERY_LOW;
    emotion_mapping_["error"] = DeviceDisplayState::ERROR;
}

// 新增：设置DeviceState到DeviceDisplayState的映射
void AcornDisplayController::SetupDeviceStateMapping() {
    device_state_mapping_[kDeviceStateIdle] = DeviceDisplayState::IDLE;
    device_state_mapping_[kDeviceStateListening] = DeviceDisplayState::LISTENING;
    device_state_mapping_[kDeviceStateSpeaking] = DeviceDisplayState::SPEAKING;
    device_state_mapping_[kDeviceStateConnecting] = DeviceDisplayState::WIFI_CONNECTING;
    device_state_mapping_[kDeviceStateWifiConfiguring] = DeviceDisplayState::WIFI_CONNECTING;
    device_state_mapping_[kDeviceStateStarting] = DeviceDisplayState::WIFI_CONNECTING;
    device_state_mapping_[kDeviceStateUpgrading] = DeviceDisplayState::WIFI_CONNECTING;
    device_state_mapping_[kDeviceStateActivating] = DeviceDisplayState::WIFI_CONNECTING;
    device_state_mapping_[kDeviceStateAudioTesting] = DeviceDisplayState::LISTENING;
    device_state_mapping_[kDeviceStateFatalError] = DeviceDisplayState::ERROR;
    
    // 注意：BATTERY_LOW状态由电源管理器直接控制，不通过Application状态
}

// 主要接口：设置设备显示状态
void AcornDisplayController::SetDeviceState(DeviceDisplayState state) {
    if (current_state_ == state) {
        return; // 状态没有变化，直接返回
    }
    
    ESP_LOGI(TAG, "Setting device state to: %d", static_cast<int>(state));
    
    // 保存之前的状态
    previous_state_ = current_state_;
    current_state_ = state;
    
    // 停止之前的定时器
    if (timeout_timer_) {
        esp_timer_stop(timeout_timer_);
    }
    
    // 应用新的显示配置
    auto it = state_configs_.find(state);
    if (it != state_configs_.end()) {
        ApplyDisplayConfig(it->second);
    } else {
        ESP_LOGW(TAG, "No configuration found for state: %d", static_cast<int>(state));
        // 如果没有找到配置，使用IDLE状态
        auto idle_it = state_configs_.find(DeviceDisplayState::IDLE);
        if (idle_it != state_configs_.end()) {
            ApplyDisplayConfig(idle_it->second);
        }
    }
}

// 新增：从DeviceState设置显示状态
void AcornDisplayController::SetDeviceState(DeviceState state) {
    auto it = device_state_mapping_.find(state);
    if (it != device_state_mapping_.end()) {
        SetDeviceState(it->second);
    } else {
        ESP_LOGW(TAG, "Unknown DeviceState: %d, falling back to idle", static_cast<int>(state));
        SetDeviceState(DeviceDisplayState::IDLE);
    }
}

void AcornDisplayController::SetEmotion(const char* emotion) {
    if (!emotion) {
        SetDeviceState(DeviceDisplayState::IDLE);
        return;
    }
    
    auto it = emotion_mapping_.find(emotion);
    if (it != emotion_mapping_.end()) {
        SetDeviceState(it->second);
    } else {
        ESP_LOGW(TAG, "Unknown emotion: %s, falling back to idle", emotion);
        SetDeviceState(DeviceDisplayState::IDLE);
    }
}

void AcornDisplayController::ApplyDisplayConfig(const DisplayConfig& config) {
    // 使用 SetCustomDisplay 统一设置所有显示区域
    std::string gif_name = GetCurrentGif(config);
    std::string status_icon = GetCurrentStatusIcon(config);
    std::string bottom_text = GetCurrentBottomText(config);
    std::string bottom_icon = GetCurrentBottomIcon(config);
    
    // 一行代码完成所有显示设置
    SetCustomDisplay(
        gif_name.empty() ? nullptr : gif_name.c_str(),
        status_icon.empty() ? nullptr : status_icon.c_str(),
        bottom_text.empty() ? nullptr : bottom_text.c_str(),
        bottom_icon.empty() ? nullptr : bottom_icon.c_str()
    );
    
    // 如果状态图标为空，明确清除状态图标
    if (status_icon.empty()) {
        ClearStatusIcon();
    }
}

std::string AcornDisplayController::GetCurrentGif(const DisplayConfig& config) {
    if (config.gif_names.empty()) {
        return "";
    }
    
    if (config.random_gif && config.gif_names.size() > 1) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, config.gif_names.size() - 1);
        return config.gif_names[dis(gen)];
    }
    
    return config.gif_names[0];
}

std::string AcornDisplayController::GetCurrentStatusIcon(const DisplayConfig& config) {
    if (HasStatusOverride()) {
        return current_status_override_;
    }
    return config.status_icon ? config.status_icon : "";
}

std::string AcornDisplayController::GetCurrentBottomText(const DisplayConfig& config) {
    if (HasBottomTextOverride()) {
        return current_bottom_text_override_;
    }
    return config.bottom_text ? config.bottom_text : "";
}

std::string AcornDisplayController::GetCurrentBottomIcon(const DisplayConfig& config) {
    if (HasBottomIconOverride()) {
        return current_bottom_icon_override_;
    }
    return config.bottom_content ? config.bottom_content : "";
}

// ===== 独立区域控制 =====
void AcornDisplayController::SetStatusIcon(const char* icon_name) {
    if (!icon_name) {
        ClearStatusIcon();
        return;
    }
    
    display_->SetStatusIcon(icon_name);  // 改为只传一个参数
    current_status_override_ = icon_name;
}

void AcornDisplayController::SetGifAnimation(const char* gif_name) {
    if (!gif_name) {
        // 如果没有指定 GIF，清除当前 GIF
        display_->ClearAll();
        return;
    }
    
    // 先清除所有显示内容，避免叠加
    display_->ClearAll();
    
    // 然后设置新的 GIF
    display_->SetCenterGif(gif_name);
    current_gif_override_ = gif_name;
}

void AcornDisplayController::SetBottomText(const char* text) {
    if (!text) {
        ClearBottomArea();
        return;
    }
    
    display_->SetBottomText(text);
    current_bottom_text_override_ = text;
    current_bottom_icon_override_.clear();
    
    // 不自动设置超时，让文字持续显示
    // 只有在明确调用 SetBottomOverlay 时才设置超时
}

void AcornDisplayController::SetBottomIcon(const char* icon_name) {
    if (!icon_name) {
        ClearBottomArea();
        return;
    }
    
    display_->SetBottomContent(icon_name);  // 改为 SetBottomContent
    current_bottom_icon_override_ = icon_name;
    current_bottom_text_override_.clear();
}

// ===== 清除操作 =====
void AcornDisplayController::ClearStatusIcon() {
    display_->ClearStatusIcon();
    current_status_override_.clear();
}

void AcornDisplayController::ClearBottomArea() {
    display_->ClearBottomContent();  // 改为 ClearBottomContent
    current_bottom_text_override_.clear();
    current_bottom_icon_override_.clear();
}

void AcornDisplayController::ClearAll() {
    display_->ClearAll();
    current_status_override_.clear();
    current_gif_override_.clear();
    current_bottom_text_override_.clear();
    current_bottom_icon_override_.clear();
}

// ===== 临时显示 =====
void AcornDisplayController::ShowTemporary(const char* gif_name, const char* status_icon, 
                                          const char* bottom_content, const char* bottom_text,
                                          int timeout_ms) {
    // 保存当前状态
    previous_state_ = current_state_;
    
    // 显示临时内容
    if (gif_name) {
        SetGifAnimation(gif_name);
    }
    
    if (status_icon) {
        SetStatusIcon(status_icon);
    }
    
    if (bottom_content) {
        SetBottomIcon(bottom_content);
    }
    
    if (bottom_text) {
        SetBottomText(bottom_text);
    }
    
    // 设置恢复定时器
    if (timeout_ms > 0) {
        esp_timer_start_once(timeout_timer_, timeout_ms * 1000);
    }
}

// 保留 SetCustomDisplay 方法实现
void AcornDisplayController::SetCustomDisplay(const char* gif_name, const char* status_icon,
                                             const char* bottom_text, const char* bottom_icon) {
    // 设置各个组件（nullptr表示不改变该区域）
    if (gif_name) {
        SetGifAnimation(gif_name);
    }
    
    if (status_icon) {
        SetStatusIcon(status_icon);
    }
    
    if (bottom_text) {
        SetBottomText(bottom_text);
    }
    
    if (bottom_icon) {
        SetBottomIcon(bottom_icon);
    }
}

void AcornDisplayController::ForceIdle() {
    SetDeviceState(DeviceDisplayState::IDLE);
}

// ===== 覆盖层控制 =====
void AcornDisplayController::SetStatusOverlay(const char* icon_name, int duration_ms) {
    if (!icon_name) return;
    
    SetStatusIcon(icon_name);
    
    if (duration_ms > 0) {
        esp_timer_start_once(overlay_timer_, duration_ms * 1000);
    }
}

void AcornDisplayController::SetBottomOverlay(const char* text, int duration_ms) {
    if (!text) return;
    
    SetBottomText(text);
    
    // 只有明确设置了 duration_ms > 0 时才启动超时定时器
    if (duration_ms > 0) {
        esp_timer_start_once(overlay_timer_, duration_ms * 1000);
    }
}

// ===== 状态查询 =====
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
    return current_status_override_ == "volume_high" || 
           current_status_override_ == "volume_low" || 
           current_status_override_ == "volume_mute";
}

// ===== 定时器回调 =====
void AcornDisplayController::TimeoutCallback(void* arg) {
    AcornDisplayController* controller = static_cast<AcornDisplayController*>(arg);
    if (controller) {
        // 清除所有覆盖，恢复到当前状态
        controller->ClearAll();
        controller->SetDeviceState(controller->current_state_);
    }
}

void AcornDisplayController::OverlayTimeoutCallback(void* arg) {
    AcornDisplayController* controller = static_cast<AcornDisplayController*>(arg);
    if (controller) {
        // 清除覆盖层，恢复到当前状态
        controller->ClearStatusIcon();
        controller->ClearBottomArea();
        controller->SetDeviceState(controller->current_state_);
    }
}