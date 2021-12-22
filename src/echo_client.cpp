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
 *  @file echo_client.cpp
 *
 */

#include "echo_client.hpp"

#include "zab/event_loop.hpp"
#include "zab/network_overlay.hpp"
#include "zab/tcp_stream.hpp"

namespace zab_bm {

    echo_client::echo_client(zab::engine* _e, std::size_t _meesage_count, std::size_t _message_size)
        : meesage_count_(_meesage_count), message_size_(_message_size)
    {
        register_engine(*_e);
    }

    zab::simple_future<bool>
    echo_client::connect(
        zab::thread_t            _thread,
        struct sockaddr_storage* _details,
        socklen_t                _size) noexcept
    {
        co_await yield(_thread);

        zab::tcp_connector con(engine_);

        auto stream_opt = co_await con.connect(_details, _size);

        if (stream_opt) { co_return co_await run_stream(std::move(*stream_opt)); }
        else
        {
            std::cout << con.last_error() << " (1)\n";
            co_return false;
        }
    }

    zab::simple_future<bool>
    echo_client::run_stream(zab::tcp_stream _stream) noexcept
    {
        std::vector<char> buffer(message_size_, 42);
        for (auto i = 0ull; i < meesage_count_; ++i)
        {
            /* Write */
            auto [_, amount] =
                co_await zab::wait_for(engine_, _stream.write(buffer), _stream.read(buffer));

            if (_stream.last_error()) [[unlikely]]
            {
                std::cout << _stream.last_error() << " (2)\n";
                co_return false;
            }

            /* Read */
            if (_stream.last_error() || amount != message_size_) [[unlikely]]
            {
                std::cout << _stream.last_error() << " (3)\n";
                co_return false;
            }
        }

        co_return true;
    }

}   // namespace zab_bm