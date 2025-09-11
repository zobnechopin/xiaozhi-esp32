#include "acorn_display.h"
#include "acorn_assets.h"
#include <esp_log.h>
#include <libs/gif/lv_gif.h>

#define TAG "AcornDisplay"

AcornDisplay::AcornDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                          int width, int height, int offset_x, int offset_y, 
                          bool mirror_x, bool mirror_y, bool swap_xy)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy),
      status_icon_(nullptr), center_gif_(nullptr), bottom_container_(nullptr),
      bottom_image_(nullptr), bottom_label_(nullptr) {
    
    SetupCustomLayout();
}

void AcornDisplay::SetupCustomLayout() {
    DisplayLockGuard lock(this);
    
    auto screen = lv_screen_active();
    
    ESP_LOGI(TAG, "Setting up custom layout...");
    
    // 创建自定义 UI 对象，让它们覆盖在默认 UI 之上
    status_icon_ = lv_img_create(screen);
    lv_obj_align(status_icon_, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_add_flag(status_icon_, LV_OBJ_FLAG_HIDDEN);
    
    center_gif_ = lv_gif_create(screen);
    lv_obj_align(center_gif_, LV_ALIGN_CENTER, 0, 0);
    
    // 创建底部容器
    bottom_container_ = lv_obj_create(screen);
    lv_obj_set_size(bottom_container_, LV_HOR_RES, 60);
    lv_obj_align(bottom_container_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(bottom_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottom_container_, 0, 0);
    lv_obj_set_style_pad_all(bottom_container_, 5, 0);
    
    // 创建底部图片
    bottom_image_ = lv_img_create(bottom_container_);
    lv_obj_align(bottom_image_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(bottom_image_, LV_OBJ_FLAG_HIDDEN);
    
    // 创建底部文字
    bottom_label_ = lv_label_create(bottom_container_);
    lv_obj_align(bottom_label_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(bottom_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(bottom_label_, LV_OBJ_FLAG_HIDDEN);
    
    ESP_LOGI(TAG, "Custom layout setup completed");
}

void AcornDisplay::SetStatusIcon(const char* icon_name) {
    DisplayLockGuard lock(this);
    
    if (!icon_name) {
        ClearStatusIcon();
        return;
    }
    
    const lv_img_dsc_t* icon = AcornAssets::GetStatusIcon(icon_name);
    if (icon) {
        lv_img_set_src(status_icon_, icon);
        lv_obj_remove_flag(status_icon_, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Set status icon: %s", icon_name);
    } else {
        ESP_LOGW(TAG, "Status icon not found: %s", icon_name);
        ClearStatusIcon();
    }
}

void AcornDisplay::SetCenterGif(const char* gif_name) {
    DisplayLockGuard lock(this);
    
    if (!gif_name) return;
    
    const lv_img_dsc_t* gif = AcornAssets::GetGif(gif_name);
    if (gif) {
        lv_gif_set_src(center_gif_, gif);
        ESP_LOGI(TAG, "Set center GIF: %s", gif_name);
    } else {
        ESP_LOGW(TAG, "GIF not found: %s", gif_name);
    }
}

void AcornDisplay::SetBottomContent(const char* content_name) {
    DisplayLockGuard lock(this);
    
    if (!content_name) {
        ClearBottomContent();
        return;
    }
    
    const lv_img_dsc_t* icon = AcornAssets::GetBottomIcon(content_name);
    if (icon) {
        lv_img_set_src(bottom_image_, icon);
        lv_obj_remove_flag(bottom_image_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(bottom_label_, LV_OBJ_FLAG_HIDDEN);  // 隐藏文字
        ESP_LOGI(TAG, "Set bottom icon: %s", content_name);
    } else {
        ESP_LOGW(TAG, "Bottom icon not found: %s", content_name);
    }
}

void AcornDisplay::SetBottomText(const char* text) {
    DisplayLockGuard lock(this);
    
    if (!text) {
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

void AcornDisplay::ClearAll() {
    ClearStatusIcon();
    ClearBottomContent();
}