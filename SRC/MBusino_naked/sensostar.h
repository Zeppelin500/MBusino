#include <stdint.h>
#include <functional>

typedef std::function<size_t(uint8_t)> WriteByteFunc;
typedef std::function<uint32_t(void)> TimestampMillisecondsFunc;

#define SENSOSTAR_REPLY_TIMEOUT 1000
#define SENSOSTAR_RX_BUF_SIZE 255

struct sensostar_data {
    int16_t flow_temperature;
    int16_t return_temperature;
    int16_t flow_return_difference_temperature;
    int32_t total_heat_energy;
    int32_t flow_speed;
    int32_t power;
    uint8_t error;
};

class Sensostar {
    public:
        Sensostar(WriteByteFunc write, TimestampMillisecondsFunc getMillis);
        uint8_t process(uint8_t c, struct sensostar_data *result); /* returns 1 when new data is available */
        void request();
        struct sensostar_data latest_data;
    
    private:
        WriteByteFunc _write;
        TimestampMillisecondsFunc _getMillis;
        uint8_t meter_address;
        uint32_t request_time;
        uint8_t rxBuf[SENSOSTAR_RX_BUF_SIZE];
        uint8_t rxBufPos;
        uint8_t rxState;
        uint8_t rxLen;
        uint8_t decode_meter_reading(struct sensostar_data *result);
        uint8_t decode_vif(uint8_t *buf, struct sensostar_data *result);
        uint8_t fcb;
};