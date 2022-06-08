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

        auto stream =
            co_await zab::tcp_connect<std::byte>(engine_, (const sockaddr*) _details, _size);

        if (auto le = stream.last_error(); !le)
        {
            std::vector<std::byte> buffer(message_size_, std::byte{42});
            for (auto i = 0ull; i < meesage_count_; ++i)
            {
                /* Write */
                auto [_, amount] =
                    co_await zab::wait_for(stream.write(buffer), stream.read(buffer));

                if (le = stream.last_error(); le || amount != (long long int) message_size_)
                    [[unlikely]]
                {
                    std::cout << le << " (2)\n";
                    std::cout << amount << " vs " << message_size_ << " (2)\n";
                    co_return false;
                }
            }

            co_return true;
        }
        else
        {
            std::cout << le << " (1)\n";
            co_return false;
        }
    }

}   // namespace zab_bm
