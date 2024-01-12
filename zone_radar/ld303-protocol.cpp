#include <string.h>

#include "ld303-protocol.h"

LD303Protocol::LD303Protocol(void)
{
    _state = STATE_HEADER_55;
    _sum = 0;
    memset(_buf, 0, sizeof(_buf));
    _len = 0;
    _idx = 0;
}

// protocol 6
// 55 5A 02 D3 84  open/close, "fixed query command"
//
// BA AB .... set parameter
// 
// change query mode: BAAB 00 F6 0007 00 55BB = send cmd F6 with parameter 0007
// -> reports data continuously?

size_t LD303Protocol::build_query(uint8_t *buf, const uint8_t *data, size_t len)
{
    int idx = 0;
    buf[idx++] = 0x55;  // header
    buf[idx++] = 0x5A;
    buf[idx++] = len + 1;
    for (size_t i = 0; i < len; i++) {
        buf[idx++] = data[i];
    }
    uint8_t sum = 0;
    for (int i = 0; i < idx; i++) {
        sum += buf[i];
    }
    buf[idx++] = sum;

    return idx;
}

size_t LD303Protocol::build_command(uint8_t *buf, uint8_t cmd, uint16_t param)
{
    int idx = 0;
    buf[idx++] = 0xBA;  // header
    buf[idx++] = 0xAB;
    buf[idx++] = 0x00;  // address
    buf[idx++] = cmd;
    buf[idx++] = (param >> 8) & 0xFF;
    buf[idx++] = param & 0xFF;
    buf[idx++] = 0;     // checksum
    buf[idx++] = 0x55;
    buf[idx++] = 0xBB;

    return idx;
}

// message from radar: "uplink data, (sent by module)" 
// 55       A5      0A      XX      XX      XX      XX      XX      XX      XX      XX      XX      XX
// header           len     adress  distance        rsvd    state   signal strength micro   closed  check

bool LD303Protocol::process_rx(uint8_t c)
{
    switch (_state) {
    case STATE_HEADER_55:
        _sum = c;
        if (c == 0x55) {
            _state = STATE_HEADER_A5;
        }
        break;
    case STATE_HEADER_A5:
        _sum += c;
        if (c == 0xA5) {
            _state = STATE_LEN;
        } else {
            _state = STATE_HEADER_55;
        }
        break;
    case STATE_LEN:
        _sum += c;
        if ((c > 0) && (c < sizeof(_buf))) {
            _len = c - 1;
            _idx = 0;
            _state = _len > 0 ? STATE_DATA : STATE_CHECK;
        } else {
            _state = STATE_HEADER_55;
        }
        break;
    case STATE_DATA:
        _sum += c;
        _buf[_idx++] = c;
        if (_idx == _len) {
            _state = STATE_CHECK;
        }
        break;
    case STATE_CHECK:
        _state = STATE_HEADER_55;
        return (c == _sum);

    default:
        _state = STATE_HEADER_55;
        break;
    }
    return false;
}

size_t LD303Protocol::get_data(uint8_t *data)
{
    memcpy(data, _buf, _len);
    return _len;
}


