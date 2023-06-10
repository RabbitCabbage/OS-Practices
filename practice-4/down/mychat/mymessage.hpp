#ifndef MYMESSAGE_HPP
#define MYMESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <boost/regex.hpp>

#define LIST 1
#define LOGIN 2
#define ADD 3
#define DEL 4
#define SEND 5
#define ADDR 6
#define HISTORY 7
#define CLEAR 8
#define HELP 9

boost::regex list_regex("^list$");
boost::regex add_regex("^add\\s+([a-zA-Z0-9_]{1,20})$");
boost::regex del_regex("^del\\s+([a-zA-Z0-9_]{1,20})$");
boost::regex send_regex("^send\\s+([a-zA-Z0-9_]{1,20})\\s+(.{1,512})$");
boost::regex sendall_regex("^send all\\s+(.{1,512})$");
boost::regex addr_regex("^friends$");
boost::regex history_regex("^history\\s+([a-zA-Z0-9_]{1,20})$");
boost::regex clear_regex("^clear\\s+([a-zA-Z0-9_]{1,20})$");
boost::regex login_regex("^login\\s+([a-zA-Z0-9_]{1,20})\\s+([a-zA-Z0-9_]{1,20})$");

class mymessage
{
    public:
        // HEADER + BODY
        // header = type + sender_len + sender + object_len + object + length + padding
        // body = content
        static constexpr std::size_t max_name_length = 20;
        static constexpr std::size_t max_passwd_length = 20;
        static constexpr std::size_t header_length = 4+4+4+4+2*max_name_length+4;
        static constexpr std::size_t max_body_length = 1024;
    private:
        char data_[header_length + max_body_length];
        char original_data[10 + max_name_length + max_body_length];
        char sender[max_name_length + 1];
    public:
        char object[max_name_length + 1];
        std::size_t body_length_;
    private:
        int message_type;
        char *get_type_ptr(){return data_;}
        char *get_sender_len_ptr(){return data_ + 4;}
        char *get_sender_ptr(){return data_ + 4 + 4;}
        
    public:
        mymessage(const char *line,const char *sender)
        {
            memcpy(original_data, line, strlen(line));
            original_data[strlen(line)] = '\0';
            memcpy(this->sender, sender, strlen(sender));
            this->sender[strlen(sender)] = '\0';
        }

        mymessage()
            : body_length_(0)   
        {}

        const char* data() const
        {
            return data_;
        }

        char* data()
        {
            return data_;
        }

        std::size_t length() const
        {
            return header_length + body_length_;
        }

        const char* body() const
        {
            return data_ + header_length;
        }

        int type() const
        {
            return message_type;
        }

        const char* sender_name() const
        {
            return sender;
        }

        const char* object_name() const
        {
            return object;
        }
        
        char* body()
        {
            return data_ + header_length;
        }

        char *get_length_ptr(){return data_ + 4 + 4 + max_name_length + 4 + max_name_length;}
        char *get_object_ptr(){return data_ + 4 + 4 + max_name_length + 4;}
        char *get_object_len_ptr(){return data_ + 4 + 4 + max_name_length;}

        std::size_t body_length() const
        {
            return body_length_;
        }

        void body_length(std::size_t new_length)
        {
            body_length_ = new_length;
            if (body_length_ > max_body_length)
                body_length_ = max_body_length;
        }

        bool decode_header()
        {
            // header读到了data_里，现在从data_里decode出类型，做相应操作
            // header = type + sender_len + sender + object_len + object + length + padding
            message_type = std::atoi(get_type_ptr());
            body_length_ = std::atoi(get_length_ptr());
            int sender_len=std::atoi(get_sender_len_ptr());
            int obj_len=std::atoi(get_object_len_ptr());
            memcpy(sender, get_sender_ptr(), sender_len);
            sender[sender_len] = '\0';
            // printf("debug: decode, the type is %d, ", message_type);
            // printf("the len of sender is %d, the len of object is %d, ", sender_len, obj_len);
            if(message_type >= 1 && message_type <=9){
                body_length_ = std::atoi(get_length_ptr());
                memcpy(object, get_object_ptr(), obj_len);
                object[obj_len] = '\0';
                memcpy(body(), get_object_ptr() + obj_len, body_length_);
                *(body() + body_length_) = '\0';
                return true;
            }else{
                printf("failed to decode header\n");
                return false;
            }
            if (body_length_ > max_body_length)
            {
                printf("the body is too long\n");
                body_length_ = 0;
                return false;
            }
            return true;
        }

        bool encode()
        {   
            memset(data_, 0, header_length + max_body_length);
            // printf("sender is %s\n", sender);
            // body_length_, body_message, message_type
            // parse original_data
            boost::smatch what;
            // build a string from original_data
            std::string original_data_str(original_data);
            if(boost::regex_match(original_data_str,what,list_regex))
            {
                message_type = LIST;
                body_length_ = 0;
                memcpy(object,sender,strlen(sender));
                object[strlen(sender)] = '\0';
            }
            else if(boost::regex_match(original_data_str,what,add_regex))
            {
                message_type = ADD;
                body_length_ = 0;
                memcpy(object,what[1].str().c_str(),strlen(what[1].str().c_str()));
                object[strlen(what[1].str().c_str())] = '\0';
            }
            else if(boost::regex_match(original_data_str,what,del_regex))
            {
                message_type = DEL;
                body_length_ = 0;
                memcpy(object,what[1].str().c_str(),strlen(what[1].str().c_str()));
                object[strlen(what[1].str().c_str())] = '\0';
            }
            else if(boost::regex_match(original_data_str,what,send_regex))
            {
                message_type = SEND;
                memcpy(object,what[1].str().c_str(),strlen(what[1].str().c_str()));
                object[strlen(what[1].str().c_str())] = '\0';
                body_length_ = strlen(what[2].str().c_str());
                memcpy(body(),what[2].str().c_str(),body_length_);
                *(body() + body_length_) = '\0';
            }
            else if(boost::regex_match(original_data_str,what,sendall_regex))
            {
                message_type = SEND;
                body_length_ = strlen(what[1].str().c_str());
                memcpy(body(),what[1].str().c_str(),body_length_);
                *(body() + body_length_) = '\0';
            }
            else if(boost::regex_match(original_data_str,what,addr_regex))
            {
                message_type = ADDR;
                body_length_ = 0;
                memcpy(object,sender,strlen(sender));
                object[strlen(sender)] = '\0';
                std::sprintf(get_object_len_ptr(), "%4d", static_cast<int> (strlen(object)));
            }
            else if(boost::regex_match(original_data_str,what,history_regex))
            {
                message_type = HISTORY;
                memcpy(object,what[1].str().c_str(),strlen(what[1].str().c_str()));    
                object[strlen(what[1].str().c_str())] = '\0';            
                body_length_ = 0;
            }
            else if(boost::regex_match(original_data_str,what,clear_regex))
            {
                message_type = CLEAR;
                memcpy(object,what[1].str().c_str(),strlen(what[1].str().c_str()));
                object[strlen(what[1].str().c_str())] = '\0';
                body_length_ = 0;
            }
            else if(boost::regex_match(original_data_str,what,login_regex))
            {
                message_type = LOGIN;
                memcpy(object,what[1].str().c_str(),strlen(what[1].str().c_str()));   
                object[strlen(what[1].str().c_str())] = '\0';             
                body_length_ = strlen(what[2].str().c_str());
                memcpy(body(),what[2].str().c_str(),body_length_);
                *(body() + body_length_) = '\0';
            }
            else if(strcmp(original_data,"help") == 0)
            {   
                memcpy(object,sender,strlen(sender));
                object[strlen(sender)] = '\0';
                message_type = HELP;
                body_length_ = 0;
            }
            else {
                return 1;// something wrong
            }
            // header = type + sender_len + sender + object_len + object + length + padding
            std::sprintf(get_type_ptr(), "%4d", message_type);
            std::sprintf(get_sender_len_ptr(), "%4d", static_cast<int> (strlen(sender)));
            memcpy(get_sender_ptr(), sender, strlen(sender));
            std::sprintf(get_object_len_ptr(), "%4d", static_cast<int> (strlen(object)));
            memcpy(get_object_ptr(), object, strlen(object));
            std::sprintf(get_length_ptr(), "%4d", static_cast<int> (body_length_));
            // printf("send from %s to %s\n",sender,object);
            // printf("send from %s to %s\n",get_sender_ptr(),get_object_ptr());
            return 0;
        }

};

#endif // MYMESSAGE_HPP