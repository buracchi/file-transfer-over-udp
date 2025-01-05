#include "worker_job.h"

enum job_state job_handle_event(struct worker_job job[static 1], struct dispatcher_event event[static 1]) {
    switch (tftp_session_handle_event(&job->session, event)) {
        case TFTP_SESSION_STATE_IDLE:
            return JOB_STATE_RUNNING;
        case TFTP_SESSION_STATE_CLOSED:
            return JOB_STATE_TERMINATED;
        case TFTP_SESSION_STATE_ERROR:
            return JOB_STATE_ERROR;
    }
    unreachable();
}
