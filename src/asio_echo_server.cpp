//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <memory>
#include <thread>
#include <utility>

#include "asio.hpp"

using asio::ip::tcp;

class session : public std::enable_shared_from_this<session> {
    public:

        session(tcp::socket socket) : data_(65534), socket_(std::move(socket)) { }

        void
        start()
        {
            do_read();
        }

    private:

        void
        do_read()
        {
            auto self(shared_from_this());
            socket_.async_read_some(
                asio::buffer(data_.data(), data_.size()),
                [this, self](std::error_code ec, auto length) mutable
                {
                    if (!ec) { do_write(length); }
                });
        }

        void
        do_write(std::size_t _length)
        {
            auto self(shared_from_this());
            asio::async_write(
                socket_,
                asio::buffer(data_.data(), _length),
                [this, self](std::error_code ec, auto) mutable
                {
                    if (!ec) { do_read(); }
                    else
                    {
                        abort();
                    }
                });
        }

        std::vector<char> data_;
        bool              writing_ = false;
        tcp::socket       socket_;
};

class server {
    public:

        server(asio::io_context& io_context, short port)
            : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
        {
            do_accept();
        }

    private:

        void
        do_accept()
        {
            acceptor_.async_accept(
                [this](std::error_code ec, tcp::socket socket)
                {
                    if (!ec) { std::make_shared<session>(std::move(socket))->start(); }

                    do_accept();
                });
        }

        tcp::acceptor acceptor_;
};

int
main(int _argc, const char** _argv)
{
    if (_argc != 2)
    {
        std::cout << "Please enter a port to listen on."
                  << "\n";
        return 1;
    }

    auto port = std::stoi(_argv[1]);

    std::cout << " ** Running echo server on port " << port << " ** \n";
    std::vector<std::jthread> theads;
    try
    {
        asio::io_context io_context;

        server s(io_context, port);

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

    } catch (std::exception& e)
    {
        std::cerr << "An Exception: " << e.what() << "\n";
    }

    return 0;
}
