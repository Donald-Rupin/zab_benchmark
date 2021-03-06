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
 *  @file echo_server.hpp
 *
 */

#ifndef ZAB_BM_ECHO_SERVER_HPP_
#define ZAB_BM_ECHO_SERVER_HPP_

#include <chrono>

#include "zab/async_function.hpp"
#include "zab/engine.hpp"
#include "zab/engine_enabled.hpp"
#include "zab/strong_types.hpp"
#include "zab/tcp_networking.hpp"
#include "zab/tcp_stream.hpp"

namespace zab_bm {

    class echo_server : public zab::engine_enabled<echo_server> {

        public:

            static constexpr std::uint16_t kDefaultThread = 0;

            echo_server(zab::engine* _e, std::uint16_t _port);

            void
            initialise() noexcept;

        private:

            zab::async_function<>
            run_acceptor() noexcept;

            zab::async_function<>
            run_stream(int _connection_count, zab::tcp_stream<std::byte> _stream) noexcept;

            zab::tcp_acceptor acceptor_;

            const std::uint16_t port_;
    };

}   // namespace zab_bm

int
main(int _argc, const char** _argv)
{
    if (_argc != 2)
    {
        std::cout << "Please enter a port to listen on."
                  << "\n";
        return 1;
    }

    {
        auto port = std::stoi(_argv[1]);

        std::cout << " ** Running echo server on port " << port << " ** \n";

        zab::engine e(zab::engine::configs{
            .threads_         = 0,
            .opt_             = zab::engine::configs::kAny,
            .affinity_set_    = false,
            .affinity_offset_ = 0});

        zab_bm::echo_server es(&e, port);

        e.start();
    }

    return 0;
}

#endif /* ZAB_BM_ECHO_SERVER_HPP_ */
