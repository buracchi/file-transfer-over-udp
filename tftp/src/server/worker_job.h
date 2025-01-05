#ifndef WORKER_JOB_H
#define WORKER_JOB_H

#include "session.h"

struct worker_job {
    uint16_t job_id;
    volatile atomic_bool is_active;
    struct dispatcher *dispatcher;
    struct tftp_session session;
};

enum job_state {
    JOB_STATE_RUNNING,
    JOB_STATE_TERMINATED,
    JOB_STATE_ERROR,
};

enum job_state job_handle_event(struct worker_job job[static 1], struct dispatcher_event event[static 1]);

#endif // WORKER_JOB_H
