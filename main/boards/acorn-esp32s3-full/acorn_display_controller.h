#pragma once
#include "../../display/lcd_display.h"
#include <esp_timer.h>
#include <map>
#include <vector>
#include <string>
#include <functional>

// 设备状态枚举
enum class DeviceDisplayState {
    IDLE, LISTENING, THINKING, SPEAKING, 
    VOLUME_UP, VOLUME_DOWN, WIFI_CONNECTING, 
    WIFI_CONNECTED, BATTERY_LOW, ERROR
};

// 简化的显示配置
struct DisplayConfig {
    std::vector<const char*> gif_names;  // 支持多个 GIF 随机选择
    const char* status_icon;             // 上方状态栏图标
    const char* bottom_content;          // 底部图标内容
    const char* bottom_text;             // 底部文字
    bool auto_return_to_idle;
    int timeout_ms;
    bool random_gif;                     // 是否随机选择 GIF
};

class AcornDisplayController {
public:
    AcornDisplayController(SpiLcdDisplay* display);
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
    // 撤回 UpdateDisplay() 中的过渡效果
    void UpdateDisplay();
    
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
    
    SpiLcdDisplay* display_;  // 改为 SpiLcdDisplay*
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

private:
    // 删除这些过渡相关的成员和方法
    // std::function<void()> fade_out_callback_;
    // void SetGifWithFadeTransition(const lv_image_dsc_t* gif, const char* gif_name);
    // void OnFadeOutComplete();
    
    // ... 保留其他现有成员 ...
};
