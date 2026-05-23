#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;
static char s_forecast_data[24][4][10]; 
static int s_uv_values[24];
static int s_is_night[24];
static int s_conditions[24];
static GBitmap *s_icon_bitmaps[24];
static bool s_is_metric = false; 
static int s_scroll_offset = 0; 

static void update_icons_for_view() {
  for(int i = 0; i < 24; i++) {
    if (i >= s_scroll_offset && i < s_scroll_offset + 6) {
      if (!s_icon_bitmaps[i] && s_conditions[i] != 0) {
        bool night = (s_is_night[i] == 1);
        uint32_t res_id;
        int cid = s_conditions[i];

        if (cid == 800) res_id = night ? RESOURCE_ID_ICON_CLEAR_NIGHT : RESOURCE_ID_ICON_CLEAR_DAY;
        else if (cid == 801) res_id = night ? RESOURCE_ID_ICON_CLOUDY_NIGHT : RESOURCE_ID_ICON_CLOUDY_SUN;
        else if (cid <= 804) res_id = RESOURCE_ID_ICON_CLOUDY;
        else if (cid >= 200 && cid <= 232) res_id = RESOURCE_ID_ICON_THUNDER_SHOWER;
        else if (cid >= 300 && cid <= 321) res_id = night ? RESOURCE_ID_ICON_SHOWER_NIGHT : RESOURCE_ID_ICON_SHOWER_DAY;
        else if (cid >= 500 && cid <= 501) res_id = RESOURCE_ID_ICON_LIGHT_RAIN;
        else if (cid >= 502 && cid <= 504) res_id = RESOURCE_ID_ICON_HEAVY_RAIN;
        else if (cid == 511) res_id = RESOURCE_ID_ICON_SLEET;
        else if (cid >= 520 && cid <= 531) res_id = RESOURCE_ID_ICON_RAIN_STORM;
        else if (cid == 600) res_id = RESOURCE_ID_ICON_LIGHT_SNOW;
        else if (cid == 601) res_id = RESOURCE_ID_ICON_SNOW;
        else if (cid == 602) res_id = RESOURCE_ID_ICON_HEAVY_SNOW;
        else if (cid >= 620 && cid <= 622) res_id = night ? RESOURCE_ID_ICON_SNOW_SHOWER_NIGHT : RESOURCE_ID_ICON_SNOW_SHOWER_DAY;
        else if (cid == 701 || cid == 741) res_id = RESOURCE_ID_ICON_FOG;
        else if (cid == 711 || cid == 721) res_id = RESOURCE_ID_ICON_HAZE;
        else if (cid == 731 || cid == 761) res_id = RESOURCE_ID_ICON_DUST;
        else if (cid == 751) res_id = RESOURCE_ID_ICON_SANDSTORM;
        else res_id = RESOURCE_ID_ICON_CLOUDY;

        s_icon_bitmaps[i] = gbitmap_create_with_resource(res_id);
      }
    } else {
      if (s_icon_bitmaps[i]) {
        gbitmap_destroy(s_icon_bitmaps[i]);
        s_icon_bitmaps[i] = NULL;
      }
    }
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_scroll_offset > 0) {
    s_scroll_offset -= 2;
    update_icons_for_view();
    layer_mark_dirty(s_canvas_layer);
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_scroll_offset < 18) {
    s_scroll_offset += 2;
    update_icons_for_view();
    layer_mark_dirty(s_canvas_layer);
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  char *temp_label = s_is_metric ? "°C" : "°F";
  char *wind_label = s_is_metric ? "km/h" : "MPH"; 
  char *headers[] = {"Time", "Cond", temp_label, wind_label, "Rain"};
  int x_pos[] = {5, 42, 85, 122, 160}; 

  // Header Background
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 24), 0, GCornerNone);

  graphics_context_set_text_color(ctx, GColorBlack);
  for(int i = 0; i < 5; i++) {
    graphics_draw_text(ctx, headers[i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), 
                       GRect(x_pos[i], 0, 42, 24), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  }

  for(int i = 0; i < 6; i++) {
    int idx = i + s_scroll_offset;
    int y = 24 + (i * 34); 
    
    // Highlight first row (Now)
    if (idx == 0) {
      graphics_context_set_fill_color(ctx, GColorPictonBlue);
      graphics_fill_rect(ctx, GRect(0, y, 3, 32), 0, GCornerNone);
    }

    graphics_context_set_stroke_color(ctx, GColorLightGray);
    graphics_draw_line(ctx, GPoint(0, y - 2), GPoint(bounds.size.w - 12, y - 2));

    // Time
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, s_forecast_data[idx][0], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
                       GRect(x_pos[0], y + 2, 35, 30), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    
    // Icon
    if(s_icon_bitmaps[idx]) {
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, s_icon_bitmaps[idx], GRect(x_pos[1], y + 2, 32, 32));
    }

    // Temp (Semantic Colors)
    int temp_val = atoi(s_forecast_data[idx][1]);
    GColor temp_color = GColorBlack;
    if (!s_is_metric) {
      if (temp_val >= 90) temp_color = GColorRed;
      else if (temp_val <= 32) temp_color = GColorBlue;
    } else {
      if (temp_val >= 32) temp_color = GColorRed;
      else if (temp_val <= 0) temp_color = GColorBlue;
    }
    graphics_context_set_text_color(ctx, temp_color);
    graphics_draw_text(ctx, s_forecast_data[idx][1], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
                       GRect(x_pos[2], y + 2, 40, 30), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    // Wind (Warning Color)
    int wind_val = atoi(s_forecast_data[idx][2]);
    graphics_context_set_text_color(ctx, (wind_val >= 20) ? GColorOrange : GColorBlack);
    graphics_draw_text(ctx, s_forecast_data[idx][2], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
                       GRect(x_pos[3], y + 2, 40, 30), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    // Rain Pie Chart
    int rain_val = atoi(s_forecast_data[idx][3]); 
    GRect rain_rect = GRect(x_pos[4], y + 6, 22, 22);
    graphics_context_set_stroke_color(ctx, GColorLightGray);
    graphics_draw_arc(ctx, rain_rect, GOvalScaleModeFitCircle, 0, TRIG_MAX_ANGLE);
    if (rain_val > 0) {
      graphics_context_set_fill_color(ctx, GColorPictonBlue);
      int32_t rain_angle = (TRIG_MAX_ANGLE * rain_val) / 100;
      graphics_fill_radial(ctx, rain_rect, GOvalScaleModeFitCircle, 11, 0, rain_angle);
    }

    // UV Safety Bar
    GColor uv_color = GColorGreen;
    int uv = s_uv_values[idx];
    if(uv > 2) uv_color = GColorYellow;
    if(uv > 5) uv_color = GColorOrange;
    if(uv > 7) uv_color = GColorRed;
    if(uv > 10) uv_color = GColorPurple;
    graphics_context_set_fill_color(ctx, uv_color);
    graphics_fill_rect(ctx, GRect(bounds.size.w - 10, y + 2, 8, 30), 2, GCornersAll);
  }

  // Scroll Bar
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, GRect(bounds.size.w - 2, 24, 2, bounds.size.h - 24), 0, GCornerNone);
  int scroll_handle_y = 24 + ((s_scroll_offset * (bounds.size.h - 64)) / 18);
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(bounds.size.w - 2, scroll_handle_y, 2, 40), 0, GCornerNone);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);
  while(t != NULL) {
    uint32_t key = t->key;
    if (key == MESSAGE_KEY_UNITS) {
      s_is_metric = (strcmp(t->value->cstring, "metric") == 0);
    } else if (key >= MESSAGE_KEY_IS_NIGHT && key < MESSAGE_KEY_IS_NIGHT + 24) {
      s_is_night[key - MESSAGE_KEY_IS_NIGHT] = t->value->int32;
    } else if (key >= MESSAGE_KEY_COLUMN_COND && key < MESSAGE_KEY_COLUMN_COND + 24) {
      s_conditions[key - MESSAGE_KEY_COLUMN_COND] = (int)t->value->int32;
    } else if (key >= MESSAGE_KEY_COLUMN_TIME && key < MESSAGE_KEY_COLUMN_TIME + 24) {
      snprintf(s_forecast_data[key - MESSAGE_KEY_COLUMN_TIME][0], 10, "%s", t->value->cstring);
    } else if (key >= MESSAGE_KEY_COLUMN_TEMP && key < MESSAGE_KEY_COLUMN_TEMP + 24) {
      snprintf(s_forecast_data[key - MESSAGE_KEY_COLUMN_TEMP][1], 10, "%s", t->value->cstring);
    } else if (key >= MESSAGE_KEY_COLUMN_WIND && key < MESSAGE_KEY_COLUMN_WIND + 24) {
      snprintf(s_forecast_data[key - MESSAGE_KEY_COLUMN_WIND][2], 10, "%s", t->value->cstring);
    } else if (key >= MESSAGE_KEY_COLUMN_RAIN && key < MESSAGE_KEY_COLUMN_RAIN + 24) {
      snprintf(s_forecast_data[key - MESSAGE_KEY_COLUMN_RAIN][3], 10, "%s", t->value->cstring);
    } else if (key >= MESSAGE_KEY_COLUMN_UV && key < MESSAGE_KEY_COLUMN_UV + 24) {
      s_uv_values[key - MESSAGE_KEY_COLUMN_UV] = (int)t->value->int32;
    }
    t = dict_read_next(iterator);
  }
  update_icons_for_view();
  layer_mark_dirty(s_canvas_layer);
}

static void main_window_load(Window *window) {
  s_canvas_layer = layer_create(layer_get_bounds(window_get_root_layer(window)));
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) { .load = main_window_load });
  window_stack_push(s_main_window, true);
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(8192, 512); 
}

static void deinit() {
  for(int i = 0; i < 24; i++) { if(s_icon_bitmaps[i]) gbitmap_destroy(s_icon_bitmaps[i]); }
  layer_destroy(s_canvas_layer);
  window_destroy(s_main_window);
}

int main(void) { init(); app_event_loop(); deinit(); }