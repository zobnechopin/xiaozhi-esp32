#pragma once
#include "acorn_display.h"  // 恢复使用 acorn_display.h
#include <esp_timer.h>
#include <map>
#include <vector>
#include <string>

// 设备状态枚举
enum class DeviceDisplayState {
    IDLE,           // 空闲状态
    LISTENING,      // 监听状态
    THINKING,       // 思考状态
    SPEAKING,       // 说话状态
    VOLUME_UP,      // 音量调高
    VOLUME_DOWN,    // 音量调低
    WIFI_CONNECTING,// WiFi连接中
    WIFI_CONNECTED, // WiFi已连接
    BATTERY_LOW,    // 电量低
    ERROR,          // 错误状态
};

// 显示配置结构
struct DisplayConfig {
    std::vector<const char*> gif_names;  // GIF名称数组
    const char* status_icon;             // 状态图标
    const char* bottom_content;          // 底部内容
    const char* bottom_text;             // 底部文字
    bool auto_return_to_idle;            // 是否自动返回IDLE
    int timeout_ms;                      // 超时时间（毫秒）
    bool random_gif;                     // 是否随机选择GIF
};

class AcornDisplayController {
public:
    AcornDisplayController(AcornDisplay* display);  // 恢复使用 AcornDisplay*
    ~AcornDisplayController();
    
    // 主要接口：设置设备状态
    void SetDeviceState(DeviceDisplayState state);
    
    // 兼容原有接口
    void SetEmotion(const char* emotion);
    
    // 临时显示（会自动返回之前状态）
    void ShowTemporary(const char* gif_name, const char* status_icon = nullptr, 
                      const char* bottom_content = nullptr, const char* bottom_text = nullptr,
                      int timeout_ms = 3000);
    
    // 强制返回IDLE状态
    void ForceIdle();
    
    // === 新增：独立区域控制方法 ===
    
    // 独立控制各个区域
    void SetStatusIcon(const char* icon_name);           // 独立控制状态栏图标
    void SetGifAnimation(const char* gif_name);          // 独立控制GIF区域
    void SetBottomText(const char* text);                // 独立控制底部文字
    void SetBottomIcon(const char* icon_name);           // 独立控制底部图标
    
    // 清除操作
    void ClearStatusIcon();                              // 清除状态栏图标
    void ClearBottomArea();                              // 清除底部区域
    void ClearAll();                                     // 清除所有覆盖
    
    // 临时覆盖（带自动清除）
    void SetStatusOverlay(const char* icon_name, int duration_ms = 0);  // 临时显示状态图标
    void SetBottomOverlay(const char* text, int duration_ms = 0);       // 临时显示底部文字
    
    // 组合控制
    void SetCustomDisplay(const char* gif_name = nullptr, 
                         const char* status_icon = nullptr,
                         const char* bottom_text = nullptr,
                         const char* bottom_icon = nullptr);
    
    // 查询当前状态
    DeviceDisplayState GetCurrentState() const { return current_state_; }
    bool HasStatusOverride() const { return !current_status_override_.empty(); }
    bool HasBottomOverride() const { return !current_bottom_text_override_.empty() || !current_bottom_icon_override_.empty(); }

private:
    void ApplyDisplayConfig(const DisplayConfig& config);
    void SetupStateMapping();
    void UpdateDisplay();  // 统一更新显示，考虑覆盖优先级
    
    // 辅助方法声明
    std::string GetCurrentGif(const DisplayConfig& config);
    std::string GetCurrentStatusIcon(const DisplayConfig& config);
    std::string GetCurrentBottomText(const DisplayConfig& config);
    std::string GetCurrentBottomIcon(const DisplayConfig& config);
    
    bool HasGifOverride() const;
    bool HasBottomTextOverride() const;
    bool HasBottomIconOverride() const;
    bool HasVolumeOverride() const;
    // 注意：HasStatusOverride() 已经在 public 部分定义了，不要重复声明
    
    // 定时器回调
    static void TimeoutCallback(void* arg);
    static void OverlayTimeoutCallback(void* arg);
    
    AcornDisplay* display_;  // 恢复使用 AcornDisplay*
    DeviceDisplayState current_state_;
    DeviceDisplayState previous_state_;
    esp_timer_handle_t timeout_timer_;
    esp_timer_handle_t overlay_timer_;  // 新增：覆盖层定时器
    
    // 状态映射表
    std::map<DeviceDisplayState, DisplayConfig> state_configs_;
    std::map<std::string, DeviceDisplayState> emotion_mapping_;
    
    // 新增：独立覆盖设置
    std::string current_status_override_;       // 当前状态图标覆盖
    std::string current_gif_override_;          // 当前GIF覆盖
    std::string current_bottom_text_override_;  // 当前底部文字覆盖
    std::string current_bottom_icon_override_;  // 当前底部图标覆盖
};
