/*
    Copyright (c) 2020 Intel Corporation

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

#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include "common/test.h"

#include "oneapi/tbb/parallel_scan.h"

#include <vector>

//! \file conformance_parallel_scan.cpp
//! \brief Test for [algorithms.parallel_scan] specification

constexpr std::size_t size = 1000;

template<typename T, typename Op>
class Body {
    const T identity;
    T sum;
    std::vector<T>& y;
    const std::vector<T>& z;
public:
    Body( const std::vector<T>& z_, std::vector<T>& y_, T id ) : identity(id), sum(id), y(y_), z(z_) {}
    T get_sum() const { return sum; }

    template<typename Tag>
    void operator()( const oneapi::tbb::blocked_range<std::size_t>& r, Tag ) {
        T temp = sum;
        for(std::size_t i=r.begin(); i<r.end(); ++i ) {
            temp = Op()(temp, z[i]);
            if( Tag::is_final_scan() )
                y[i] = temp;
        }
        sum = temp;
    }
    Body( Body& b, oneapi::tbb::split ): identity(b.identity), sum(b.identity), y(b.y), z(b.z) {}
    void reverse_join( Body& a ) { sum = Op()(a.sum, sum); }
    void assign( Body& b ) { sum = b.sum; }
};

class default_partitioner_tag{};

template<typename Partitioner>
struct parallel_scan_wrapper{
    template<typename... Args>
    void operator()(Args&&... args) {
    oneapi::tbb::parallel_scan(std::forward<Args>(args)..., Partitioner());
    }
};

template<>
struct parallel_scan_wrapper<default_partitioner_tag>{
    template<typename... Args>
    void operator()(Args&&... args) {
    oneapi::tbb::parallel_scan(std::forward<Args>(args)...);
    }
};

// Test scan tag
//! \brief \ref interface
TEST_CASE("scan tags testing") {
    CHECK(oneapi::tbb::pre_scan_tag::is_final_scan()==false);
    CHECK(oneapi::tbb::final_scan_tag::is_final_scan()==true);
    CHECK((bool)oneapi::tbb::pre_scan_tag()==false);
    CHECK((bool)oneapi::tbb::final_scan_tag()==true);
}

//! Test parallel prefix sum calculation for body-based interface
//! \brief \ref requirement \ref interface
TEST_CASE_TEMPLATE("Test parallel scan with body", Partitioner, default_partitioner_tag, oneapi::tbb::simple_partitioner, oneapi::tbb::auto_partitioner) {
    std::vector<int> input(size);
    std::vector<int> output(size);
    std::vector<int> control(size);

    for(size_t i = 0; i < size; ++i) {
        input[i] = int(i / 2);
        if(i)
            control[i] = control[i-1] + input[i];
        else
            control[i] = input[i];
    }
    Body<int, std::plus<int>> body(input, output, 0);
    parallel_scan_wrapper<Partitioner>()(oneapi::tbb::blocked_range<std::size_t>(0U, size, 1U), body);
    CHECK((control == output));
}


//! Test parallel prefix sum calculation for scan-based interface
//! \brief \ref requirement \ref interface
TEST_CASE_TEMPLATE("Test parallel scan with body", Partitioner, default_partitioner_tag, oneapi::tbb::simple_partitioner, oneapi::tbb::auto_partitioner) {
    std::vector<int> input(size);
    std::vector<int> output(size);
    std::vector<int> control(size);

    for (size_t i = 0; i<size; ++i) {
        input[i] = int(i);
        if (i)
            control[i] = control[i-1]+input[i];
        else
            control[i] = input[i];
    }
    parallel_scan_wrapper<Partitioner>()(oneapi::tbb::blocked_range<std::size_t>(0U, size, 1U), 0U,
        [&](const oneapi::tbb::blocked_range<std::size_t>& r, int sum, bool is_final) -> int
        {
            int temp = sum;
            for (std::size_t i = r.begin(); i<r.end(); ++i) {
                temp = temp + input[i];
                if (is_final)
                    output[i] = temp;
            }
            return temp;
        },
        [](int a, int b) -> int
        {
            return a + b;
        });

    CHECK((control==output));
}
