#include "pebble.h"

static Window *s_main_window;
static TextLayer *s_time_layer;

static TextLayer *s_date_layer;

static int pox_x;
static int pox_y;
static uint32_t OLD_POX_X = 1;
static uint32_t OLD_POX_Y = 2;

char *sys_locale ;
//Pour la carte
static GBitmap *s_bitmap_circle;
static GBitmap *s_bitmap_sea;
static GBitmap *s_bitmap_map;
static GBitmap *s_bitmap_cutted_map;
static GBitmap *s_bitmap_clouds;
static GBitmap *s_bitmap_cutted_clouds;

static BitmapLayer *s_bitmap_layer_circle;
static BitmapLayer *s_bitmap_layer_sea;
static TextLayer *s_bitmap_layer_sea_APLITE;
static BitmapLayer *s_bitmap_layer_map;
static BitmapLayer *s_bitmap_layer_clouds;

static Layer *s_layer_lines;



static AppSync sync;
static uint8_t sync_buffer[64];
 
enum {
  LONGITUDE = 0x0,
  LATITUDE = 0x1
};
 
char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

void sync_tuple_changed_callback(const uint32_t key,
        const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
   
  switch (key) {
    case LONGITUDE:
      pox_x = new_tuple->value->uint32;
      break;
    case LATITUDE:
      pox_y = new_tuple->value->uint32;
      break;
  }
}

void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "... Sync Error: %s", translate_error(app_message_error));
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  // Create a long-lived buffer
  static char buffer_time[] = "00:00";
  static char buffer_date[] = "00/00";

  // Set the TextLayer's contents depending on locale
  if (strcmp("fr_FR", sys_locale) == 0) {
    strftime(buffer_date, sizeof("00/00"), "%d/%m", tick_time);
  } else if (strcmp("us_US", sys_locale) == 0 ) {
    strftime(buffer_date, sizeof("00/00"), "%m/%d", tick_time); 
  } else {
    // Fall back to ISO
    strftime(buffer_date, sizeof("00/00"), "%d/%m", tick_time); 
  }
  
  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer_time, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer_time, sizeof("00:00"), "%I:%M", tick_time);
  }

  text_layer_set_text(s_time_layer,buffer_time);
  text_layer_set_text(s_date_layer,buffer_date);
}

#ifdef PBL_PLATFORM_BASALT
static void update_map(){
        APP_LOG(APP_LOG_LEVEL_DEBUG, "LONG GEN %ld", pox_x);

  GRect sub_rect = GRect(((int)persist_read_int(OLD_POX_X)),0,120,120);
  GRect map_rect = GRect(11,37,120,120);
  
  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map,GCompOpSet );
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_circle),bitmap_layer_get_layer(s_bitmap_layer_map));  
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map),bitmap_layer_get_layer(s_bitmap_layer_circle));     
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_sea),bitmap_layer_get_layer(s_bitmap_layer_map));

}

static void generate_map(Layer *window_layer, GRect bounds){
  //init circle$
  s_bitmap_circle = gbitmap_create_with_resource(RESOURCE_ID_STARS); 
  s_bitmap_layer_circle = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer_circle, s_bitmap_circle);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_circle,GCompOpSet );

  //init map
      APP_LOG(APP_LOG_LEVEL_DEBUG, "LONG GEN %ld", pox_x);


  GRect sub_rect = GRect(persist_read_int(OLD_POX_X),0,120,120);
  GRect map_rect = GRect(11,37,120,120);
  
  s_bitmap_map = gbitmap_create_with_resource(RESOURCE_ID_DOUBLEMAPDECALE); 
  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map,GCompOpSet );
  
  //init sea
  s_bitmap_sea = gbitmap_create_with_resource(RESOURCE_ID_SEA);
  s_bitmap_layer_sea = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer_sea, s_bitmap_sea);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_sea,GCompOpSet );

  //init time
  s_time_layer = text_layer_create(GRect(0, 0, 136, 28));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  
    //init time
  s_date_layer = text_layer_create(GRect(0, 30, 136, 16));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  
  //add layers
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer)); 
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer)); 
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_circle),bitmap_layer_get_layer(s_bitmap_layer_map));  
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_map),bitmap_layer_get_layer(s_bitmap_layer_sea));  
  layer_add_child(window_layer, s_layer_lines); 

  //sort layers
  layer_insert_above_sibling(text_layer_get_layer(s_time_layer) ,bitmap_layer_get_layer(s_bitmap_layer_circle));     
  layer_insert_above_sibling(text_layer_get_layer(s_date_layer) ,bitmap_layer_get_layer(s_bitmap_layer_circle));     
  layer_insert_above_sibling(s_layer_lines ,bitmap_layer_get_layer(s_bitmap_layer_circle));     
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map),bitmap_layer_get_layer(s_bitmap_layer_circle));     
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_sea),bitmap_layer_get_layer(s_bitmap_layer_map));
    
  update_time();
  
}

#else  
  static void update_map(){
        APP_LOG(APP_LOG_LEVEL_DEBUG, "LONG GEN %ld", (long) pox_x);

  GRect sub_rect = GRect(((int)persist_read_int(OLD_POX_X)),0,120,120);
  GRect map_rect = GRect(11,37,120,120);
  
  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map,GCompOpAssign);
  layer_add_child(s_layer_lines, bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map) ,s_layer_lines);     


}

static void generate_map(Layer *window_layer, GRect bounds){
  //init map
  APP_LOG(APP_LOG_LEVEL_DEBUG, "LONG GEN %ld", (long) pox_x);
  GRect sub_rect = GRect(persist_read_int(OLD_POX_X),0,120,120);
  GRect map_rect = GRect(11,37,120,120);
  
  s_bitmap_map = gbitmap_create_with_resource(RESOURCE_ID_DOUBLEMAPDECALE); 
  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map,GCompOpAssign);
  //init time
  s_time_layer = text_layer_create(GRect(0, 0, 136, 28));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  
    //init time
  s_date_layer = text_layer_create(GRect(0, 30, 136, 16));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  
  //add layers
    layer_add_child(window_layer, s_layer_lines); 
  layer_add_child(s_layer_lines, bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer)); 
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer)); 

  //sort layers
  layer_insert_above_sibling(text_layer_get_layer(s_time_layer) ,s_layer_lines);     
  layer_insert_above_sibling(text_layer_get_layer(s_date_layer) ,s_layer_lines);     
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map) ,s_layer_lines);     
  
  update_time();

  
} 
#endif

#ifdef PBL_PLATFORM_BASALT
static void update_line_proc(Layer *this_layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"LAT DESSIN %ld", (long) pox_y);
  graphics_fill_rect(ctx, GRect(71, 30, 1, pox_y), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(71, 30, 63, 1), 0, GCornerNone);

  if(persist_read_int(OLD_POX_X) != pox_x || persist_read_int(OLD_POX_Y) != pox_y){
          persist_write_int(OLD_POX_X, pox_x);
          persist_write_int(OLD_POX_Y, pox_y);
        APP_LOG(APP_LOG_LEVEL_DEBUG,"LONG update %ld", persist_read_int(OLD_POX_X));
        update_map();
  } 
  
}
#else
 static void update_line_proc(Layer *this_layer, GContext *ctx) {

  //draw circle
  int i = 0;
  graphics_context_set_fill_color(ctx,GColorBlack);
  //gauche
  graphics_fill_rect(ctx, GRect(0, 0, 11, 168), 0, GCornerNone);
  // haut
  graphics_fill_rect(ctx, GRect(11, 0, 121, 37), 0, GCornerNone);
  //bas
  graphics_fill_rect(ctx, GRect(11, 157, 121, 11), 0, GCornerNone);
  //droite
  graphics_fill_rect(ctx, GRect(132, 0, 12, 168), 0, GCornerNone);
  
  //cercle
  //h=4
  graphics_fill_rect(ctx, GRect(11, 37, 1, 58), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(11, 100, 1, 60), 0, GCornerNone);
  //h=24
  graphics_fill_rect(ctx, GRect(12, 37, 1, 47), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(12, 109, 1, 60), 0, GCornerNone);
  //h=32
  graphics_fill_rect(ctx, GRect(13, 37, 1, 43), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(13, 113, 1, 60), 0, GCornerNone);
  //h=38
  graphics_fill_rect(ctx, GRect(14, 37, 1, 40), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(14, 116, 1, 60), 0, GCornerNone); 
  //h=44
  graphics_fill_rect(ctx, GRect(15, 37, 1, 37), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(15, 119, 1, 60), 0, GCornerNone);  
  //h=46 1
  graphics_fill_rect(ctx, GRect(16, 37, 1, 35), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(16, 121, 1, 60), 0, GCornerNone);  
  //h=50 2
  graphics_fill_rect(ctx, GRect(17, 37, 1, 33), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(17, 123, 1, 60), 0, GCornerNone); 
  
  //h=54 3
  graphics_fill_rect(ctx, GRect(18, 37, 1, 31), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(18, 125, 1, 60), 0, GCornerNone); 
  //h=58 4
  graphics_fill_rect(ctx, GRect(19, 37, 1, 29), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(19, 127, 1, 60), 0, GCornerNone);   
  //h=62 5
  graphics_fill_rect(ctx, GRect(20, 37, 1, 27), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(20, 129, 1, 60), 0, GCornerNone);
  
  // h =64 1
  graphics_fill_rect(ctx, GRect(21, 37, 1, 26), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(21, 130, 1, 60), 0, GCornerNone);
  
  //h=68 2
  graphics_fill_rect(ctx, GRect(22, 37, 1, 24), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(22, 132, 1, 60), 0, GCornerNone);
  
    //h=70 1
  graphics_fill_rect(ctx, GRect(23, 37, 1, 23), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(23, 133, 1, 60), 0, GCornerNone);
  
  //h=72 1
  graphics_fill_rect(ctx, GRect(24, 37, 1, 22), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(24, 134, 1, 60), 0, GCornerNone);
  //h=76 2
  graphics_fill_rect(ctx, GRect(25, 37, 1, 20), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(25, 136, 1, 60), 0, GCornerNone);
  
  
  for(i=1;i<8;i++){
      graphics_fill_rect(ctx, GRect(25+i, 37, 1, 20 - i), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(25 + i, 136 + i, 1, 60), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(117 - i, 37, 1, 20 - i), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(117-i, 136 + i, 1, 60), 0, GCornerNone);
  }
  
    for(i=1;i<3;i++){
      graphics_fill_rect(ctx, GRect(32+i, 37, 1, 13), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(32 + i, 143, 1, 60), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(110 - i, 37, 1, 13), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(110 -i, 143, 1, 60), 0, GCornerNone);
  }
  
      for(i=1;i<2;i++){
      graphics_fill_rect(ctx, GRect(34+i, 37, 1, 12), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(34 + i, 144, 1, 60), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(108 - i, 37, 1, 12), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(108 -i, 144, 1, 60), 0, GCornerNone);
  }
  
  for(i=1;i<6;i=i+2){
      graphics_fill_rect(ctx, GRect(35+i, 37, 1, 10-i/2), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(35+i, 145+i/2, 1, 60), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(35+i+1, 37, 1, 10-i/2), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(35+i+1, 145+(i+1)/2, 1, 60), 0, GCornerNone);
          
      graphics_fill_rect(ctx, GRect(107-i-1, 37, 1, 10-i/2), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(107-i-1, 145+i/2, 1, 60), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(107-i, 37, 1, 10-i/2), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(107-i, 145+i/2, 1, 60), 0, GCornerNone);
  }
  
    graphics_fill_rect(ctx, GRect(42, 41, 3, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 151, 3, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 40, 6, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 152, 6, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 39, 9, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 153, 9, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 38, 13, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 154, 13, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 37, 26, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 155,26, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 36, 28, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(42, 156,150, 1), 0, GCornerNone);
  
    graphics_fill_rect(ctx, GRect(98, 42, 70, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(98, 150, 70, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(96, 41, 70, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(96, 151,70, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(93, 40, 70, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(93, 152, 70, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(90, 39, 70, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(90, 153, 70, 1), 0, GCornerNone);
  
    graphics_fill_rect(ctx, GRect(87, 38, 70, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(87, 154,70, 1), 0, GCornerNone);
  
    graphics_fill_rect(ctx, GRect(74, 37, 70, 1), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(74, 155,70, 1), 0, GCornerNone);
  

  

  //h=76 2
  graphics_fill_rect(ctx, GRect(117, 37, 1, 20), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(117, 136, 1, 60), 0, GCornerNone);
  //h=72 1
  graphics_fill_rect(ctx, GRect(118, 37, 1, 22), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(118, 134, 1, 60), 0, GCornerNone);
  //h=70
  graphics_fill_rect(ctx, GRect(119, 37, 1, 23), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(119, 133, 1, 60), 0, GCornerNone);
  
  //h=68
  graphics_fill_rect(ctx, GRect(120, 37, 1, 24), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(120, 132, 1, 60), 0, GCornerNone); 
  // h =64
  graphics_fill_rect(ctx, GRect(121, 37, 1, 26), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(121, 130, 1, 60), 0, GCornerNone);
  
  //h=62 5
  graphics_fill_rect(ctx, GRect(122, 37, 1, 27), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(122, 129, 1, 60), 0, GCornerNone); 
  //h=58 4
  graphics_fill_rect(ctx, GRect(123, 37, 1, 29), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(123, 127, 1, 60), 0, GCornerNone); 
  //h=54 3
  graphics_fill_rect(ctx, GRect(124, 37, 1, 31), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(124, 125, 1, 60), 0, GCornerNone);  
  //h=50 2
  graphics_fill_rect(ctx, GRect(125, 37, 1, 33), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(125, 123, 1, 60), 0, GCornerNone);
  //h=46
  graphics_fill_rect(ctx, GRect(126, 37, 1, 35), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(126, 121, 1, 60), 0, GCornerNone);
  //h=44
  graphics_fill_rect(ctx, GRect(127, 37, 1, 37), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(127, 119, 1, 60), 0, GCornerNone);
  //h=38
  graphics_fill_rect(ctx, GRect(128, 37, 1, 40), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(128, 116, 1, 60), 0, GCornerNone);
  //h=32
  graphics_fill_rect(ctx, GRect(129, 37, 1, 43), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(129, 113, 1, 60), 0, GCornerNone); 
  //h=22
  graphics_fill_rect(ctx, GRect(130, 37, 1, 47), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(130, 109, 1, 60), 0, GCornerNone);
  //h=4
  graphics_fill_rect(ctx, GRect(131, 37, 1, 58), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(131, 100, 1, 60), 0, GCornerNone);

    graphics_context_set_fill_color(ctx, GColorWhite);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"LAT DESSIN %ld",(long) pox_y);
  graphics_fill_rect(ctx, GRect(71, 30, 1, persist_read_int(OLD_POX_Y)), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(71, 30, 63, 1), 0, GCornerNone);

  
  if(persist_read_int(OLD_POX_X) != pox_x || persist_read_int(OLD_POX_Y) != pox_y){
          persist_write_int(OLD_POX_X, pox_x);
          persist_write_int(OLD_POX_Y, pox_y);
        APP_LOG(APP_LOG_LEVEL_DEBUG,"LONG update %ld", (long) pox_x);
        update_map();
  } 
  
} 
  
#endif

static void main_window_load(Window *window) {
    
  Layer *window_layer  = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // location init
  Tuplet initial_values[] = {
     TupletInteger(LATITUDE, 0),    
     TupletInteger(LONGITUDE, 0)
  };
  
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
  
  //init lines
  s_layer_lines = layer_create(bounds);
  layer_set_update_proc(s_layer_lines,update_line_proc);
  
  generate_map(window_layer, bounds);

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}


static void main_window_unload(Window *window) {
}

static void init(void) {
  // Register with TickTimerService
  
  persist_write_int(OLD_POX_X, 600);
  persist_write_int(OLD_POX_Y, 600);
  s_main_window = window_create();
  sys_locale = setlocale(LC_ALL, "");
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  //message location open
  app_message_open(64, 64);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  // Make sure the time is displayed from the start
  update_time();
}

static void deinit(void) {
  window_destroy(s_main_window);
  gbitmap_destroy(s_bitmap_map);
  gbitmap_destroy(s_bitmap_cutted_map);
  gbitmap_destroy(s_bitmap_sea);
  gbitmap_destroy(s_bitmap_circle);


}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

