#include <pebble_worker.h>
#include "../../common/c/logging.h"

static bool s_sleeping;

static void prv_health_event_handler(HealthEventType event, void *context) {
    logf();
    if (event == HealthEventSignificantUpdate) {
        prv_health_event_handler(HealthEventSleepUpdate, context);
    } else if (event == HealthEventSleepUpdate || (event == HealthEventMovementUpdate && s_sleeping)) {
        HealthActivityMask activities = health_service_peek_current_activities();
        bool sleeping = ((activities & HealthActivitySleep) || (activities & HealthActivityRestfulSleep)) && event != HealthEventMovementUpdate;
        if (sleeping != s_sleeping) {
            s_sleeping = sleeping;
            worker_launch_app();
        }
    }
}

#ifdef DEBUG
static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    logf();
    if (tick_time->tm_min % 5 == 0) worker_launch_app();
}
#endif

static void prv_init(void) {
    logf();
    HealthActivityMask activities = health_service_peek_current_activities();
    s_sleeping = (activities & HealthActivitySleep) || (activities & HealthActivityRestfulSleep);
    health_service_events_subscribe(prv_health_event_handler, NULL);

#ifdef DEBUG
    tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
#endif
}

static void prv_deinit(void) {
    logf();
#ifdef DEBUG
    tick_timer_service_unsubscribe();
#endif
    health_service_events_unsubscribe();
}

int main(void) {
    logf();
    prv_init();
    worker_event_loop();
    prv_deinit();
}
