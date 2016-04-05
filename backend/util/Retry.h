//
// Created by wastl on 05.04.16.
//

#ifndef PRIMARYSOURCES_RETRYEXCEPTION_H
#define PRIMARYSOURCES_RETRYEXCEPTION_H

#include <thread>
#include <chrono>
#include <glog/logging.h>

#define RETRY_INTERNAL(ID, CODE, COUNT, EXCEPTION) \
    bool success_ ## ID = false; \
    int retries_ ## ID = 0; \
    int backoff_ ## ID = 1000; \
    do { \
        try { \
            CODE ; \
            success_ ## ID = true; \
        } catch (const EXCEPTION& e) { \
            retries_ ## ID += 1; \
            if (retries_ ## ID <= COUNT) { \
                LOG(ERROR) << "Execution failed, retry " << retries_ ## ID << " with backoff " << backoff_ ## ID; \
            } else { \
                LOG(ERROR) << "Execution failed, giving up"; \
                throw e; \
            } \
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ ## ID)); \
            backoff_ ## ID = backoff_ ## ID << 1; \
        } \
    } while (!success_ ## ID && retries_ ## ID <= COUNT);

// Retry a code block count times with exponential backoff when exception is raised.
#define RETRY(CODE, COUNT, EXCEPTION) RETRY_INTERNAL(__COUNTER__, CODE, COUNT, EXCEPTION)

#endif //PRIMARYSOURCES_RETRYEXCEPTION_H
