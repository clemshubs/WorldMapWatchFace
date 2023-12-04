#include "pebble.h"

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static int pox_x;
static int pox_y;
static int locOK;

char *sys_locale;
// Pour la carte

static GBitmap *s_bitmap_sea;
static GBitmap *s_bitmap_map;
static GBitmap *s_bitmap_cutted_map;

static BitmapLayer *s_bitmap_layer_map;

#ifdef PBL_PLATFORM_BASALT
static GBitmap *s_bitmap_circle;
static BitmapLayer *s_bitmap_layer_circle;
static BitmapLayer *s_bitmap_layer_sea;
#endif

#ifdef PBL_PLATFORM_CHALK
static GBitmap *s_bitmap_circle;
static BitmapLayer *s_bitmap_layer_circle;
static BitmapLayer *s_bitmap_layer_sea;
#endif

static Layer *s_layer_lines;

static AppSync sync;
static uint8_t sync_buffer[64];

enum
{
  LONGITUDE = 0x0,
  LATITUDE = 0x1
};

char *translate_error(AppMessageResult result)
{
  switch (result)
  {
  case APP_MSG_OK:
    return "APP_MSG_OK";
  case APP_MSG_SEND_TIMEOUT:
    return "APP_MSG_SEND_TIMEOUT";
  case APP_MSG_SEND_REJECTED:
    return "APP_MSG_SEND_REJECTED";
  case APP_MSG_NOT_CONNECTED:
    return "APP_MSG_NOT_CONNECTED";
  case APP_MSG_APP_NOT_RUNNING:
    return "APP_MSG_APP_NOT_RUNNING";
  case APP_MSG_INVALID_ARGS:
    return "APP_MSG_INVALID_ARGS";
  case APP_MSG_BUSY:
    return "APP_MSG_BUSY";
  case APP_MSG_BUFFER_OVERFLOW:
    return "APP_MSG_BUFFER_OVERFLOW";
  case APP_MSG_ALREADY_RELEASED:
    return "APP_MSG_ALREADY_RELEASED";
  case APP_MSG_CALLBACK_ALREADY_REGISTERED:
    return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
  case APP_MSG_CALLBACK_NOT_REGISTERED:
    return "APP_MSG_CALLBACK_NOT_REGISTERED";
  case APP_MSG_OUT_OF_MEMORY:
    return "APP_MSG_OUT_OF_MEMORY";
  case APP_MSG_CLOSED:
    return "APP_MSG_CLOSED";
  case APP_MSG_INTERNAL_ERROR:
    return "APP_MSG_INTERNAL_ERROR";
  default:
    return "UNKNOWN ERROR";
  }
}

static void send_int(int key, int value)
{
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, key, &value, sizeof(int), true);
  app_message_outbox_send();
}

void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "... Sync Error: %s", translate_error(app_message_error));
}

static void update_time()
{
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer_time[] = "00:00";
  static char buffer_date[] = "00/00";

  // Set the TextLayer's contents depending on locale
  if (strcmp("fr_FR", sys_locale) == 0)
  {
    strftime(buffer_date, sizeof("00/00"), "%d/%m", tick_time);
  }
  else if (strcmp("us_US", sys_locale) == 0)
  {
    strftime(buffer_date, sizeof("00/00"), "%m/%d", tick_time);
  }
  else
  {
    // Fall back to ISO
    strftime(buffer_date, sizeof("00/00"), "%d/%m", tick_time);
  }

  // Write the current hours and minutes into the buffer
  if (clock_is_24h_style() == true)
  {
    // Use 24 hour format
    strftime(buffer_time, sizeof("00:00"), "%H:%M", tick_time);
  }
  else
  {
    // Use 12 hour format
    strftime(buffer_time, sizeof("00:00"), "%I:%M", tick_time);
  }

  text_layer_set_text(s_time_layer, buffer_time);
  text_layer_set_text(s_date_layer, buffer_date);
}

#ifdef PBL_PLATFORM_APLITE

static void update_map()
{
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "LONG Update %d",  pox_x);

  GRect sub_rect = GRect(pox_x, 0, 120, 120);
  GRect map_rect = GRect(12, 37, 120, 120);

  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map, GCompOpAssign);
  layer_add_child(s_layer_lines, bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map), s_layer_lines);
}

static void generate_map(Layer *window_layer, GRect bounds)
{
  // init map
  GRect sub_rect = GRect(pox_x, 0, 120, 120);
  GRect map_rect = GRect(12, 37, 120, 120);

  s_bitmap_map = gbitmap_create_with_resource(RESOURCE_ID_DOUBLEMAPDECALE);
  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map, GCompOpAssign);
  // init time
  s_time_layer = text_layer_create(GRect(0, 0, 136, 28));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);

  // init date
  s_date_layer = text_layer_create(GRect(0, 30, 135, 16));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);

  // add layers
  layer_add_child(window_layer, s_layer_lines);
  layer_add_child(s_layer_lines, bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  // sort layers
  layer_insert_above_sibling(text_layer_get_layer(s_time_layer), s_layer_lines);
  layer_insert_above_sibling(text_layer_get_layer(s_date_layer), s_layer_lines);
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map), s_layer_lines);

  update_time();
  // send_int(2,0);
}
#endif
#ifdef PBL_PLATFORM_BASALT

static void update_map()
{

  GRect sub_rect = GRect(pox_x, 0, 120, 120);
  GRect map_rect = GRect(11, 37, 120, 120);

  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map, GCompOpSet);
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_circle), bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map), bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_sea), bitmap_layer_get_layer(s_bitmap_layer_map));
}

static void generate_map(Layer *window_layer, GRect bounds)
{
  // init circle$
  s_bitmap_circle = gbitmap_create_with_resource(RESOURCE_ID_STARS);
  s_bitmap_layer_circle = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer_circle, s_bitmap_circle);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_circle, GCompOpSet);

  GRect sub_rect = GRect(pox_x, 0, 120, 120);
  GRect map_rect = GRect(11, 37, 120, 120);

  s_bitmap_map = gbitmap_create_with_resource(RESOURCE_ID_DOUBLEMAPDECALE);
  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map, GCompOpSet);

  // init sea
  s_bitmap_sea = gbitmap_create_with_resource(RESOURCE_ID_SEA);
  s_bitmap_layer_sea = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer_sea, s_bitmap_sea);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_sea, GCompOpSet);

  // init time
  s_time_layer = text_layer_create(GRect(0, 0, 136, 28));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);

  // init date
  s_date_layer = text_layer_create(GRect(0, 30, 135, 16));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);

  // add layers
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_circle), bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_map), bitmap_layer_get_layer(s_bitmap_layer_sea));
  layer_add_child(window_layer, s_layer_lines);

  // sort layers
  layer_insert_above_sibling(text_layer_get_layer(s_time_layer), bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_insert_above_sibling(text_layer_get_layer(s_date_layer), bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_insert_above_sibling(s_layer_lines, bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map), bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_sea), bitmap_layer_get_layer(s_bitmap_layer_map));

  update_time();
}
#endif
#ifdef PBL_PLATFORM_CHALK
static void update_map()
{

  GRect sub_rect = GRect(pox_x, 0, 120, 120);
  GRect map_rect = GRect(30, 37, 120, 120);

  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map, GCompOpSet);
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_circle), bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map), bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_sea), bitmap_layer_get_layer(s_bitmap_layer_map));
}

static void generate_map(Layer *window_layer, GRect bounds)
{
  // init circle$
  s_bitmap_circle = gbitmap_create_with_resource(RESOURCE_ID_STARS);
  s_bitmap_layer_circle = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer_circle, s_bitmap_circle);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_circle, GCompOpSet);

  GRect sub_rect = GRect(pox_x, 0, 120, 120);
  GRect map_rect = GRect(11, 37, 120, 120);

  s_bitmap_map = gbitmap_create_with_resource(RESOURCE_ID_DOUBLEMAPDECALE);
  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map, GCompOpSet);

  // init sea
  s_bitmap_sea = gbitmap_create_with_resource(RESOURCE_ID_SEA);
  s_bitmap_layer_sea = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer_sea, s_bitmap_sea);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_sea, GCompOpSet);

  // init time
  s_time_layer = text_layer_create(GRect(63, 10, 54, 28));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);

  // init date
  s_date_layer = text_layer_create(GRect(0, 2, 105, 15));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  // add layers
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_circle), bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_map), bitmap_layer_get_layer(s_bitmap_layer_sea));
  layer_add_child(window_layer, s_layer_lines);

  // sort layers
  layer_insert_above_sibling(text_layer_get_layer(s_time_layer), bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_insert_above_sibling(text_layer_get_layer(s_date_layer), bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_insert_above_sibling(s_layer_lines, bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map), bitmap_layer_get_layer(s_bitmap_layer_circle));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_sea), bitmap_layer_get_layer(s_bitmap_layer_map));

  update_time();
}
#endif

#ifdef PBL_PLATFORM_APLITE
float my_sqrt(float num)
{
  float a, p, e = 0.001, b;

  a = num;
  p = a * a;
  while (p - num >= e)
  {
    b = (a + (num / a)) / 2;
    a = b;
    p = a * a;
  }
  return a;
}

static void draw_circle(GContext *ctx)
{
  int r = 3600;
  for (int i = 12; i < 72; i++)
  {
    int x = 60 - i + 12;
    int y = (int)my_sqrt(r - x * x);
    // up left
    graphics_fill_rect(ctx, GRect(i, 36, 1, 60 - y), 0, GCornerNone);
    // down left
    graphics_fill_rect(ctx, GRect(i, 96 + y, 1, 60 - y), 0, GCornerNone);
    // up right
    graphics_fill_rect(ctx, GRect(144 - i, 36, 1, 60 - y), 0, GCornerNone);
    // down right
    graphics_fill_rect(ctx, GRect(144 - i, 96 + y, 1, 60 - y), 0, GCornerNone);
  }
}
static void update_line_proc(Layer *this_layer, GContext *ctx)
{
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, 144, 36), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0, 156, 144, 12), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0, 0, 12, 168), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(132, 0, 12, 168), 0, GCornerNone);

  draw_circle(ctx);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(71, 30, 1, pox_y), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(71, 30, 63, 1), 0, GCornerNone);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(70, 30, 1, pox_y), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(72, 31, 1, pox_y), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(71, 30 + pox_y + 1, 1, 1), 0, GCornerNone);
}
#elif PBL_PLATFORM_BASALT
static void update_line_proc(Layer *this_layer, GContext *ctx)
{

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(71, 30, 1, pox_y), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(71, 30, 63, 1), 0, GCornerNone);

  // send_int(2,0);
}

#elif PBL_PLATFORM_CHALK
static void update_line_proc(Layer *this_layer, GContext *ctx)
{

  graphics_context_set_fill_color(ctx, GColorWhite);
  // map line
  graphics_fill_rect(ctx, GRect(71 + 19, 39, 1, pox_y - 9), 0, GCornerNone);
  // box
  // vertical
  graphics_fill_rect(ctx, GRect(63, 18, 54, 1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(63, 39, 54, 1), 0, GCornerNone);
  // horizontal
  graphics_fill_rect(ctx, GRect(63, 18, 1, 22), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(117, 18, 1, 22), 0, GCornerNone);

  graphics_context_set_fill_color(ctx, GColorBlack);

  // black lines (change location)
  // top
  graphics_fill_rect(ctx, GRect(0, 0, 180, 10), 0, GCornerNone);
  // bottom
  graphics_fill_rect(ctx, GRect(160, 0, 20, 180), 0, GCornerNone);
  // left
  graphics_fill_rect(ctx, GRect(0, 0, 20, 180), 0, GCornerNone);
  // right
  graphics_fill_rect(ctx, GRect(0, 170, 180, 10), 0, GCornerNone);

  // send_int(2,0);
}
#endif

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  update_time();
}

void sync_tuple_changed_callback(const uint32_t key,
                                 const Tuple *new_tuple, const Tuple *old_tuple, void *context)
{

  switch (key)
  {
  case LONGITUDE:
    pox_x = new_tuple->value->uint32;
    break;
  case LATITUDE:
    pox_y = new_tuple->value->uint32;
    break;
  }
  update_map();
}

static void main_window_load(Window *window)
{

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // location init
  Tuplet initial_values[] = {
      TupletInteger(LATITUDE, 0),
      TupletInteger(LONGITUDE, 0)};

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
                sync_tuple_changed_callback, sync_error_callback, NULL);

  // init lines
  s_layer_lines = layer_create(bounds);
  layer_set_update_proc(s_layer_lines, update_line_proc);

  generate_map(window_layer, bounds);
}

static void main_window_unload(Window *window)
{
}

static void init(void)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Start init ");

  // Register with TickTimerService
  s_main_window = window_create();

#ifndef PBL_PLATFORM_APLITE
  sys_locale = setlocale(LC_ALL, "");
#endif

  pox_x = 165;
  pox_y = 0;
  locOK = 0;
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload});

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // message location open
  app_message_open(64, 64);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  // Make sure the time is displayed from the start
  update_time();
}

static void deinit(void)
{
  window_destroy(s_main_window);
  gbitmap_destroy(s_bitmap_map);
  gbitmap_destroy(s_bitmap_cutted_map);
  gbitmap_destroy(s_bitmap_sea);

#ifndef PBL_PLATFORM_APLITE
  gbitmap_destroy(s_bitmap_circle);
#endif
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}
