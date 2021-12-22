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
#include <iostream>
#include <memory>
#include <utility>

#include "asio.hpp"

using asio::ip::tcp;

class session : public std::enable_shared_from_this<session> {
    public:

        session(tcp::socket socket) : socket_(std::move(socket)) { }

        void
        start()
        {
            do_read();
        }

    private:

        void
        do_read()
        {
            std::vector<char> data(65534);
            auto              b = asio::buffer(data.data(), data.size());

            auto self(shared_from_this());
            socket_.async_read_some(
                b,
                [this, data = std::move(data), self](std::error_code ec, auto length) mutable
                {
                    if (!ec)
                    {

                        data.resize(length);
                        do_write(std::move(data));
                        do_read();
                    }
                });
        }

        void
        do_write(std::vector<char>&& _data)
        {
            auto b = asio::buffer(_data.data(), _data.size());

            auto self(shared_from_this());
            asio::async_write(
                socket_,
                b,
                [this, self, _data = std::move(_data)](std::error_code ec, auto) mutable
                {
                    if (ec) { abort(); }
                });
        }

        tcp::socket socket_;
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

    try
    {
        asio::io_context io_context;

        server s(io_context, port);

        io_context.run();

    } catch (std::exception& e)
    {
        std::cerr << "An Exception: " << e.what() << "\n";
    }

    return 0;
}
