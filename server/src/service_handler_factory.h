#pragma once

#include "service_handler.h"

typedef struct service_handler_factory {
	struct service_parameters service_parameters;
} service_handler_factory_t;

// Initialize a service handler factory
static inline void service_handler_factory_init(struct service_handler_factory *factory, struct service_parameters service_parameters) {
	factory->service_parameters = service_parameters;
}

// Create a new service handler
static inline void service_handler_factory_create(struct service_handler_factory *factory, dispatcher_t *dispatcher, struct connection_details connection_details) {
	service_handler_t *service_handler = service_handler_init(dispatcher, factory->service_parameters, connection_details);
}
