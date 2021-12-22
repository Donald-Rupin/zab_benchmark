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

#include "zab/event_loop.hpp"
#include "zab/for_each.hpp"
#include "zab/tcp_stream.hpp"

namespace zab_bm {

    echo_server::echo_server(zab::engine* _e, std::uint16_t _port) : acceptor_(_e), port_(_port)
    {
        register_engine(*_e);
    }

    zab::async_function<>
    echo_server::run_acceptor() noexcept
    {
        int connection_count = 0;
        if (acceptor_.listen(AF_INET, port_, 10000))
        {
            std::optional<zab::tcp_stream> stream;
            while (stream = co_await acceptor_.accept())
            {
                run_stream(connection_count++, std::move(*stream));
                stream.reset();
            }

            std::cout << "Acceptor errored: " << acceptor_.last_error() << "\n";
        }
        else
        {
            std::cout << "Acceptor errored: " << acceptor_.last_error() << "\n";
        }

        engine_->stop();
    }

    zab::async_function<>
    echo_server::run_stream(int _connection_count, zab::tcp_stream _stream) noexcept
    {
        zab::thread_t thread{
            (std::uint16_t)(_connection_count % engine_->get_event_loop().number_of_workers())};
        /* Lets load balance connections between available threads... */
        co_await yield(thread);

        while (!_stream.last_error())
        {
            auto data = co_await _stream.read_some(65534);

            if (data) { write_stream(std::move(*data), _stream); }
            else
            {
                break;
            }
        }

        _stream.cancel();

        /* Dirty, but easier then tracking writes*/
        co_await yield(zab::order::seconds(1));

        /* Wait for the stream to shutdown */
        co_await _stream.shutdown();
    }

    zab::async_function<>
    echo_server::write_stream(std::vector<char> _data, zab::tcp_stream& _stream) noexcept
    {
        co_await _stream.write(_data);
    }

}   // namespace zab_bm