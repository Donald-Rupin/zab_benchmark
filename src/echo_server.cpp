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
 *  @file echo_server.cpp
 *
 */
#include "echo_server.hpp"

#include <chrono>
#include <cstring>
#include <cxxabi.h>     // for __cxa_demangle
#include <execinfo.h>   // for backtrace
#include <malloc.h>
#include <sstream>
#include <string>

#include "zab/event_loop.hpp"
#include "zab/for_each.hpp"
#include "zab/tcp_stream.hpp"

namespace zab_bm {

    echo_server::echo_server(zab::engine* _e, std::uint16_t _port) : acceptor_(_e), port_(_port)
    {
        register_engine(*_e);

        /* Since we will run in an infinite loop use ctr-c to cancel program */
        _e->get_signal_handler().handle(
            SIGINT,
            zab::thread_t{kDefaultThread},
            [this](int) { engine_->stop(); });
    }

    void
    echo_server::initialise() noexcept
    {
        run_acceptor();
    }

    zab::async_function<>
    echo_server::run_acceptor() noexcept
    {
        int connection_count = 0;
        if (acceptor_.listen(AF_INET, port_, 50000))
        {
            co_await zab::for_each(
                acceptor_.get_accepter(),
                [&](auto&& _stream) noexcept -> zab::for_ctl
                {
                    if (_stream)
                    {
                        run_stream(connection_count++, std::move(*_stream));
                        return zab::for_ctl::kContinue;
                    }
                    else
                    {
                        return zab::for_ctl::kBreak;
                    }
                });
        }
        else
        {
            std::cout << "Acceptor errored: " << acceptor_.last_error() << "\n";
            abort();
        }

        engine_->stop();
    }

    zab::async_function<>
    echo_server::run_stream(int _connection_count, zab::tcp_stream _stream) noexcept
    {
        static constexpr auto kBufferSize = 65534;

        /* Lets load balance connections between available threads... */
        zab::thread_t thread{(std::uint16_t)(_connection_count % engine_->number_of_workers())};
        co_await yield(thread);

        std::vector<char>           buffer(kBufferSize);
        zab::tcp_stream::op_control oc{.data_ = (std::byte*) buffer.data(), .size_ = kBufferSize};

        auto reader = _stream.get_reader(&oc);
        auto writer = _stream.get_writer(&oc);

        while (!_stream.last_error())
        {
            auto size = co_await reader;

            if (size && *size)
            {
                oc.size_ = *size;
                co_await writer;
                oc.size_ = kBufferSize;
            }
            else
            {
                if (_stream.last_error())
                {
                    std::cout << "Stream failed " << _stream.last_error() << "\n";
                }

                break;
            }
        }
    }

}   // namespace zab_bm