#ifndef MYMESSAGE_HPP
#define MYMESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>

class mymessage
{
    public:
        // HEADER + BODY
        // in header, there are type for the message, and the length of the body
        static constexpr std::size_t header_length = 4;
        static constexpr std::size_t max_body_length = 512;

        mymessage(): body_length_(0)
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

        char* body()
        {
            return data_ + header_length;
        }

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
            char header[header_length + 1] = "";
            std::strncat(header, data_, header_length);
            body_length_ = std::atoi(header);
            if (body_length_ > max_body_length)
            {
                body_length_ = 0;
                return false;
            }
            return true;
        }

        void encode_header()
        {
            char header[header_length + 1] = "";
            std::sprintf(header, "%4d", static_cast<int>(body_length_));
            std::memcpy(data_, header, header_length);
        }

    private:
        char data_[header_length + max_body_length];
        std::size_t body_length_;
};

#endif // MYMESSAGE_HPP