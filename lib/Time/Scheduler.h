#pragma once

#include <etl/priority_queue.h>
#include "Units.h"

typedef void (*TaskFunc)(void *);

class Task {
public:
    Task(Duration max_duration, Duration interval, TaskFunc func);
    void setData(void *data);

    friend class Scheduler;
    
    bool operator<(const Task& other) const {
        // the ordering is reversed, because ETL's priority queue always puts
        // the greatest element at the top
        return m_next_run > other.m_next_run;
    }

    bool operator>(const Task& other) const {
        // the ordering is reversed, because ETL's priority queue always puts
        // the greatest element at the top
        return m_next_run < other.m_next_run;
    }

private:
    uint32_t m_max_duration_ms;
    uint32_t m_interval_ms;
    uint64_t m_next_run = 0;
    void *m_data = nullptr;
    TaskFunc m_func;

    void run();
    bool isReady();
};

class Scheduler {
public:
    void runNextTask();
    void addTask(Task task);

private:
    etl::priority_queue<Task, 10> m_tasks;
};