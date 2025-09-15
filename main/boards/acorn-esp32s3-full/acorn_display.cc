#include "acorn_display.h"
#include "acorn_assets.h"
#include <esp_log.h>
#include <libs/gif/lv_gif.h>
#include <cstring>

#define TAG "AcornDisplay"

AcornDisplay::AcornDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                          int width, int height, int offset_x, int offset_y, 
                          bool mirror_x, bool mirror_y, bool swap_xy)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy),
      status_icon_(nullptr), center_gif_(nullptr), bottom_container_(nullptr),
      bottom_image_(nullptr), bottom_label_(nullptr) {
    
    SetupCustomLayout();
}

void AcornDisplay::SetChatMessage(const char* role, const char* content) {
    // 如果是系统消息，显示在底部区域
    if (role && strcmp(role, "system") == 0) {
        ESP_LOGI(TAG, "System message received: %s", content ? content : "(empty)");
        SetBottomText(content);
        return;
    }
    
    // 对于其他类型的消息（user, assistant），调用父类的默认实现
    // 但由于我们使用的是 Halo UI，这些消息通常不会显示在聊天区域
    // 所以我们可以选择忽略它们，或者记录日志
    if (role && content) {
        ESP_LOGI(TAG, "Chat message [%s]: %s (ignored in Halo UI)", role, content);
    }
}

void AcornDisplay::SetupCustomLayout() {
    DisplayLockGuard lock(this);
    
    auto screen = lv_screen_active();
    
    ESP_LOGI(TAG, "Setting up custom layout...");
    
    // 创建自定义 UI 对象，让它们覆盖在默认 UI 之上
    status_icon_ = lv_img_create(screen);
    lv_obj_align(status_icon_, LV_ALIGN_TOP_MID, 0, 5);
    // 暂时不隐藏，方便调试
    // lv_obj_add_flag(status_icon_, LV_OBJ_FLAG_HIDDEN);
    
    // 移除调试边框，避免重复显示
    // lv_obj_set_style_border_color(status_icon_, lv_color_hex(0xFF0000), 0);  // 红色边框
    // lv_obj_set_style_border_width(status_icon_, 3, 0);
    // lv_obj_set_style_bg_opa(status_icon_, LV_OPA_20, 0);  // 添加半透明背景
    // lv_obj_set_style_bg_color(status_icon_, lv_color_hex(0x000000), 0);
    
    center_gif_ = lv_gif_create(screen);
    lv_obj_align(center_gif_, LV_ALIGN_CENTER, 0, -10);  // 向上移动10像素
    
    // 移除调试边框
    // lv_obj_set_style_border_color(center_gif_, lv_color_hex(0x00FF00), 0);  // 绿色边框
    // lv_obj_set_style_border_width(center_gif_, 3, 0);
    // lv_obj_set_style_bg_opa(center_gif_, LV_OPA_20, 0);  // 添加半透明背景
    // lv_obj_set_style_bg_color(center_gif_, lv_color_hex(0x000000), 0);
    
    // 创建底部容器
    bottom_container_ = lv_obj_create(screen);
    lv_obj_set_size(bottom_container_, LV_HOR_RES, 60);
    lv_obj_align(bottom_container_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(bottom_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottom_container_, 0, 0);
    lv_obj_set_style_pad_all(bottom_container_, 5, 0);
    
    // 添加明显的边框用于调试
    lv_obj_set_style_border_color(bottom_container_, lv_color_hex(0x0000FF), 0);  // 蓝色边框
    lv_obj_set_style_border_width(bottom_container_, 3, 0);
    
    // 创建底部图片
    bottom_image_ = lv_img_create(bottom_container_);
    lv_obj_align(bottom_image_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(bottom_image_, LV_OBJ_FLAG_HIDDEN);
    
    // 创建底部文字
    bottom_label_ = lv_label_create(bottom_container_);
    lv_obj_align(bottom_label_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(bottom_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(bottom_label_, lv_color_white(), 0);  // 设置文字颜色为白色
    lv_obj_set_style_text_font(bottom_label_, style_.text_font, 0);   // 使用样式中的字体
    lv_obj_add_flag(bottom_label_, LV_OBJ_FLAG_HIDDEN);
    
    ESP_LOGI(TAG, "Custom layout setup completed");
}

void AcornDisplay::SetStatusIcon(const char* icon_name) {
    DisplayLockGuard lock(this);
    
    if (!icon_name) {
        ClearStatusIcon();
        return;
    }
    
    // 使用现有的 GetIcon 方法，但需要类型转换
    const lv_image_dsc_t* icon = AcornAssets::GetIcon(icon_name);
    if (icon) {
        // 需要将 lv_image_dsc_t* 转换为 lv_img_dsc_t*
        lv_img_set_src(status_icon_, (const lv_img_dsc_t*)icon);
        lv_obj_remove_flag(status_icon_, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Set status icon: %s", icon_name);
    } else {
        ESP_LOGW(TAG, "Status icon not found: %s", icon_name);
        ClearStatusIcon();
    }
}

void AcornDisplay::SetCenterGif(const char* gif_name) {
    DisplayLockGuard lock(this);
    
    if (!gif_name) {
        // 如果没有指定 GIF，隐藏当前 GIF
        if (center_gif_) {
            lv_obj_add_flag(center_gif_, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    
    const lv_img_dsc_t* gif = AcornAssets::GetGif(gif_name);
    if (gif) {
        // 先显示 GIF 对象，然后设置新的源
        lv_obj_clear_flag(center_gif_, LV_OBJ_FLAG_HIDDEN);
        lv_gif_set_src(center_gif_, gif);
        ESP_LOGI(TAG, "Set center GIF: %s", gif_name);
    } else {
        ESP_LOGW(TAG, "GIF not found: %s", gif_name);
        // 如果找不到 GIF，隐藏当前显示
        if (center_gif_) {
            lv_obj_add_flag(center_gif_, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void AcornDisplay::SetBottomContent(const char* content_name) {
    DisplayLockGuard lock(this);
    
    if (!content_name) {
        ClearBottomContent();
        return;
    }
    
    // 使用现有的 GetIcon 方法，但需要类型转换
    const lv_image_dsc_t* icon = AcornAssets::GetIcon(content_name);
    if (icon) {
        // 需要将 lv_image_dsc_t* 转换为 lv_img_dsc_t*
        lv_img_set_src(bottom_image_, (const lv_img_dsc_t*)icon);
        lv_obj_remove_flag(bottom_image_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(bottom_label_, LV_OBJ_FLAG_HIDDEN);  // 隐藏文字
        ESP_LOGI(TAG, "Set bottom icon: %s", content_name);
    } else {
        ESP_LOGW(TAG, "Bottom icon not found: %s", content_name);
    }
}

void AcornDisplay::SetBottomText(const char* text) {
    DisplayLockGuard lock(this);
    
    if (!text || strlen(text) == 0) {
        ClearBottomContent();
        return;
    }
    
    lv_label_set_text(bottom_label_, text);
    lv_obj_remove_flag(bottom_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(bottom_image_, LV_OBJ_FLAG_HIDDEN);  // 隐藏图片
    ESP_LOGI(TAG, "Set bottom text: %s", text);
}

void AcornDisplay::ClearStatusIcon() {
    DisplayLockGuard lock(this);
    lv_obj_add_flag(status_icon_, LV_OBJ_FLAG_HIDDEN);
}

void AcornDisplay::ClearBottomContent() {
    DisplayLockGuard lock(this);
    lv_obj_add_flag(bottom_image_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(bottom_label_, LV_OBJ_FLAG_HIDDEN);
}

void AcornDisplay::SetCustomGif(const lv_image_dsc_t* gif_src) {
    if (!center_gif_ || !gif_src) return;
    
    DisplayLockGuard lock(this);
    lv_gif_set_src(center_gif_, gif_src);
    
    // 重新应用绿色边框样式，因为 lv_gif_set_src 会重置样式
    lv_obj_set_style_border_color(center_gif_, lv_color_hex(0x00FF00), 0);  // 绿色边框
    lv_obj_set_style_border_width(center_gif_, 3, 0);
    lv_obj_set_style_bg_opa(center_gif_, LV_OPA_20, 0);  // 添加半透明背景
    lv_obj_set_style_bg_color(center_gif_, lv_color_hex(0x000000), 0);
    
    ESP_LOGI(TAG, "Custom GIF updated with green border");
}

void AcornDisplay::ClearCenterGif() {
    DisplayLockGuard lock(this);
    
    if (center_gif_) {
        lv_gif_set_src(center_gif_, nullptr);  // 停止GIF动画
        lv_obj_add_flag(center_gif_, LV_OBJ_FLAG_HIDDEN);
    }
}

void AcornDisplay::ClearAll() {
    DisplayLockGuard lock(this);
    
    // 清除状态图标
    ClearStatusIcon();
    
    // 清除底部内容
    ClearBottomContent();
    
    // 停止并清除中心 GIF 动画
    ClearCenterGif();
}