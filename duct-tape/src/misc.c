#include <darlingserver/duct-tape.h>
#include <darlingserver/duct-tape/hooks.h>
#include <darlingserver/duct-tape/log.h>

#include <kern/waitq.h>
#include <kern/clock.h>
#include <kern/turnstile.h>
#include <kern/thread_call.h>
#include <ipc/ipc_init.h>

const dtape_hooks_t* dtape_hooks;

int vsnprintf(char* buffer, size_t buffer_size, const char* format, va_list args);

void dtape_log(dtape_log_level_t level, const char* format, ...) {
	char message[4096];

	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	dtape_hooks->log(level, message);
};

void dtape_init(const dtape_hooks_t* hooks) {
	dtape_hooks = hooks;

	dtape_log_debug("waitq_bootstrap");
	waitq_bootstrap();

	dtape_log_debug("clock_init");
	clock_init();

	dtape_log_debug("turnstiles_init");
	turnstiles_init();

	dtape_log_debug("thread_call_initialize");
	thread_call_initialize();

	dtape_log_debug("ipc_thread_call_init");
	ipc_thread_call_init();

	dtape_log_debug("clock_service_create");
	clock_service_create();
};

void dtape_deinit(void) {

};
