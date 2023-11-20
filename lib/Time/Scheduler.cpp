#include "Scheduler.h"
#include <libopencm3/cm3/assert.h>
#include <etl/algorithm.h>
#include "Time.h"

Task::Task(Duration max_duration, Duration interval, TaskFunc func)
: m_func{func} {
    m_max_duration_ms = max_duration.asMilliseconds();
    cm3_assert(m_max_duration_ms > 0);
    m_interval_ms = interval.asMilliseconds();
    cm3_assert(m_interval_ms > 0);
}

void Task::setData(void *data) {
    m_data = data;
}

void Task::run() {
    m_next_run = time::getMsTicks() + m_interval_ms;
    m_func(m_data);
}

bool Task::isReady() {
    return time::getMsTicks() >= m_next_run;
}

void Scheduler::runNextTask() {
    if (m_tasks.empty()) {
        return;
    }

    // FIXME: Maybe check for timeout?
    while (!m_tasks.top().isReady())
        ;

    Task t = m_tasks.top();
    m_tasks.pop();
    t.run();
    m_tasks.push(t);
}

void Scheduler::addTask(Task task) {
    m_tasks.push(task);
}
