#include "applications/game.h"

#define reliable true
#define SEND_FEEDBACK true

DECLARE_FIFO(addresses, 20)
;

struct game_color_t game_colors[] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1,
        1, 0 }, { 0, 1, 1 }, { 1, 0, 1 }, { 1, 1, 1 } };

static struct {
  bool has_color;
  bool is_color_active;
  uint8_t my_color;
  uint8_t intensity;
  uint8_t app_packet_size;
  bool reliable_communication;
} settings;

struct local_packet {
  uint16_t address;
  uint8_t data[LOCAL_PACKET_SIZE];
};

extern uint16_t my_address;

void get_colors(uint8_t gamecolor, uint8_t *red, uint8_t *green, uint8_t *blue) {
  struct game_color_t color;
  if (gamecolor > 6) {
    for (uint8_t i = 0; i < 3; i++) {
      set_all_leds(LIGHT_WHITE);
      delay_ms(100);
      set_all_leds(LIGHT_OFF);
      delay_ms(100);
      set_all_leds(LIGHT_BLUE);
      delay_ms(100);
    }
  }
  
  color = game_colors[gamecolor];
  *red = color.red * settings.intensity;
  *green = color.green * settings.intensity;
  *blue = color.blue * settings.intensity;
}
void show_color(uint8_t gamecolor) {
  uint8_t red;
  uint8_t green;
  uint8_t blue;

  settings.is_color_active = true;
  get_colors(gamecolor, &red, &green, &blue);
  set_all_leds(red, green, blue);
}

void show_color_one_led(uint8_t gamecolor, uint8_t led) {
  uint8_t red, green, blue;

  settings.is_color_active = true;
  get_colors(gamecolor, &red, &green, &blue);
  set_one_led(red, green, blue, led);
}

void set_color(uint8_t gamecolor) {
  settings.my_color = gamecolor;
  settings.has_color = true;
  settings.is_color_active = true;
  settings.intensity = 8;
  show_color(gamecolor);
}

void set_color_one_led(uint8_t gamecolor, uint8_t led) {
  uint8_t red, green, blue;

  show_color_one_led(gamecolor, led);
  settings.my_color = gamecolor;
  settings.has_color = true;
  settings.is_color_active = true;
  settings.intensity = 8;

}
uint8_t get_current_intensity() {
  return settings.intensity;
}

void define_color(uint8_t gamecolor) {
  settings.my_color = gamecolor;
  settings.has_color = true;
}

void set_faded_color(uint8_t gamecolor, uint8_t intensity) {
  settings.my_color = gamecolor;
  settings.has_color = true;
  settings.is_color_active = true;
  settings.intensity = intensity;
  show_color(gamecolor);
}

void set_faded_led(uint8_t gamecolor, uint8_t led, uint8_t intensity) {
  settings.my_color = gamecolor;
  settings.has_color = true;
  settings.is_color_active = true;
  settings.intensity = intensity;
  show_color_one_led(gamecolor, led);
}

bool has_color() {
  return settings.has_color;
}

void turn_on() {
  show_color(settings.my_color);
  settings.is_color_active = true;
}

void turn_off() {
  set_all_leds(LIGHT_OFF);
  settings.is_color_active = false;
}

void clear_color() {
  turn_off();
  settings.has_color = false;
}

bool is_color_active() {
  return settings.is_color_active;
}

uint8_t my_color() {
  return settings.my_color;
}

void send_command(uint16_t address, void *data) {
  send_packet(address, data, settings.app_packet_size, settings.reliable_communication);
}

void send_command_all(void *data) {
  send_command(LOCAL_BROADCAST, data);
  send_command(my_address, data);
}

void send_command_global(void *data) {
  send_command(BROADCAST, data);
  send_command(my_address, data);
}

void send_command_all_neighbours(void *data) {
  for (uint8_t i = 0; i < 4; i++) {
    send_command(NEIGHBOUR_MASK + i, data);
  }
}

void game_init(uint8_t packet_size) {
  settings.has_color = false;
  settings.my_color = 0;
  settings.intensity = 8;
  settings.app_packet_size = packet_size;
  settings.reliable_communication = reliable;
  fifo_init(addresses, 20);
}

void game_update() {
  if (is_master()) {
    if (!fifo_full(addresses)) {
      fifo_put(addresses, get_random_address() & 0xFF);
    }
  }
}

uint16_t get_random_buffered_address() {
  if (is_master()) {
    if (fifo_count(addresses)) {
      return my_master_addr() + fifo_get(addresses);
    } else {
      return get_random_address();
    }
  } else {
    return my_master_addr();
  }
}

void clear_random_address_buffer() {
  fifo_init(addresses, 20);
}

struct game_color_t * get_game_color(uint8_t game_color_index) {
  return &game_colors[game_color_index];
}

void send_feedback(uint8_t point, uint8_t miss, uint8_t duration,
    uint8_t winner, uint8_t level) {
#ifdef SEND_FEEDBACK 
  if (is_master()) {
    uint8_t data[7];

    data[0] = 25; //info for the java program
    data[1] = point;
    data[2] = miss;
    data[3] = duration;
    data[4] = winner;
    data[5] = level;
    data[6] = TOP_calc_size_of_platform();
    //send_packet(BROADCAST, data, 6, true);
    NWL_send_new_datagram(RMT_FEEDBACK, BROADCAST, data, 7, true);
  }
#endif
}

void send_game_feedback(uint8_t cmd, uint8_t point, uint8_t miss, uint8_t duration,
    uint8_t winner, uint8_t level) {
#ifdef SEND_FEEDBACK 
  if (is_master()) {
    uint8_t data[7];

    data[0] = cmd; //info for the java program
    data[1] = point;
    data[2] = miss;
    data[3] = duration;
    data[4] = winner;
    data[5] = level;
    data[6] = TOP_calc_size_of_platform();
    //send_packet(BROADCAST, data, 6, true);
    NWL_send_new_datagram(RMT_FEEDBACK, BROADCAST, data, 7, true);
  }
#endif
}

