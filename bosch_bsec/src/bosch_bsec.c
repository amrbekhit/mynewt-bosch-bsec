#include "syscfg/syscfg.h"
#include "os/os.h"
#include "hal/hal_timer.h"

#include "bosch_bsec/bosch_bsec.h"
#include "bosch_bsec/bsec_integration.h"

#if MYNEWT_VAL(BOSCH_BSEC_CONFIG) == 0
    #include "../config/generic_18v_3s_4d/bsec_serialized_configurations_iaq.c"
#elif MYNEWT_VAL(BOSCH_BSEC_CONFIG) == 1
    #include "../config/generic_18v_3s_28d/bsec_serialized_configurations_iaq.c"
#elif MYNEWT_VAL(BOSCH_BSEC_CONFIG) == 2
    #include "../config/generic_18v_300s_4d/bsec_serialized_configurations_iaq.c"
#elif MYNEWT_VAL(BOSCH_BSEC_CONFIG) == 3
    #include "../config/generic_18v_300s_28d/bsec_serialized_configurations_iaq.c"
#elif MYNEWT_VAL(BOSCH_BSEC_CONFIG) == 4
    #include "../config/generic_33v_3s_4d/bsec_serialized_configurations_iaq.c"
#elif MYNEWT_VAL(BOSCH_BSEC_CONFIG) == 5
    #include "../config/generic_33v_3s_28d/bsec_serialized_configurations_iaq.c"
#elif MYNEWT_VAL(BOSCH_BSEC_CONFIG) == 6
    #include "../config/generic_33v_300s_4d/bsec_serialized_configurations_iaq.c"
#elif MYNEWT_VAL(BOSCH_BSEC_CONFIG) == 7
    #include "../config/generic_33v_300s_28d/bsec_serialized_configurations_iaq.c"
#else 
    #error Must specify a valid BSEC config (see BOSCH_BSEC_CONFIG in syscfg)
#endif

#if MYNEWT_VAL(BOSCH_BSEC_SAVE_STATE)
#include "fs/fs.h"
#endif

/******************************************
 * BSEC task definitions
 * ***************************************/
#define BSEC_TASK_PRI   MYNEWT_VAL(BOSCH_BSEC_TASK_PRIORITY)
#define BSEC_TASK_STACK_SIZE  MYNEWT_VAL(BOSCH_BSEC_TASK_STACK_SIZE)
static struct os_task bsec_task;
OS_TASK_STACK_DEFINE(bsec_task_stack, BSEC_TASK_STACK_SIZE);

static output_ready_fct output_cb = NULL;

static void delay_ms(uint32_t period)
{
	uint32_t ticks;

	os_time_ms_to_ticks(period, &ticks);
	os_time_delay(ticks);
}

static void state_save(const uint8_t *state_buffer, uint32_t length)
{
#if MYNEWT_VAL(BOSCH_BSEC_SAVE_STATE)
    int rc;
    struct fs_file *file;
    
    rc = fs_open(MYNEWT_VAL(BOSCH_BSEC_SAVE_STATE_FILE), FS_ACCESS_WRITE | FS_ACCESS_TRUNCATE, &file);
    if (rc == 0) {
        fs_write(file, state_buffer, length);
        fs_close(file);
    }
#endif
}

static uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer)
{
    uint32_t bytes_read = 0;
#if MYNEWT_VAL(BOSCH_BSEC_SAVE_STATE)
    int rc;
    struct fs_file *file;
    
    rc = fs_open(MYNEWT_VAL(BOSCH_BSEC_SAVE_STATE_FILE), FS_ACCESS_READ, &file);
    if (rc == 0) {
        fs_read(file, n_buffer, state_buffer, &bytes_read);
        fs_close(file);
    }
#endif
    return bytes_read;
}

static uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer)
{
    memcpy(config_buffer, bsec_config_iaq, sizeof(bsec_config_iaq));
    return sizeof(bsec_config_iaq);
}

int64_t get_timestamp_us(void) 
{
    return (int64_t)hal_timer_read(MYNEWT_VAL(BOSCH_BSEC_TIMER)) * hal_timer_get_resolution(MYNEWT_VAL(BOSCH_BSEC_TIMER)) / 1000.0;
}

static void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, float humidity,
     float pressure, float raw_temperature, float raw_humidity, float gas, bsec_library_return_t bsec_status)
{
    if (output_cb) {
        output_cb(timestamp, iaq, iaq_accuracy, temperature, humidity, pressure,
                raw_temperature, raw_humidity, gas, bsec_status);
    }
}

static void bsec_task_func(void *arg)
{
    int rc;
    struct sensor *bme680;

#if MYNEWT_VAL(BOSCH_BSEC_TIMER_INIT)
    hal_timer_config(MYNEWT_VAL(BOSCH_BSEC_TIMER), MYNEWT_VAL(BOSCH_BSEC_TIMER_FREQ));
#endif

    bme680 = sensor_mgr_find_next_bydevname(MYNEWT_VAL(BOSCH_BSEC_SENSOR_DEV), NULL);
    assert(bme680);
    
    rc = bsec_iot_init(BSEC_SAMPLE_RATE_LP, MYNEWT_VAL(BOSCH_BSEC_TEMPERATURE_OFFSET), bme680, delay_ms, state_load, config_load);
    assert(rc == 0);
    
    bsec_iot_loop(delay_ms, get_timestamp_us, output_ready, state_save, MYNEWT_VAL(BOSCH_BSEC_SAVE_STATE_INTERVAL));
}

void bosch_bsec_output_cb(output_ready_fct output_ready)
{
    output_cb = output_ready;
}

void bosch_bsec_init(void)
{
    int rc;

    rc = os_task_init(&bsec_task, "bsec", bsec_task_func, NULL, BSEC_TASK_PRI, 
                        OS_WAIT_FOREVER, bsec_task_stack, BSEC_TASK_STACK_SIZE);
    assert(rc == 0);
}