/****************************************************************************
 *
 *  util.cpp
 *      ($nativecoin-cpp\src)
 *
 *  by icedac
 *
 ***/
#include "stdafx.h"
#include "util.h"

namespace nc {
    byte char_to_byte(const char c) {
        if (c >= '0' && c <= '9')
            return (byte)(c - '0');
        if (c >= 'a' && c <= 'f')
            return 10 + (byte)(c - 'a');
        if (c >= 'A' && c <= 'F')
            return 10 + (byte)(c - 'A');
        return -1;
    }

    bool util::is_hexstring(gsl::span< const char > bin)
    {
        if ((bin.size() % 2) == 1)
        {
            return false;
        }

        for (byte b : bin)
            if (char_to_byte(b) == -1) return false;

        return true;
    }

    string util::binary_to_hexstring(gsl::span< const byte > bin)
    {
        const char HEX_CHAR[] = "0123456789abcdef";
        ostringstream ss;
        for (auto b : bin) {
            char hex[3];
            hex[0] = HEX_CHAR[(b & 0xf0) >> 4];
            hex[1] = HEX_CHAR[(b & 0x0f) >> 0];
            hex[2] = 0;
            ss << hex;
        }
        return ss.str();
    }

    vector<byte> util::hexstring_to_binary(gsl::span< const char > bin)
    {
        if ((bin.size() % 2) == 1)
        {
            std::cout << "nc::util::string_to_binary(); binary size is wrong " << bin.size() << "\n";
            return nc::vector<nc::byte>();
        }

        const char* s = bin.data();
        size_t s_len = bin.size();

        vector< byte > data;

        for (size_t i = 0; i < s_len; i += 2) {
            byte b = char_to_byte(s[i]);
            b = b << 4;
            b += char_to_byte(s[i + 1]);
            data.push_back(b);
        }

        return data;
    }

    string normalize_newline(string s) {
        // \n -> \r\n
        // \r -> \r\n
        return s;
    }

    template<typename Out>
    void split(const std::string &s, char delim, Out result) {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }

    std::vector<std::string> split(const std::string &s, char delim) {
        std::vector<std::string> elems;
        split(s, delim, std::back_inserter(elems));
        return elems;
    }

    string util::tab_strings(string s)
    {
        string r;
        auto lines = split(s, '\n');
        for (const auto& line : lines) {
            r += "\t";
            r += line;
            r += "\n";
        }
        return r;
    }

    nc::timestamp_t util::time_since_epoch()
    {
        using std::chrono::system_clock;
        return system_clock::to_time_t(system_clock::now());
    }

}
