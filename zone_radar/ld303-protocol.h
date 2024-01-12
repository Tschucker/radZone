#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// register dump @ 9600 bps: 108 bytes
// 0    FA FB 00 64 FA FB header
// 6    00 01 (0xB1) working mode
// 8    00 60 (0xD4) baud rate
// 10   00 01 () XXXX
//      0B B8 (0xB3)= 3000, fitting coefficient
//      07 A5 (0xB4)= 1957, offset correction
// 16   00 70 firmware version nr
//      00 06 XXX
// 20   00 0A 
//      00 01
//      00 0C
//      00 00
//      2E E0 = 12000
// 30   00 00
//      00 50
//      00 00
//      00 0A
//      00 00
// 40   00 05
//      13 88 = 5000
//      00 08
//      00 B9
//      00 00
// 50   00 00
//      00 00
//      00 00
// 56   00 0B signal interval
//      00 01 XX
//      00 00 XX
// 62   01 11 fretting threshold
//      00 00 XXXX
// 66   00 0A proportion statistics
// 68   01 23 percentage of invalid distance
// 70   00 22 percentage
// 72   00 0A extreme value statistics
// 74   00 02 extremum filtering time
// 76   00 09 (0xE9) number of swipes
// 78   00 06 (0xF6?) agreement type
//      00 00 XXXX
// 82   00 00 output target
// 84   03 E8 = 1000
// 86   01 90 sensitivity
//      00 01 
//      00 64 
//      01 2C 
// 94   00 02 data response time
//      00 FA (0xe5) = 250 maximum detection distance
//      00 0A (0xe0) = 10 minimum detection distance 
// 100  00 00 (0xd2) close treatment
// 102  00 EF marker
// 104  0E 01 check
// 106  AA 55 footer

// command definitions
typedef enum {
    CMD_OPERATING_MODE = 0xB1,      // 0 = sensitive, 1 = stable
    CMD_FITTING_COEFFICIENT = 0xB3, // unit 0.001
    CMD_OFFSET_CORRECTION = 0xB4,   // unit 0.01 cm
    CMD_DELAY_TIME = 0xD1,          // how long the 'presence' bit remain high after detection (ms)
    CMD_CLOSE_TREATMENT = 0xD2,     // 0 = keep last measured distance, 1 = clear distance result
    CMD_MEASUREMENT = 0xD3,         // read measurement?
    CMD_BAUD_RATE = 0xD4,           // unit 100 bps, e.g. 96 for 9600 bps, activated on next power-up
    CMD_TRIGGER_THRESHOLD = 0xD5,   // unit 'k'
    CMD_OUTPUT_TARGET = 0xD9,       // 0 = nearest, 1 = maximum goal
    CMD_SIGNAL_INTERVAL = 0xDA,     // range 5-20, unit 40 ms
    CMD_RESET = 0xDE,               // reset, 0, immediate
    CMD_MIN_DETECTION_DIST = 0xE0,  // unit 'cm'
    CMD_SENSITIVITY = 0xE1,         // unit 'k', 60-2000 (default 300)
    CMD_MAX_DETECTION_DIST = 0xE5,  // unit 'cm'
    CMD_REPORT_INTERVAL = 0xE6,     // range 0-20, unit 40 ms
    CMD_EXTREME_VALUE_STATS = 0xE7, // unit 'times'
    CMD_EXTREME_FILTER_TIMES = 0xE8,// unit 'times'
    CMD_NUMBER_OF_SWIPES = 0xE9,    // unit 'times'
    CMD_PROTOCOL_TYPE = 0xF6,       // 0 = ASCII, 1 = hex protocol 1, 6 = standard protocol (query), 7 = automatic
    CMD_PROPORTION_STATISTIC = 0xF9,// 0-100
    CMD_INVALID_DISTANCE = 0xFA,    // unit 'cm'
    CMD_PERCENTAGE = 0xFB,          // unit '%'
    CMD_FIRMWARE_UPGRADE = 0xDF,    // 1
    CMD_QUERY_PARAMETERS = 0xFE     // 0, query all parameters
} ld303_cmd_t;

// parsing state
typedef enum {
    STATE_HEADER_55,
    STATE_HEADER_A5,
    STATE_LEN,
    STATE_DATA,
    STATE_CHECK
} state_t;

class LD303Protocol {

private:
    state_t _state;
    uint8_t _sum;
    uint8_t _buf[32];
    uint8_t _len;
    uint8_t _idx;

public:
    LD303Protocol();

    // builds a parameter setting command
    size_t build_command(uint8_t *buf, uint8_t cmd, uint16_t param);

    // queries the module for measurement data (param usually 0xD3)
    size_t build_query(uint8_t *buf, const uint8_t *data, size_t len);

    // processes received data, returns true if measurement data was found
    bool process_rx(uint8_t c);

    // call this when process_rx returns true, copies received data into buffer, returns length
    size_t get_data(uint8_t *data);

};

