#include "server.h"

#include "session.h"

#include <boost/asio.hpp>

#include <iostream>
#include <memory>

namespace ws = boost::beast::websocket;

Server::Server(size_t threadCount, size_t port)
    : pool_(threadCount)
    , acceptor_(ioc_)
    , playerManager_(std::make_shared<PlayerManager>())
    , gameManager_(std::make_shared<GameManager>())
    , threadCount_(threadCount)
    , port_(port)
{}

void Server::start()
{
    for (int i = 0; i < threadCount_; ++i) {
        boost::asio::post(pool_, [this](){
            auto workGuard = boost::asio::make_work_guard(ioc_);
            ioc_.run();
        });
    }

    ip::tcp::endpoint endpoint(ip::tcp::v4(), port_);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    onAcceptAsync();
    pool_.join();
}

void Server::onAcceptAsync()
{
    auto ws = std::make_shared<ws::stream<boost::beast::tcp_stream>>(ioc_);
    ws->set_option(ws::stream_base::timeout::suggested(boost::beast::role_type::server));

    acceptor_.async_accept(boost::beast::get_lowest_layer(*ws).socket(), [this, ws](boost::system::error_code ec)
        {
            if (ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }

                std::cerr << ec.message() << std::endl;
                return;
            }
            std::make_shared<Session>(ws, playerManager_, gameManager_)->start();
            onAcceptAsync();
        });
}

void Server::stop() {
    ioc_.stop();
}
