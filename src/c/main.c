#include <pebble.h>
#include "../../common/c/logging.h"

#define PERSIST_KEY_WAKEUP_ID 0x00

static Window *s_window;

static void prv_wakeup_handler(WakeupId wakeup_id, int32_t cookie) {
    logf();
    AppWorkerResult result = app_worker_launch();
    logd("app_worker_launch() = %d", result);
}

static void prv_inbox_received(DictionaryIterator *iterator, void *context) {
    logf();
    Tuple *tuple = dict_find(iterator, MESSAGE_KEY_APP_READY);
    if (tuple) {
        HealthActivityMask activities = health_service_peek_current_activities();
        bool sleeping = (activities & HealthActivitySleep) || (activities & HealthActivityRestfulSleep);

        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        dict_write_uint8(iter, MESSAGE_KEY_SLEEPING, sleeping);
        app_message_outbox_send();
    }

    tuple = dict_find(iterator, MESSAGE_KEY_MSG_PROCESSED);
    if (tuple) window_stack_remove(s_window, false);
}

static void prv_window_load(Window *window) {
    logf();
    window_set_background_color(window, GColorClear);

    size_t buffer_size = dict_calc_buffer_size(1, sizeof(bool));
    app_message_register_inbox_received(prv_inbox_received);
    app_message_open(buffer_size, buffer_size);
}

static void prv_window_unload(Window *window) {
    logf();
    app_message_deregister_callbacks();
}

static void prv_init(void) {
    logf();
    AppLaunchReason reason = launch_reason();
    logd("launch_reason() = %d", reason);
    if (reason == APP_LAUNCH_WORKER) {
        bool connected = connection_service_peek_pebble_app_connection();
        if (connected) {
            s_window = window_create();
            window_set_window_handlers(s_window, (WindowHandlers) {
                .load = prv_window_load,
                .unload = prv_window_unload
            });
            window_stack_push(s_window, false);
        }
    } else {
        prv_wakeup_handler(0, 0);

        WakeupId wakeup_id = persist_read_int(PERSIST_KEY_WAKEUP_ID);
        logd("persist_read_int() = %d", wakeup_id);
        if (wakeup_id > 0 && wakeup_query(wakeup_id, NULL)) return;

        wakeup_service_subscribe(prv_wakeup_handler);

        time_t now = time(NULL);
        logd("now = %d", now);
        time_t then = now + (SECONDS_PER_HOUR * 6);
        do {
            logd("then = %d", then);
            wakeup_id = wakeup_schedule(then, 0, true);
            if (wakeup_id < 0 && wakeup_id != E_RANGE) {
                loge("could not schedule wakeup");
                break;
            }
            logd("wakeup_schedule() = %d", wakeup_id);
            then += SECONDS_PER_MINUTE;
        } while (wakeup_id == E_RANGE);

        if (wakeup_id > 0) persist_write_int(PERSIST_KEY_WAKEUP_ID, wakeup_id);
    }
}

static void prv_deinit(void) {
    logf();
    if (s_window) window_destroy(s_window);
}

int main(void) {
    logf();
    prv_init();
    app_event_loop();
    prv_deinit();
}
