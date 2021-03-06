#include "pebble.h"
#include "num2words-en.h"

#define DEBUG 0
#define BUFFER_SIZE 44

static Window *window;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

typedef struct {
	TextLayer *currentLayer;
	TextLayer *nextLayer;	
	PropertyAnimation *currentAnimation;
	PropertyAnimation *nextAnimation;
} Line;


static Line line1;
static Line line2;


static struct tm *t;
static GFont lightFont;
static GFont boldFont;

static char line1Str[2][BUFFER_SIZE];
static char line2Str[2][BUFFER_SIZE];

//int pauseAnimation;

// Animation handler
static void animationStoppedHandler(struct Animation *animation, bool finished, void *context)
{

  property_animation_destroy((PropertyAnimation *) context);
}

// Animate line
static void makeAnimationsForLayers(Line *line, TextLayer *current, TextLayer *next)
{
 // if (pauseAnimation){return;}
	GRect fromRect = layer_get_frame(text_layer_get_layer(next));
	GRect toRect = fromRect;
	fromRect.origin.x=144;
  toRect.origin.x = 0;
	line->nextAnimation = property_animation_create_layer_frame(text_layer_get_layer(next), &fromRect, &toRect);
  animation_set_duration((Animation *)line->nextAnimation, 400);
  animation_set_curve((Animation *)line->nextAnimation, AnimationCurveEaseOut);
  animation_set_handlers((Animation *)line->nextAnimation, (AnimationHandlers){
    .stopped = (AnimationStoppedHandler)animationStoppedHandler}, line->nextAnimation);
	animation_schedule((Animation *)line->nextAnimation);
	GRect fromRect2 = layer_get_frame(text_layer_get_layer(current));
	GRect toRect2 = fromRect2;
	fromRect2.origin.x = 0;
  toRect2.origin.x = -144;
	line->currentAnimation = property_animation_create_layer_frame(text_layer_get_layer(current), &fromRect2, &toRect2);
  animation_set_duration((Animation *)line->currentAnimation, 400);
  animation_set_curve((Animation *)line->currentAnimation, AnimationCurveEaseOut);
	animation_set_handlers((Animation *)line->currentAnimation, (AnimationHandlers) {
		.stopped = (AnimationStoppedHandler)animationStoppedHandler
	}, line->currentAnimation);
	animation_schedule((Animation *)line->currentAnimation);
}

// Update line
static void updateLineTo(Line *line, char lineStr[2][BUFFER_SIZE], char *value)
{
	TextLayer *next, *current;
	
	GRect rect = layer_get_frame(text_layer_get_layer(line->currentLayer));
	current = (rect.origin.x == 0) ? line->currentLayer : line->nextLayer;
	next = (current == line->currentLayer) ? line->nextLayer : line->currentLayer;
	
	// Update correct text only
	if (current == line->currentLayer) {
		memset(lineStr[1], 0, BUFFER_SIZE);
		memcpy(lineStr[1], value, strlen(value));
		text_layer_set_text(next, lineStr[1]);
	} else {
		memset(lineStr[0], 0, BUFFER_SIZE);
		memcpy(lineStr[0], value, strlen(value));
		text_layer_set_text(next, lineStr[0]);
	}
	
	makeAnimationsForLayers(line, current, next);
}

// Check to see if the current line needs to be updated
static bool needToUpdateLine(Line *line, char lineStr[2][BUFFER_SIZE], char *nextValue)
{
	char *currentStr;
	GRect rect = layer_get_frame(text_layer_get_layer(line->currentLayer));
	currentStr = (rect.origin.x == 0) ? lineStr[0] : lineStr[1];

	if (memcmp(currentStr, nextValue, strlen(nextValue)) != 0 ||
		(strlen(nextValue) == 0 && strlen(currentStr) != 0)) {
		return true;
	}
	return false;
}

// Update screen based on new time
static void display_time(struct tm *t)
{
	// The current time text will be stored in the following 2 strings
	char textLine1[BUFFER_SIZE];
	char textLine2[BUFFER_SIZE];
	
	int ShowDie;
  
	ShowDie = time_to_2words(t->tm_hour, t->tm_min, textLine1, textLine2, BUFFER_SIZE);	
  
  layer_set_hidden(bitmap_layer_get_layer(s_background_layer), ShowDie);
  
	if (needToUpdateLine(&line1, line1Str, textLine1)) {
		updateLineTo(&line1, line1Str, textLine1);	
	}

  if (needToUpdateLine(&line2, line2Str, textLine2)) {
		updateLineTo(&line2, line2Str, textLine2);	
	}
}

// Update screen without animation first time we start the watchface
static void display_initial_time(struct tm *t)
{
	time_to_2words(t->tm_hour, t->tm_min, line1Str[0], line2Str[0], BUFFER_SIZE);
	text_layer_set_text(line1.currentLayer, line1Str[0]);
	text_layer_set_text(line2.currentLayer, line2Str[0]);
	  
}

// Debug methods. For quickly debugging enable debug macro on top to transform the watchface into
// a standard app and you will be able to change the time with the up and down buttons
#if DEBUG

static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  t->tm_min += 1;
	if (t->tm_min >= 60) {
		t->tm_min = 0;
		t->tm_hour += 1;
		
		if (t->tm_hour >= 24) {
			t->tm_hour = 0;
		}
	}
	display_time(t);
}


static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  t->tm_min -= 1;
	if (t->tm_min < 0) {
		t->tm_min = 59;
		t->tm_hour -= 1;
	}
	display_time(t);
}

static void click_config_provider(ClickRecognizerRef recognizer, void *context) {
	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler)up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler)down_single_click_handler);
}

#endif

// Configure the first line of text
static void configureBoldLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, boldFont);
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentLeft);
}

// Configure for the 2nd and 3rd lines
static void configureLightLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, lightFont);
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentLeft);
}

// Time handler called every minute by the system
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	t = tick_time;
  display_time(tick_time);
}

/*
void focus_handler(bool in_focus) {
  if (in_focus) {
    pauseAnimation = 0;
    display_time(t);
  } else {
    pauseAnimation = 1;
  }
}
*/

static void init() {
//  pauseAnimation = 0;
  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);


  
  // app_focus_service_subscribe(focus_handler);
  
  // seed random
  srand(time(NULL));
  
  
  //Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_D20);
  s_background_layer = bitmap_layer_create(GRect(22, 47, 102, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_set_hidden(bitmap_layer_get_layer(s_background_layer), 1);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  
	// Custom fonts
	lightFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_gotham_light_31));
	boldFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_gotham_bold_36));

	// 1st line layers
	line1.currentLayer = text_layer_create(GRect(0, 18, 144, 50));
	line1.nextLayer = text_layer_create(GRect(144, 18, 144, 50));
	configureBoldLayer(line1.currentLayer);
	configureBoldLayer(line1.nextLayer);

	// 2nd layers
	line2.currentLayer = text_layer_create(GRect(0, 55, 144, 50));
	line2.nextLayer = text_layer_create(GRect(144, 55, 144, 50));
	configureLightLayer(line2.currentLayer);
	configureLightLayer(line2.nextLayer);

	
	// Configure time on init
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
	display_initial_time(t);

	// Load layers
	Layer *window_layer = window_get_root_layer(window);
	layer_add_child(window_layer, text_layer_get_layer(line1.currentLayer));
	layer_add_child(window_layer, text_layer_get_layer(line1.nextLayer));
	layer_add_child(window_layer, text_layer_get_layer(line2.currentLayer));
	layer_add_child(window_layer, text_layer_get_layer(line2.nextLayer));
	
	#if DEBUG
	// Button functionality
	window_set_click_config_provider(window, (ClickConfigProvider) click_config_provider);
	#endif

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

            
static void deinit() {
  tick_timer_service_unsubscribe();
  app_focus_service_unsubscribe();
	window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}