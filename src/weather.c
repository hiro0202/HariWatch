#include "pebble.h"

static Window *s_main_window;

static TextLayer *s_temperature_layer;
static TextLayer *s_city_layer;
static TextLayer *s_temperature2_layer;
static TextLayer *text1_layer;
static TextLayer *text2_layer;
static TextLayer *s_time_layer, *s_date_layer;
static BitmapLayer *s_icon_layer;
static BitmapLayer *onpu_layer;
static GBitmap *s_icon_bitmap = NULL;
static GBitmap *onpu_bitmap = NULL;
int counter = 0;
bool isConnect = true;

//バッテリー表示レイヤー
static TextLayer *s_bat_layer;
//Bluetoothステータス
static char bt_status[4] = "O";
//過去のBluetoothステータス
static int bt_st_old = 1;

//バッテリーハンドラ
static void battery_handler(BatteryChargeState new_state) {
   //バッテリー状態取得して表示(ついでにBluetooth接続状態を表示)
  static char s_battery_buffer[32];
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", new_state.charge_percent);
  //text_layer_set_text(s_bat_layer, s_battery_buffer);
}
//Bluetoothハンドラ
void bt_handler(bool connected) {
  if (connected) {
      //接続に変化したらバイブ
      if(bt_st_old == 0)
         vibes_short_pulse();
      bt_st_old = 1;
      snprintf(bt_status,4,"O");
      isConnect = true;
   } else {
      //切断に変化したらバイブ
      if(bt_st_old == 1)
         vibes_short_pulse();
      bt_st_old = 0;
      snprintf(bt_status,4,"-");
      isConnect = false;
  }
   //現在のバッテリーを読む(BT表示)
   battery_handler(battery_state_service_peek());
}


enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
  WEATHER_CITY_KEY = 0x2,         // TUPLE_CSTRING
  WEATHER_HUMID_KEY = 0x3,  // TUPLE_CSTRING
};

static AppSync s_sync;
static uint8_t s_sync_buffer[64];


static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  
  switch (key) {

    case WEATHER_TEMPERATURE_KEY:
       printf("temp\n");
      // App Sync keeps new_tuple in s_sync_buffer, so we may use it directly
      if(strlen(new_tuple->value->cstring) > 2){
        text_layer_set_text(s_temperature_layer, new_tuple->value->cstring);
      }
      break;

    case WEATHER_CITY_KEY:
      printf("temp2\n");
    if(strlen(new_tuple->value->cstring) > 2){
      text_layer_set_text(s_city_layer, new_tuple->value->cstring);
    
     if (onpu_bitmap) {
        gbitmap_destroy(onpu_bitmap);
      }
      if(atoi(new_tuple->value->cstring) > 18){
        if(isConnect){
          onpu_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEART);
        }else{
          onpu_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ONPU);
        }
      }else{
        onpu_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_IKARI);
      }
      bitmap_layer_set_compositing_mode(onpu_layer,GCompOpSet);
      bitmap_layer_set_bitmap(onpu_layer, onpu_bitmap);
    }
      break;
    
    case WEATHER_HUMID_KEY:
      printf("humid\n");
    if(strlen(new_tuple->value->cstring) > 2){
      text_layer_set_text(s_temperature2_layer, new_tuple->value->cstring);
    }
      break;
    default :
      break;
  }
}

static void request_weather(void) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) {
    // Error creating outbound message
    return;
  }

  int value = 1;
  dict_write_int(iter, 1, &value, sizeof(int), true);
  dict_write_end(iter);

  app_message_outbox_send();
}

//-----------------------------------------------------------
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
 
  // Create a long-lived buffer
  static char buffer[] = "00:00";
 
  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
 
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  
   // Copy date into buffer from tm structure
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);

  // Show the date
  text_layer_set_text(s_date_layer, date_buffer);
  
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
   update_time();
   counter++;
  
      if (s_icon_bitmap) {
        gbitmap_destroy(s_icon_bitmap);
      }
      
      if(counter%2 == 0){
      s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEADGEHOG);
      bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
      bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
      }else{
      s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEADGEHOG2);
      bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
      bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
      }
  
  if(counter%600 == 0){
    request_weather();
    counter = 0;
  }
    
}
//-----------------------------------------------------------


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  //ハリネズミの画像表示用
  s_icon_layer = bitmap_layer_create(GRect(40, 20, 97, 60));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));
  
  //ステータス画像表示用
  onpu_layer = bitmap_layer_create(GRect(-50, 15, bounds.size.w, 33));
  layer_add_child(window_layer, bitmap_layer_get_layer(onpu_layer));

  text1_layer = text_layer_create(GRect(0, 80, bounds.size.w, 32));
  text_layer_set_text_color(text1_layer, GColorWhite);
  text_layer_set_background_color(text1_layer, GColorClear);
  text_layer_set_font(text1_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(text1_layer, GTextAlignmentCenter);
  text_layer_set_text(text1_layer, "Room       Cage");
  layer_add_child(window_layer, text_layer_get_layer(text1_layer));
  
  s_temperature_layer = text_layer_create(GRect(-30, 90, bounds.size.w, 32));
  text_layer_set_text_color(s_temperature_layer, GColorWhite);
  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_font(s_temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));

  s_city_layer = text_layer_create(GRect(30, 90, bounds.size.w, 32));
  text_layer_set_text_color(s_city_layer, GColorWhite);
  text_layer_set_background_color(s_city_layer, GColorClear);
  text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_city_layer));
  
    text2_layer = text_layer_create(GRect(0, 120, bounds.size.w, 32));
  text_layer_set_text_color(text2_layer, GColorWhite);
  text_layer_set_background_color(text2_layer, GColorClear);
  text_layer_set_font(text2_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(text2_layer, GTextAlignmentCenter);
  text_layer_set_text(text2_layer, "Humidity");
  layer_add_child(window_layer, text_layer_get_layer(text2_layer));
  
   s_temperature2_layer = text_layer_create(GRect(0, 130, bounds.size.w, 32));
  text_layer_set_text_color(s_temperature2_layer, GColorWhite);
  text_layer_set_background_color(s_temperature2_layer, GColorClear);
  text_layer_set_font(s_temperature2_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_temperature2_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_temperature2_layer));
  
  s_bat_layer = text_layer_create(GRect(0, 0, bounds.size.w, 32));
  text_layer_set_text_color(s_bat_layer, GColorWhite);
  text_layer_set_background_color(s_bat_layer, GColorClear);
  text_layer_set_font(s_bat_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_bat_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_bat_layer));
  
  
  //---------------------------------------------------------------
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 30, bounds.size.w, 32));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
 
  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(0, 53, bounds.size.w, 32));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  //---------------------------------------------------------------

  Tuplet initial_values[] = {
    TupletCString(WEATHER_TEMPERATURE_KEY, "-\u00B0C"),
    TupletCString(WEATHER_CITY_KEY, "-\u00B0C"),
    TupletCString(WEATHER_HUMID_KEY, "-％"),
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 1),
  };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
  
 //バッテリー状態サービスに登録する
   battery_state_service_subscribe(battery_handler);
   //現在のバッテリーを読む
   battery_handler(battery_state_service_peek());
   
   //Bluetooth接続状態サービスに登録する
   connection_service_subscribe((ConnectionHandlers){
     .pebble_app_connection_handler = bt_handler
  });
  
  request_weather();
}

static void window_unload(Window *window) {
  if (s_icon_bitmap) {
    gbitmap_destroy(s_icon_bitmap);
  }

  text_layer_destroy(s_city_layer);
  text_layer_destroy(s_temperature_layer);
  text_layer_destroy(s_temperature2_layer);
  text_layer_destroy(text1_layer);
  text_layer_destroy(text2_layer);
  bitmap_layer_destroy(s_icon_layer);
  bitmap_layer_destroy(onpu_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
//  text_layer_destory(s_bat_layer);
}

static void init(void) {
  s_main_window = window_create();
  window_set_background_color(s_main_window, PBL_IF_COLOR_ELSE(GColorBlack, GColorBlack));
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_main_window, true);

  app_message_open(64, 64);
  
    // Register with TickTimerService
tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
}

static void deinit(void) {
  window_destroy(s_main_window);

  app_sync_deinit(&s_sync);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
