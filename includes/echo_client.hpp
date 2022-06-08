/*
 *  MMM"""AMV       db      `7MM"""Yp,
 *  M'   AMV       ;MM:       MM    Yb
 *  '   AMV       ,V^MM.      MM    dP
 *     AMV       ,M  `MM      MM"""bg.
 *    AMV   ,    AbmmmqMA     MM    `Y
 *   AMV   ,M   A'     VML    MM    ,9
 *  AMVmmmmMM .AMA.   .AMMA..JMMmmmd9
 *
 *
 * MIT License
 *
 * Copyright (c) 2021 Donald-Rupin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 *  @file echo_client.hpp
 *
 */

#ifndef ZAB_BM_ECHO_CLIENT_HPP_
#define ZAB_BM_ECHO_CLIENT_HPP_

#include <chrono>
#include <cstring>
#include <memory>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "zab/engine.hpp"
#include "zab/engine_enabled.hpp"
#include "zab/for_each.hpp"
#include "zab/simple_future.hpp"
#include "zab/strong_types.hpp"
#include "zab/tcp_networking.hpp"
#include "zab/tcp_stream.hpp"
#include "zab/wait_for.hpp"

namespace zab_bm {

    class echo_client : public zab::engine_enabled<echo_client> {

        public:

            static constexpr std::uint16_t kDefaultThread = 0;

            echo_client(zab::engine* _e, std::size_t _meesage_count, std::size_t _message_size);

            zab::simple_future<bool>
            connect(
                zab::thread_t            _thread,
                struct sockaddr_storage* _details,
                socklen_t                _size) noexcept;

        private:

            const std::size_t meesage_count_;

            const std::size_t message_size_;
    };

    struct bench_struct {
            struct sockaddr_storage* address_;
            std::size_t              meesage_count_;
            std::size_t              message_size_;
    };

    zab::async_function<>
    run_benchmark(
        zab::engine*  _engine,
        std::uint16_t _port,
        const char*   _address,
        std::size_t   _connector_count,
        std::size_t   _meesage_count,
        std::size_t   _message_size)
    {
        std::cout << " ------------------ \n";
        std::cout << " ** Running benchmark ** \n"
                     "Port: "
                  << _port << "\nAddress: " << _address
                  << "\nNumber of connections: " << _connector_count
                  << "\nNumber of messages: " << _meesage_count
                  << "\nMessage size: " << _message_size << "\n";

        std::cout << " ------------------ \n";

        /* Wait until its started */
        co_await yield(_engine, zab::order::now(), zab::thread_t{0});

        std::size_t average_time = 0;
        std::size_t total_time   = 0;

        auto t_start = std::chrono::high_resolution_clock::now();

        struct addrinfo  hints;
        struct addrinfo* addr;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags    = 0;
        hints.ai_protocol = 0;

        auto port_str = std::to_string(_port);
        auto success  = getaddrinfo(_address, port_str.c_str(), &hints, &addr);

        if (success || !addr)
        {
            std::cout << "Failed getaddrinfo "
                      << "\n";
            _engine->stop();
            co_return;
        }

        auto num_threads = _engine->number_of_workers();

        std::vector<zab::simple_future<std::size_t>> futures;

        bench_struct bench_data{
            .address_       = (sockaddr_storage*) addr->ai_addr,
            .meesage_count_ = _meesage_count,
            .message_size_  = _message_size};

        for (auto i = 0ull; i < _connector_count; ++i)
        {
            futures.emplace_back(
                [](zab::engine*  _engine,
                   auto*         _bench_data,
                   zab::thread_t _t) -> zab::simple_future<std::size_t>
                {
                    /* Move to correct thread */
                    auto t_start = std::chrono::high_resolution_clock::now();

                    echo_client client(
                        _engine,
                        _bench_data->meesage_count_,
                        _bench_data->message_size_);

                    auto success = co_await client.connect(
                        _t,
                        _bench_data->address_,
                        sizeof(*(_bench_data->address_)));

                    auto t_end = std::chrono::high_resolution_clock::now();

                    if (success)
                    {
                        co_return std::chrono::duration_cast<std::chrono::milliseconds>(
                            t_end - t_start)
                            .count();
                    }
                    else { co_return std::nullopt; }
                }(_engine, &bench_data, zab::thread_t{(std::uint16_t)(i % num_threads)}));
        }

        auto results = co_await zab::wait_for(_engine, std::move(futures));

        freeaddrinfo(addr);

        auto t_end = std::chrono::high_resolution_clock::now();

        for (const auto time : results)
        {
            if (!time)
            {
                std::cout << "A connection failed."
                          << "\n";
            }
            else { average_time += *time; }
        }

        average_time = average_time / results.size();

        total_time = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

        double mb_sent            = (_meesage_count * _message_size * 2) / (1024.0 * 1024.0);
        double av_time_seconds    = average_time / 1000.0;
        double total_time_seconds = total_time / 1000.0;

        std::cout << " ------------------ \n";
        std::cout << " ** Benchmark Results ** \n";
        std::cout << "Average time per connection: " << average_time << "\n";
        std::cout << "Total time: " << total_time << "\n";
        std::cout << "IO per connection: " << mb_sent << " Mbs\n";
        std::cout << "IO in total: " << (_connector_count * mb_sent) << " Mbs\n";
        std::cout << "Average throughput per connection: " << mb_sent / av_time_seconds
                  << " Mb/s\n";
        std::cout << "Total throughput: " << (_connector_count * mb_sent) / total_time_seconds
                  << " Mb/s\n";
        std::cout << " ------------------ \n";

        _engine->stop();
    }

}   // namespace zab_bm

int
main(int _argc, const char** _argv)
{
    if (_argc != 6)
    {
        std::cout << "Please enter a [port address number_connectors message_count message_size]."
                  << "\n";
        return 1;
    }

    auto port = std::stoi(_argv[1]);

    auto address = _argv[2];

    std::size_t connectors   = std::stoi(_argv[3]);
    std::size_t messages     = std::stoi(_argv[4]);
    std::size_t message_size = ((std::size_t) std::stoi(_argv[5])) * 1024;

    zab::engine e(zab::engine::configs{
        .threads_         = 0,
        .opt_             = zab::engine::configs::kAny,
        .affinity_set_    = false,
        .affinity_offset_ = 0});

    zab_bm::run_benchmark(&e, port, address, connectors, messages, message_size);

    e.start();

    return 0;
}

#endif /* ZAB_BM_ECHO_CLIENT_HPP_ */
