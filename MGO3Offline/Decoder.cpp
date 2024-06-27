#include "stdafx.h"
#include "Blowfish.cpp"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <json/json.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <unistd.h>

#define DECODE_PACK ">l"
#if defined(__linux__) && __SIZEOF_POINTER__ > 4
    #define DECODE_PACK ">L"
#endif

class Decoder {
public:
    Blowfish __static_blowfish;
    Blowfish* __session_blowfish;
    std::vector<uint8_t> __crypto_key;

    std::vector<uint8_t> read_file(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + file_path);
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            throw std::runtime_error("Failed to read file: " + file_path);
        }
        return buffer;
    }

    std::vector<uint8_t> base64_decode(const std::string& input) {
        static const std::string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::vector<uint8_t> output;
        size_t in_len = input.size();
        size_t i = 0;
        uint32_t a, b, c, d;

        while (i < in_len) {
            do {
                a = base64_chars.find(input[i++]);
            } while (i < in_len && a == std::string::npos);

            do {
                b = base64_chars.find(input[i++]);
            } while (i < in_len && b == std::string::npos);

            do {
                c = base64_chars.find(input[i++]);
            } while (i < in_len && c == std::string::npos);

            do {
                d = base64_chars.find(input[i++]);
            } while (i < in_len && d == std::string::npos);

            if (a != std::string::npos && b != std::string::npos && c != std::string::npos && d != std::string::npos) {
                output.push_back((a << 2) | (b >> 4));
                output.push_back(((b & 0x0F) << 4) | (c >> 2));
                output.push_back(((c & 0x03) << 6) | (d & 0x3F));
            }
        }
        return output;
    }

    std::vector<uint8_t> decipher(Blowfish* blow, const std::vector<uint8_t>& data) {
        std::vector<uint8_t> full_text;
        size_t offset = 0;
        while (offset < data.size()) {
            std::vector<uint8_t> chunk(data.begin() + offset, data.begin() + offset + 8);
            uint32_t x = *reinterpret_cast<uint32_t*>(chunk.data());
            uint32_t y = *reinterpret_cast<uint32_t*>(chunk.data() + 4);
            blow->blowfish_decipher(x, y);
            full_text.insert(full_text.end(), reinterpret_cast<uint8_t*>(&x), reinterpret_cast<uint8_t*>(&x) + 4);
            full_text.insert(full_text.end(), reinterpret_cast<uint8_t*>(&y), reinterpret_cast<uint8_t*>(&y) + 4);
            offset += 8;
        }
        return full_text;
    }

    Json::Value get_json(const std::string& text) {
        std::string cleaned_text = text.substr(0, text.rfind('}') + 1);
        cleaned_text.erase(std::remove(cleaned_text.begin(), cleaned_text.end(), '\\'), cleaned_text.end());
        cleaned_text.erase(std::remove(cleaned_text.begin(), cleaned_text.end(), '"'), cleaned_text.end());
        Json::Value json;
        Json::CharReaderBuilder builder;
        std::istringstream iss(cleaned_text);
        std::string errs;
        if (!Json::parseFromStream(builder, iss, &json, &errs)) {
            throw std::runtime_error("Failed to parse JSON: " + errs);
        }
        return json;
    }

    void init_session_blowfish(const std::vector<uint8_t>& crypto_key) {
        __session_blowfish = new Blowfish();
        __session_blowfish->initialize(crypto_key);
    }

    std::vector<uint8_t> decompress(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> decompressed_data;
        z_stream zs;
        memset(&zs, 0, sizeof(zs));
        if (inflateInit(&zs) != Z_OK) {
            throw std::runtime_error("Failed to initialize zlib inflate");
        }
        zs.next_in = const_cast<Bytef*>(data.data());
        zs.avail_in = data.size();
        int ret;
        char outbuffer[32768];
        do {
            zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
            zs.avail_out = sizeof(outbuffer);
            ret = inflate(&zs, Z_NO_FLUSH);
            if (decompressed_data.size() < zs.total_out) {
                decompressed_data.insert(decompressed_data.end(), outbuffer, outbuffer + (zs.total_out - decompressed_data.size()));
            }
        } while (ret == Z_OK);
        inflateEnd(&zs);
        if (ret != Z_STREAM_END) {
            throw std::runtime_error("Failed to decompress data");
        }
        return decompressed_data;
    }
    
    Decoder(const std::vector<uint8_t>& static_key = std::vector<uint8_t>(), const std::vector<uint8_t>& crypto_key = std::vector<uint8_t>()) {
        if (static_key.empty()) {
            std::vector<uint8_t> key = read_file(Settings::STATIC_KEY_FILE_PATH);
            __static_blowfish.initialize(key);
        } else {
            __static_blowfish.initialize(static_key);
        }
        __session_blowfish = nullptr;
        __crypto_key = crypto_key;
        if (!crypto_key.empty()) {
            init_session_blowfish(crypto_key);
        }
    }

    ~Decoder() {
        delete __session_blowfish;
    }

    Json::Value decode(const std::string& data) {
        std::vector<uint8_t> data_encoded = base64_decode(data);
        std::vector<uint8_t> data_decoded = decipher(&__static_blowfish, data_encoded);
        std::string decoded_data(reinterpret_cast<char*>(data_decoded.data()), data_decoded.size());
        Json::Value data_json = get_json(decoded_data);
        if (!__session_blowfish) {
            if (data_json["session_crypto"].asBool()) {
                __crypto_key = base64_decode(data_json["data"].asString());
                init_session_blowfish(__crypto_key);
            }
        }
        if (data_json["session_crypto"].asBool()) {
            if (__session_blowfish) {
                std::vector<uint8_t> embedded = base64_decode(data_json["data"].asString());
                std::vector<uint8_t> deciphered = decipher(__session_blowfish, embedded);
                std::string deciphered_str(deciphered.begin(), deciphered.end());
                data_json["data"] = deciphered_str;
                if (data_json["compress"].asBool()) {
                    std::vector<uint8_t> decompressed = decompress(deciphered);
                    std::string decompressed_str(decompressed.begin(), decompressed.end());
                    data_json["data"] = decompressed_str;
                }
            }
        } else {
            if (data_json["compress"].asBool()) {
                std::vector<uint8_t> decompressed = decompress(base64_decode(data_json["data"].asString()));
                std::string decompressed_str(decompressed.begin(), decompressed.end());
                data_json["data"] = decompressed_str;
            }
        }
        if (data_json.isMember("original_size") && data_json["data"].isString()) {
            data_json["data"] = data_json["data"].asString().substr(0, data_json["original_size"].asInt());
            try {
                Json::Value j;
                std::istringstream iss(data_json["data"].asString());
                iss >> j;
                data_json["data"] = j;
            } catch (const std::exception&) {
                // Not JSON, skipping
            }
        }
        return data_json;
    }
};
