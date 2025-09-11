#pragma once
#include "display/lcd_display.h"

class AcornDisplay : public SpiLcdDisplay {
public:
    AcornDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                int width, int height, int offset_x, int offset_y, 
                bool mirror_x, bool mirror_y, bool swap_xy);
    
    virtual ~AcornDisplay() = default;
    
    // 专门的UI控制方法
    void SetStatusIcon(const char* icon_name);           // 设置顶部状态图标
    void SetCenterGif(const char* gif_name);             // 设置中央GIF
    void SetBottomContent(const char* content_name);     // 设置底部内容（图片）
    void SetBottomText(const char* text);                // 设置底部文字
    
    // 清空方法
    void ClearStatusIcon();
    void ClearBottomContent();
    void ClearAll();

private:
    void SetupCustomLayout();
    
    // UI组件
    lv_obj_t* status_icon_;      // 顶部状态图标
    lv_obj_t* center_gif_;       // 中央GIF区域
    lv_obj_t* bottom_container_; // 底部容器
    lv_obj_t* bottom_image_;     // 底部图片
    lv_obj_t* bottom_label_;     // 底部文字
};
