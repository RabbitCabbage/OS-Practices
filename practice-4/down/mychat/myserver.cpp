#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <map>
#include <vector>
#include <utility>
#include <boost/asio.hpp>
#include "mymessage.hpp"

using boost::asio::ip::tcp;

struct account{
    std::string name;
    std::string password;
    bool valid;// 是否是tourist
    bool online;
    std::map<std::string, std::vector<std::string>> friends_to_history;
};

class chat_participant
{
    public:
        int id;
        virtual ~chat_participant() {}
        virtual void deliver(const mymessage& msg) = 0;
};

class chat_room
{
    private:
    std::set<std::shared_ptr<chat_participant>> participants_;
    int client_id = 0;// id of the client
    public:
    std::map<int, std::string> id_to_name;// map client id to name
    std::map<std::string, account> name_to_account;// map name to account
    std::string tourist = "tourist";
    private:
    int max_recent_msgs = 100;
    std::deque<mymessage> recent_msgs_;
    public:
    void join(std::shared_ptr<chat_participant> participant)
    {
        participant->id = ++client_id;//give it an id
        id_to_name[client_id] = tourist;// associate this id to a tourist account
        participants_.insert(participant); // add this client
        for (auto msg: recent_msgs_){
            participant->deliver(msg);
        }
    }

    void leave(std::shared_ptr<chat_participant> participant)
    {
        name_to_account[id_to_name[participant->id]].online = false;
        id_to_name.erase(participant->id);
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
        printf("Delivered message\n");
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
            printf("sender name is %s\n", msg.sender_name());
            printf("object name is %s\n", msg.object_name());
            printf("participant name is %s\n", room_.id_to_name[id].c_str());
            if(msg.type() == ADD || msg.type() == DEL || msg.type() == HISTORY || msg.type() == CLEAR || msg.type() == ADDR){
                 if(room_.id_to_name[id] != msg.sender_name())return;
            }else if(room_.id_to_name[id] != msg.object_name() && msg.object_name() != "all")return;
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
            printf("Read Header: the name of this participant is %s\n", room_.id_to_name[id].c_str());
            auto self(shared_from_this());
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), mymessage::header_length), 
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
                {   
                    // printf("Read header=>");
                    if (!ec && read_msg_.decode_header()){
                        // different types of messages
                        // printf("debug: read header, the type is %d, ", read_msg_.type());
                        // printf("from %s, to %s, ", read_msg_.sender_name(), read_msg_.object_name());
                        do_read_body();
                        // printf("the message is %s\n", read_msg_.body());
                    }
                    else room_.leave(shared_from_this());
                });
        }

        void operations(){
            printf("the type is %d\n", read_msg_.type());
            if(read_msg_.type() == LOGIN){
                bool login = false;
                bool already_online = false;
                if(room_.name_to_account.find(read_msg_.object_name())!=room_.name_to_account.end()) {
                    // login the existing account
                    if(room_.name_to_account[read_msg_.object_name()].online == true) {
                        printf("This account is already online\n");
                        already_online = true;
                    }
                    else{
                        if(strcmp(room_.name_to_account[read_msg_.object_name()].password.c_str(), read_msg_.body())!=0) printf("Wrong password\n");
                        else{
                            // logout the previous account
                            if(room_.name_to_account[room_.id_to_name[id]].valid){
                                room_.name_to_account[room_.id_to_name[id]].online = false;
                                printf("make the account named %s offline\n", room_.id_to_name[id].c_str());
                            }
                            room_.name_to_account[read_msg_.object_name()].online = true;
                            room_.name_to_account[read_msg_.object_name()].valid = true;
                            printf("make the account named %s online\n", read_msg_.object_name());
                            room_.id_to_name[id] = read_msg_.object_name();// name of participant changed
                            login = true;
                        } 
                    }
                } else {
                    // create a new account
                    if(room_.name_to_account[room_.id_to_name[id]].valid){
                        room_.name_to_account[room_.id_to_name[id]].online = false;
                        printf("make the account named %s offline\n", room_.id_to_name[id].c_str());
                    }
                    account new_account;
                    new_account.name = read_msg_.object_name();
                    new_account.password = read_msg_.body();
                    new_account.valid = true;
                    new_account.online = true;
                    room_.name_to_account[read_msg_.object_name()] = new_account;
                    room_.id_to_name[id] = read_msg_.object_name();
                    login = true;
                }
                if(login){
                    const char *msg = "Login successfully\0";
                    memcpy(read_msg_.body(), msg, strlen(msg));
                    *(read_msg_.body() + strlen(msg)) = '\0';
                    read_msg_.body_length_ = strlen(msg);
                    std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));                 
                }else if(already_online) {
                    printf("Login failed\n");
                    const char *msg = "Login failed, user already online\0";
                    memcpy(read_msg_.body(), msg, strlen(msg));
                    *(read_msg_.body() + strlen(msg)) = '\0';
                    read_msg_.body_length_ = strlen(msg);
                    std::sprintf(read_msg_.get_length_ptr(), "%4d", static_cast<int>(read_msg_.body_length_));
                    memcpy(read_msg_.get_object_ptr(), read_msg_.sender_name(), strlen(read_msg_.sender_name()));
                    *(read_msg_.get_object_ptr() + strlen(read_msg_.sender_name())) = '\0';
                    std::sprintf(read_msg_.get_object_len_ptr(), "%4d", static_cast<int>(strlen(read_msg_.sender_name())));
                    memcpy(read_msg_.object, read_msg_.sender_name(), strlen(read_msg_.sender_name()));
                } else {
                    printf("Login failed\n");
                    const char *msg = "Login failed, wrong password\0";
                    memcpy(read_msg_.body(), msg, strlen(msg));
                    *(read_msg_.body() + strlen(msg)) = '\0';
                    read_msg_.body_length_ = strlen(msg);
                    std::sprintf(read_msg_.get_length_ptr(), "%4d", static_cast<int>(read_msg_.body_length_));
                    memcpy(read_msg_.get_object_ptr(), read_msg_.sender_name(), strlen(read_msg_.sender_name()));
                    *(read_msg_.get_object_ptr() + strlen(read_msg_.sender_name())) = '\0';
                    std::sprintf(read_msg_.get_object_len_ptr(), "%4d", static_cast<int>(strlen(read_msg_.sender_name())));
                    memcpy(read_msg_.object, read_msg_.sender_name(), strlen(read_msg_.sender_name()));
                }
                room_.deliver(read_msg_);
            } else if(room_.name_to_account[room_.id_to_name[id]].valid == false){
                std::string msg = "You are a tourist, please login first\0";
                memcpy(read_msg_.body(), msg.c_str(), msg.length());    
                *(read_msg_.body() + msg.length()) = '\0';
                read_msg_.body_length_ = msg.length();
                std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                printf("server operation: send from %s to %s\n", read_msg_.sender_name(), read_msg_.object_name());
                room_.deliver(read_msg_);
            } else{
                if(read_msg_.type() == LIST){
                    std::string list = std::string("=========================\nThe list of online users:\n");
                    for(auto it = room_.name_to_account.begin(); it!=room_.name_to_account.end(); it++){
                        if(it->second.online) list += it->first + "\n";
                    }
                    list += "=========================\n";
                    // printf("the sender is %s\n", read_msg_.sender_name());
                    // printf("the object is %s\n", read_msg_.object_name());
                    // printf("the body is %s\n", read_msg_.body());
                    memcpy(read_msg_.body(), list.c_str(), list.length());
                    *(read_msg_.body() + list.length()) = '\0';
                    read_msg_.body_length_ = list.length();
                    std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    // printf("debug: the body length is %ld\n", read_msg_.body_length_);
                    // printf("debug: the body is %s\n", read_msg_.body());
                    room_.deliver(read_msg_);
                } else if(read_msg_.type() == ADD){
                    std::string add_name = read_msg_.object_name();
                    if(room_.name_to_account.find(add_name)==room_.name_to_account.end()){
                        const char *msg = "The user does not exist\0";
                        memcpy(read_msg_.body(), msg, strlen(msg));
                        *(read_msg_.body() + strlen(msg)) = '\0';
                        read_msg_.body_length_ = strlen(msg);
                        std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    } else if(room_.name_to_account[room_.id_to_name[id]].friends_to_history.find(add_name)!=room_.name_to_account[room_.id_to_name[id]].friends_to_history.end()){
                        const char *msg = "Friend has been added\0";
                        memcpy(read_msg_.body(), msg, strlen(msg));
                        *(read_msg_.body() + strlen(msg)) = '\0';
                        read_msg_.body_length_ = strlen(msg);
                        std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    } else {
                        room_.name_to_account[room_.id_to_name[id]].friends_to_history[add_name] = std::vector<std::string>();
                        room_.name_to_account[add_name].friends_to_history[room_.id_to_name[id]] = std::vector<std::string>();
                        const char *msg = "Add successfully\0";
                        memcpy(read_msg_.body(), msg, strlen(msg));
                        *(read_msg_.body() + strlen(msg)) = '\0';
                        read_msg_.body_length_ = strlen(msg);
                        std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    }
                    room_.deliver(read_msg_);
                } else if(read_msg_.type() == DEL){
                    std::string del_name = read_msg_.object_name();
                    if(room_.name_to_account.find(del_name)==room_.name_to_account.end()||room_.name_to_account[room_.id_to_name[id]].friends_to_history.find(del_name)==room_.name_to_account[room_.id_to_name[id]].friends_to_history.end()){
                        const char *msg = "The friend does not exist\0";
                        memcpy(read_msg_.body(), msg, strlen(msg));
                        *(read_msg_.body() + strlen(msg)) = '\0';
                        read_msg_.body_length_ = strlen(msg);
                        std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    } else {
                        room_.name_to_account[room_.id_to_name[id]].friends_to_history.erase(del_name);
                        room_.name_to_account[del_name].friends_to_history.erase(room_.id_to_name[id]);
                        const char *msg = "Delete successfully\0";
                        memcpy(read_msg_.body(), msg, strlen(msg));
                        *(read_msg_.body() + strlen(msg)) = '\0';
                        read_msg_.body_length_ = strlen(msg);
                        std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    }
                    room_.deliver(read_msg_);                        
                } else if(read_msg_.type() == SEND){
                    std::string to_name = read_msg_.object_name();
                    if(to_name!="all" && (room_.name_to_account.find(to_name)==room_.name_to_account.end()||room_.name_to_account[room_.id_to_name[id]].friends_to_history.find(to_name)==room_.name_to_account[room_.id_to_name[id]].friends_to_history.end())){
                        std::string msg = "Failed to send message, ";
                        if(room_.name_to_account.find(to_name)==room_.name_to_account.end()) msg += "the friend does not exist\0";
                        else msg += "you are not friends\0";
                        memcpy(read_msg_.body(), msg.c_str(), msg.length());
                        *(read_msg_.body() + msg.length()) = '\0';
                        read_msg_.body_length_ = msg.length();
                        std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    } else {
                        std::string msg = read_msg_.sender_name() + std::string(": ") + read_msg_.body();
                        memcpy(read_msg_.body(), msg.c_str(), msg.length());
                        *(read_msg_.body() + msg.length()) = '\0';
                        read_msg_.body_length_ = msg.length();
                        std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                        room_.name_to_account[read_msg_.sender_name()].friends_to_history[read_msg_.object_name()].push_back(std::string(read_msg_.body()));
                        room_.name_to_account[read_msg_.object_name()].friends_to_history[read_msg_.sender_name()].push_back(std::string(read_msg_.body()));
                    }
                    room_.deliver(read_msg_);
                }else if(read_msg_.type() == HISTORY){
                        std::string history_name = read_msg_.object_name();
                        if(room_.name_to_account[room_.id_to_name[id]].friends_to_history.find(history_name)==room_.name_to_account[room_.id_to_name[id]].friends_to_history.end()){
                            std::string msg = "You have no friend named "+history_name+"\0";
                            memcpy(read_msg_.body(), msg.c_str(), msg.length());
                            *(read_msg_.body() + msg.length()) = '\0';
                            read_msg_.body_length_ = msg.length();
                            std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                        } else {
                            std::string history = std::string("=========================\nThe history of " + history_name + ":\n");
                            for(auto it = room_.name_to_account[room_.id_to_name[id]].friends_to_history[history_name].begin(); it!=room_.name_to_account[room_.id_to_name[id]].friends_to_history[history_name].end(); it++){
                                history += *it + std::string("\n");
                            }
                            history += "=========================\n";
                            memcpy(read_msg_.body(), history.c_str(), history.length());
                            *(read_msg_.body() + history.length()) = '\0';
                            read_msg_.body_length_ = history.length();
                            std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                        }
                        room_.deliver(read_msg_);
                }else if(read_msg_.type() == HELP){
                    std::string help("The commands are:\nlist\nadd <name>\ndel <name>\nsend <name> <message>\naddr\nhistory <name>\nclear <name>\nlogin <name> <password>\nhelp");
                    memcpy(read_msg_.body(), help.c_str(), help.length());
                    *(read_msg_.body() + help.length()) = '\0';
                    read_msg_.body_length_ = help.length();
                    std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    room_.deliver(read_msg_);
                }else if(read_msg_.type() == CLEAR){
                    std::string clear_name = read_msg_.object_name();
                    if(room_.name_to_account[room_.id_to_name[id]].friends_to_history.find(clear_name)==room_.name_to_account[room_.id_to_name[id]].friends_to_history.end()){
                        std::string msg = "You have no friend named "+clear_name+"\0";
                        memcpy(read_msg_.body(), msg.c_str(), msg.length());
                        *(read_msg_.body() + msg.length()) = '\0';
                        read_msg_.body_length_ = msg.length();
                        std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    } else {
                        room_.name_to_account[room_.id_to_name[id]].friends_to_history[clear_name].clear();
                        const char *msg = "Clear successfully\0";
                        memcpy(read_msg_.body(), msg, strlen(msg));
                        *(read_msg_.body() + strlen(msg)) = '\0';
                        read_msg_.body_length_ = strlen(msg);
                        std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    }
                    room_.deliver(read_msg_);
                }else if(read_msg_.type() == ADDR){
                    std::string addr = "=========================\nThe contacts of " + std::string(read_msg_.sender_name()) + ":\n";
                    for(auto it = room_.name_to_account[read_msg_.sender_name()].friends_to_history.begin(); it!=room_.name_to_account[read_msg_.sender_name()].friends_to_history.end(); it++){
                        addr += it->first + std::string("\n");
                    }
                    addr += "=========================\n";
                    memcpy(read_msg_.body(), addr.c_str(), addr.length());
                    *(read_msg_.body() + addr.length()) = '\0';
                    // printf("debug: the body is %s\n", read_msg_.body());
                    read_msg_.body_length_ = addr.length();
                    std::sprintf(read_msg_.get_length_ptr(), "%4d",static_cast<int>(read_msg_.body_length_));
                    room_.deliver(read_msg_);
                }
            }
        }


        void do_read_body()
        {
            auto self(shared_from_this());
            boost::asio::async_read(socket_,
                    boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                    [this, self](boost::system::error_code ec, std::size_t /*length*/)
                    {   
                        if (!ec){
                            operations();
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