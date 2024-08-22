/*
    Copyright (c) 2005-2020 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "common/test.h"
#include "common/utils.h"

#include "oneapi/tbb/task_arena.h"
#include "oneapi/tbb/task_scheduler_observer.h"
#include "oneapi/tbb/enumerable_thread_specific.h"
#include "oneapi/tbb/parallel_for.h"

//! \file conformance_task_arena.cpp
//! \brief Test for [scheduler.task_arena scheduler.task_scheduler_observer] specification

//! Test task arena interfaces
//! \brief \ref requirement \ref interface
TEST_CASE("Arena interfaces") {
    //! Initialization interfaces
    oneapi::tbb::task_arena a(1,1); a.initialize();
    //! Enqueue interface
    a.enqueue(utils::DummyBody(10));
    //! Execute interface
    a.execute([&] {
        //! oneapi::tbb::this_task_arena interfaces
        CHECK(oneapi::tbb::this_task_arena::max_concurrency() == 2);
        CHECK(oneapi::tbb::this_task_arena::current_thread_index() >= 0);
        //! Attach interface
        oneapi::tbb::task_arena attached_arena = oneapi::tbb::task_arena(oneapi::tbb::task_arena::attach());
        CHECK(attached_arena.is_active());
    });
    //! Terminate interface
    a.terminate();
}

//! Test tasks isolation for inner oneapi::tbb::parallel_for loop
//! \brief \ref requirement \ref interface
TEST_CASE("Task isolation") {
    const int N1 = 1000, N2 = 1000;
    oneapi::tbb::enumerable_thread_specific<int> ets;
    oneapi::tbb::parallel_for(0, N1, [&](int i) {
        // Set a thread specific value
        ets.local() = i;
        // Run the second parallel loop in an isolated region to prevent the current thread
        // from taking tasks related to the outer parallel loop.
        oneapi::tbb::this_task_arena::isolate([&]{
            oneapi::tbb::parallel_for(0, N2, utils::DummyBody(10));
        });
        REQUIRE(ets.local() == i);
    });
}

class conformance_observer: public oneapi::tbb::task_scheduler_observer {
public:
    std::atomic<bool> is_entry_called{false};
    std::atomic<bool> is_exit_called{false};

    conformance_observer( oneapi::tbb::task_arena &a ) : oneapi::tbb::task_scheduler_observer(a) {
        observe(true); // activate the observer
    }

    void on_scheduler_entry(bool) override {
        is_entry_called.store(true, std::memory_order_relaxed);
    }

    void on_scheduler_exit(bool) override {
        is_exit_called.store(true, std::memory_order_relaxed);
    }

    bool is_callbacks_called() {
        return is_entry_called.load(std::memory_order_relaxed)
            && is_exit_called.load(std::memory_order_relaxed);
    }
};

//! Test task arena observer interfaces
//! \brief \ref requirement \ref interface
TEST_CASE("Task arena observer") {
    oneapi::tbb::task_arena a; a.initialize();
    conformance_observer observer(a);
    a.execute([&] {
        oneapi::tbb::parallel_for(0, 100, utils::DummyBody(10), oneapi::tbb::simple_partitioner());
    });
    REQUIRE(observer.is_callbacks_called());
}

//! Test task arena copy constructor
//! \brief \ref interface \ref requirement
TEST_CASE("Task arena copy constructor") {
    oneapi::tbb::task_arena arena(1);
    oneapi::tbb::task_arena copy = arena;

    REQUIRE(arena.max_concurrency() == copy.max_concurrency());
    REQUIRE(arena.is_active() == copy.is_active());
}
