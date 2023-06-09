#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <random>
#include <ctime>
#include "mymessage.hpp"

using boost::asio::ip::tcp;
std::string sender = "tourist";
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */
class chat_client
{
    private:
        boost::asio::io_context& io_context_;
        tcp::socket socket_;
        mymessage read_msg_;
        std::deque<mymessage> write_msgs_;
    public:
        chat_client(boost::asio::io_context& io_context,
            const tcp::resolver::results_type& endpoints)
            : io_context_(io_context),
            socket_(io_context)
        {
            do_connect(endpoints);
        }

        void write(const mymessage& msg)
        {
            boost::asio::post(io_context_,
                [this, msg]()
                {
                    bool write_in_progress = !write_msgs_.empty();
                    write_msgs_.push_back(msg);
                    if (!write_in_progress) do_write();
                });
        }

        void close()
        {
            boost::asio::post(io_context_, [this]() { socket_.close(); });
        }
    private:
        void do_connect(const tcp::resolver::results_type& endpoints)
        {
            boost::asio::async_connect(socket_, endpoints,
                [this](boost::system::error_code ec, tcp::endpoint)
                {
                    if (!ec) do_read_header();
                });
        }

        void do_read_header()
        {
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), mymessage::header_length),
                [this](boost::system::error_code ec, std::size_t /*length*/)
                {
                    if (!ec && read_msg_.decode_header()) do_read_body();
                    else socket_.close();
                });
        }

        void do_read_body()
        {
            boost::asio::async_read(socket_,
                    boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                    [this](boost::system::error_code ec, std::size_t /*length*/)
                    {
                        if (!ec){
                            std::cout<<BOLDMAGENTA;
                            std::cout.write(read_msg_.body(), read_msg_.body_length());
                            std::cout<<RESET;
                            std::cout << "\n";
                            // printf("type: %d\n",read_msg_.type());
                            // printf("object: %s\n",read_msg_.object_name());
                            // printf("body: %s\n",read_msg_.body());
                            // printf("the result of strcmp is %d\n",strcmp(read_msg_.body(),"Login successfully"));
                            if(read_msg_.type() == 2 && strcmp(read_msg_.body(),"Login successfully")==0) {
                                sender = std::string(read_msg_.object_name());
                                printf("Successfully login as name %s\n",sender.c_str());
                                // printf("login as name %s\n",sender);
                            }
                            do_read_header();
                        }
                        else socket_.close();
                    });
        }

        void do_write()
        {
            boost::asio::async_write(socket_,
                    boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
                    [this](boost::system::error_code ec, std::size_t /*length*/)
                    {
                        if (!ec)
                        {
                            write_msgs_.pop_front();
                            if (!write_msgs_.empty()) do_write();
                        }
                        else socket_.close();
                    });
        }
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: chat_client <host> <port>\n";
            return 1;
        }
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(argv[1], argv[2]);
        chat_client c(io_context, endpoints);
        std::thread t([&io_context](){ io_context.run(); });
        char line[mymessage::max_body_length + 1];
        while (std::cin.getline(line, mymessage::max_body_length + 1))
        {   
            if(sender == "tourist"){
                int id = rand()%10000;
                char tmp[10];
                sprintf(tmp,"%d",id);
                sender = std::string("tourist") + std::string(tmp);
                std::cout << "Initial name is " << sender << std::endl;
            }
            mymessage msg(line,sender.c_str());
            bool error = msg.encode();
            if(error) printf("Invalid command, \'help\' for tips of commands.\n");
            else c.write(msg);
        }
        c.close();
        t.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}