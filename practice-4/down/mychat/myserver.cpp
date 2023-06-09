#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "mymessage.hpp"

using boost::asio::ip::tcp;

class chat_participant
{
    public:
        virtual ~chat_participant() {}
        virtual void deliver(const mymessage& msg) = 0;
};

class chat_room
{
    private:
    std::set<std::shared_ptr<chat_participant>> participants_;
    int max_recent_msgs = 100;
    std::deque<mymessage> recent_msgs_;
    public:
    void join(std::shared_ptr<chat_participant> participant)
    {
        participants_.insert(participant);
        for (auto msg: recent_msgs_)
            participant->deliver(msg);
    }

    void leave(std::shared_ptr<chat_participant> participant)
    {
        participants_.erase(participant);
    }

    void deliver(const mymessage& msg)
    {
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > max_recent_msgs)
        {
            recent_msgs_.pop_front();
        }

        for (auto participant: participants_)
        {
            participant->deliver(msg);
        }
    }
};

class chat_session
    : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
    private:
        tcp::socket socket_;
        chat_room& room_;
        mymessage read_msg_;
        std::deque<mymessage> write_msgs_;
    public: 
        chat_session(tcp::socket socket, chat_room& room)
            : socket_(std::move(socket)),
            room_(room)
        {}

        void start()
        {
            room_.join(shared_from_this());
            do_read_header();
        }

        void deliver(const mymessage& msg)
        {
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg);
            if (!write_in_progress)
            {
                do_write();
            }
        }

    private:
        void do_read_header()
        {
            auto self(shared_from_this());
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), mymessage::header_length), 
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
                {   
                    if (!ec && read_msg_.decode_header()){
                        // todo different types of messages
                        printf("debug: read header, the type is %d\n", read_msg_.type());
                        printf("from %s, to %s\n", read_msg_.sender_name(), read_msg_.object_name());
                        do_read_body();
                        printf("the message is %s", read_msg_.body());
                    }
                    else room_.leave(shared_from_this());
                });
        }

        void do_read_body()
        {
            auto self(shared_from_this());
            boost::asio::async_read(socket_,
                    boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                    [this, self](boost::system::error_code ec, std::size_t /*length*/)
                    {   
                        if (!ec){
                            room_.deliver(read_msg_);
                            do_read_header();
                        }
                        else{
                            room_.leave(shared_from_this());
                        }
                    });
        }

        void do_write()
        {
            auto self(shared_from_this());
            boost::asio::async_write(socket_,
                    boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
                    [this, self](boost::system::error_code ec, std::size_t /*length*/)
                    {
                        if (!ec)
                        {
                            write_msgs_.pop_front();
                            if (!write_msgs_.empty()) do_write();
                        }
                        else room_.leave(shared_from_this());
                    });
        }
};


class chat_server
{
    private:
        tcp::acceptor acceptor_;
        chat_room room_;
    public:
        chat_server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint)
            : acceptor_(io_context, endpoint)
        {
            do_accept();
        }

    private:
        void do_accept()
        {
            acceptor_.async_accept(
                [this](boost::system::error_code ec, tcp::socket socket)
                {
                    if (!ec) std::make_shared<chat_session>(std::move(socket), room_)->start();
                    do_accept();
                });
        }
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc < 2)
        {
            std::cerr << "Usage: chat_server <port> [<port> ...]\n";
            return 1;
        }

        boost::asio::io_context io_context;

        std::list<chat_server> servers;
        for (int i = 1; i < argc; ++i)
        {
            tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
            servers.emplace_back(io_context, endpoint);
        }

        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}