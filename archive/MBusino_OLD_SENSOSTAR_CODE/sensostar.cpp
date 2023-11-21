#include "sensostar.h"

Sensostar::Sensostar(WriteByteFunc write, TimestampMillisecondsFunc getMillis) {
    _write = write;
    _getMillis = getMillis;
    meter_address = 0xFE;   /* broadcast */
    request_time = 0;
    fcb = 0;
}

uint8_t Sensostar::decode_vif(uint8_t *buf, struct sensostar_data *result) {
    int32_t intval;

    switch(buf[0] & 0x0F) {
        /* DIF (buf[0]) encodes data length + data type (e.g. total energy vs. last billing energy) */
        /* inside, check VIF (buf[1]) but also have to check full DIF to get only one data point */
        case 0x00:
            /* no data */
            return 2;
            break;
        case 0x01:
            if (buf[1] == 0xFD && buf[2] == 0x17) {
                result->error = buf[3];
            }
            return 4;
            break;
        case 0x02:
            intval = buf[3] * 256 + buf[2];
            if (buf[1] == 0x5B && buf[0] == 0x02) {
                result->flow_temperature = intval;
            } else if (buf[1] == 0x5F && buf[0] == 0x02) {
                result->return_temperature = intval;
            } else if (buf[1] == 0x61 && buf[0] == 0x02) {
                result->flow_return_difference_temperature = intval;
            }
            return 4;
            break;
        case 0x03:
            return 6;
            break;
        case 0x04:
            intval = buf[2] | (buf[3] << 8) | (buf[4] << 16) | (buf[5] << 24);
            if (buf[1] == 0x06 && buf[0] == 0x04) {
                result->total_heat_energy = intval;
            } else if (buf[1] == 0x2B && buf[0] == 0x04) {
                result->power = intval;
            } else if (buf[1] == 0x3B && buf[0] == 0x04) {
                result->flow_speed = intval;
            }
            return 6;
            break;
        default:
        return 2;
        break;
    }
}

/*  data from meter. This method is specifically crafted to work with the Engelmann Sensostar U.
    Other meters might need more functionality. */
uint8_t Sensostar::decode_meter_reading(struct sensostar_data *result) {
    if ((rxBuf[0] & 0xCF) == 0x08) {
        /* RSP_UD */
        if (rxBuf[2] == 0x72) {
            /* variable data structure */
            /* 3: 4 byte ID
               7: 2 byte manufacturer
               9: 1 byte generation
               10: 1 byte medium
               11: 1 byte access counter (modulo 256 number of RSP_UD commands)
               12: 1 byte status (0: no error)
               13: 2 byte signature */
            uint8_t pos = 15;
            while (pos < rxBufPos) {
                pos += decode_vif(rxBuf + pos, result);
            }
            return 1;
        }
    }
    return 0;

}

uint8_t Sensostar::process(uint8_t c, struct sensostar_data *result) {
    uint8_t res = 0;
    uint8_t sum = 0;
    switch(rxState) {
        case 0:
            if (c == 0x68) {
                ++rxState;
            }
            break;
        case 1:
            rxLen = c;
            ++rxState;
            break;
        case 2:
            if (rxLen != c) {
                /* duplicate length mismatch: reset */
                rxState = 0;
            } else {
                ++rxState;
            }
            break;
        case 3:
            if (c == 0x68) {
                ++rxState;
                rxBufPos = 0;
            } else {
                rxState = 0;
            }
            break;
        case 4:
            if (rxBufPos >= SENSOSTAR_RX_BUF_SIZE) {
                /* packet too long: skip */
                rxState = 0;
            }
            rxBuf[rxBufPos] = c;
            ++rxBufPos;
            if (rxBufPos == rxLen) {
                rxState = 5;
            }
            break;
        case 5:
            for (int i = 0; i < rxBufPos; ++i) {
                sum += rxBuf[i];
            }
            if (c != sum) {
                rxState = 0;
            } else {
                rxState = 6;
            }
            break;
        case 6:
            if (c == 0x16) {
                res = decode_meter_reading(result);
                fcb ^= 1;
            }
            rxState = 0;
            break;
        default:
        break;
    }
    return res;
}

void Sensostar::request() {
    uint32_t timestamp = _getMillis();
    if (timestamp > request_time + SENSOSTAR_REPLY_TIMEOUT) {
        /* REQ_UD2 */
        _write(0x10);
        _write(0x5B | (fcb << 5));
        _write(meter_address);
        _write((0x5B + meter_address + (fcb << 5)) % 256);
        _write(0x16);
        request_time = _getMillis();
    }
}
