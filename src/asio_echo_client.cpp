//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include "asio.hpp"

using asio::ip::tcp;

enum {
    max_length = 1024
};

struct bench_struct {
        std::uint64_t connection_count_;
        std::uint64_t meesage_count_;
        std::uint64_t message_size_;
};

class session : public std::enable_shared_from_this<session> {
    public:

        session(
            asio::io_context& _io,
            bench_struct      _bs,
            std::uint32_t&    _time,
            std::uint32_t&    _count,
            tcp::socket       socket)
            : io_(_io), bs_(_bs), time_(_time), count_(_count), socket_(std::move(socket)),
              buffer_(bs_.message_size_, 42)
        { }

        void
        start()
        {
            start_ = std::chrono::high_resolution_clock::now();
            do_write(0);
            do_read(0, 0);
        }

    private:

        void
        do_read(std::uint32_t _up_to, std::uint32_t _iterations)
        {
            auto self(shared_from_this());
            socket_.async_read_some(
                asio::buffer(buffer_.data() + _up_to, buffer_.size() - _up_to),
                [this, self, _up_to, _iterations](std::error_code ec, std::uint32_t length)
                {
                    if (!ec)
                    {
                        if (_up_to + length != buffer_.size())
                        {
                            do_read(_up_to + length, _iterations);
                        }
                        else
                        {
                            if (_iterations + 1 < bs_.meesage_count_)
                            {
                                do_write(0);
                                do_read(0, _iterations + 1);
                            }
                            else
                            {
                                /* finished */
                                auto t_end = std::chrono::high_resolution_clock::now();

                                time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                                            t_end - start_)
                                            .count();

                                if (++count_ == bs_.connection_count_) { io_.stop(); }
                            }
                        }
                    }
                });
        }

        void
        do_write(std::uint32_t _up_to)
        {
            auto self(shared_from_this());
            asio::async_write(
                socket_,
                asio::buffer(buffer_.data() + _up_to, buffer_.size() - _up_to),
                [this, self, _up_to](std::error_code ec, std::uint32_t length)
                {
                    if (!ec)
                    {
                        if (_up_to + length != buffer_.size()) { do_write(_up_to + length); }
                    }
                });
        }

        asio::io_context&                                   io_;
        bench_struct                                        bs_;
        std::uint32_t&                                      time_;
        std::uint32_t&                                      count_;
        tcp::socket                                         socket_;
        std::vector<char>                                   buffer_;
        decltype(std::chrono::high_resolution_clock::now()) start_;
};

class clients {
    public:

        clients(
            bench_struct       _bs,
            asio::io_context&  io_context,
            const std::string& _address,
            short              _port)
            : bs_(_bs), io_(io_context), r(io_context), q(_address, std::to_string(_port)),
              total_complete_(0)
        {
            start_ = std::chrono::high_resolution_clock::now();

            for (auto i = 0u; i < bs_.connection_count_; ++i)
            {
                times_.emplace_back(0, tcp::socket(io_));
            }

            do_connect();
        }

        void
        print_results()
        {
            auto t_end = std::chrono::high_resolution_clock::now();

            std::uint32_t average_time = 0;
            for (const auto& [t, s] : times_)
            {
                average_time += t;
            }

            average_time = average_time / times_.size();

            auto total_time =
                std::chrono::duration_cast<std::chrono::milliseconds>(t_end - start_).count();

            double mb_sent = (bs_.meesage_count_ * bs_.message_size_ * 2) / (1024.0 * 1024.0);
            double av_time_seconds    = average_time / 1000.0;
            double total_time_seconds = total_time / 1000.0;

            std::cout << " ------------------ \n";
            std::cout << " ** Benchmark Results ** \n";
            std::cout << "Average time per connection: " << average_time << "\n";
            std::cout << "Total time: " << total_time << "\n";
            std::cout << "IO per connection: " << mb_sent << " Mbs\n";
            std::cout << "IO in total: " << (bs_.connection_count_ * mb_sent) << " Mbs\n";
            std::cout << "Average throughput per connection: " << mb_sent / av_time_seconds
                      << " Mb/s\n";
            std::cout << "Total throughput: "
                      << (bs_.connection_count_ * mb_sent) / total_time_seconds << " Mb/s\n";
            std::cout << " ------------------ \n";
        }

    private:

        void
        do_connect()
        {
            r.async_resolve(
                q,
                [this](const auto& ec, tcp::resolver::results_type results)
                {
                    if (!ec)
                    {
                        for (auto& [t, s] : times_)
                        {
                            asio::async_connect(
                                s,
                                results,
                                [this, &s, &t](const auto& ec, const tcp::endpoint&) mutable
                                {
                                    if (!ec)
                                    {
                                        auto ses = std::make_shared<session>(
                                            io_,
                                            bs_,
                                            t,
                                            total_complete_,
                                            std::move(s));

                                        ses->start();
                                    }
                                    else
                                    {
                                        std::cout << "connect failed: " << ec << "\n";
                                        io_.stop();
                                    }
                                });
                        }
                    }
                    else
                    {
                        std::cout << "resolve failed: " << ec << "\n";
                        io_.stop();
                    }
                });
        }

        bench_struct         bs_;
        asio::io_context&    io_;
        tcp::resolver        r;
        tcp::resolver::query q;

        decltype(std::chrono::high_resolution_clock::now()) start_;
        std::vector<std::pair<std::uint32_t, tcp::socket>>  times_;
        std::uint32_t                                       total_complete_;
};

int
main(int _argc, char* _argv[])
{
    try
    {
        if (_argc != 6)
        {
            std::cout
                << "Please enter a [port address number_connectors message_count message_size]."
                << "\n";
            return 1;
        }

        auto port = std::stoi(_argv[1]);

        auto address = _argv[2];

        bench_struct bs{
            .connection_count_ = (std::uint32_t) std::stoi(_argv[3]),
            .meesage_count_    = (std::uint32_t) std::stoi(_argv[4]),
            .message_size_     = (std::uint32_t) std::stoi(_argv[5]) * 1024};

        std::cout << " ------------------ \n";
        std::cout << " ** Running benchmark ** \n"
                     "Port: "
                  << port << "\nAddress: " << address
                  << "\nNumber of connections: " << bs.connection_count_
                  << "\nNumber of messages: " << bs.meesage_count_
                  << "\nMessage size: " << bs.message_size_ << "\n";

        std::cout << " ------------------ \n";

        asio::io_context io_context;

        clients c(bs, io_context, address, port);

        std::vector<std::jthread> theads;

        auto hc = std::thread::hardware_concurrency();
        if (!hc) { hc = 1; }

        for (auto i = 0u; i < hc; i++)
        {
            theads.emplace_back([&io_context]() { io_context.run(); });
        }

        for (auto& t : theads)
        {
            if (t.joinable()) { t.join(); }
        }

        c.print_results();

    } catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
