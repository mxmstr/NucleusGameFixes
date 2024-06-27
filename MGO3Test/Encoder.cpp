#include "Settings.h"
#include "Blowfish.cpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <json/json.h>
#include <zlib.h>
#include <stdexcept>
#include <cstring>
#include <cstdint>

class Encoder {
private:
    Blowfish __static_blowfish;
    Blowfish __session_blowfish;
    std::vector<uint8_t> __crypto_key;

    std::vector<uint8_t> __encipher__(Blowfish& blow, std::vector<uint8_t>& data) {
        std::vector<uint8_t> full_text;
        for (size_t offset = 0; offset < data.size(); offset += 8) {
            std::vector<uint8_t> chunk(data.begin() + offset, data.begin() + offset + 8);
            uint32_t x = 0, y = 0;
            std::memcpy(&x, chunk.data(), sizeof(uint32_t));
            std::memcpy(&y, chunk.data() + sizeof(uint32_t), sizeof(uint32_t));

            blow.blowfish_encipher(x, y);

            std::vector<uint8_t> x_text(sizeof(uint32_t)), y_text(sizeof(uint32_t));
            std::memcpy(x_text.data(), &x, sizeof(uint32_t));
            std::memcpy(y_text.data(), &y, sizeof(uint32_t));

            full_text.insert(full_text.end(), x_text.begin(), x_text.end());
            full_text.insert(full_text.end(), y_text.begin(), y_text.end());
        }
        return full_text;
    }

    void __add_padding__(std::vector<uint8_t>& text) {
        if (text.size() % 8 != 0) {
            uint8_t padding_size = 8 - (text.size() % 8);
            text.insert(text.end(), padding_size, padding_size);
        }
    }

    std::string __compress_data__( Json::Value& data) {
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string text = Json::writeString(writer, data);
        std::replace(text.begin(), text.end(), ',', ',');
        std::replace(text.begin(), text.end(), ':', ':');
        return text;
    }

public:
    Encoder(std::vector<uint8_t>& static_key, std::vector<uint8_t>& crypto_key) {
        if (static_key.empty()) {
            std::ifstream key_file(Settings::STATIC_KEY_FILE_PATH, std::ios::binary);
            if (!key_file) {
                throw std::runtime_error("Failed to open static key file");
            }
            // resize static_key to 16 bytes
            static_key.resize(16);
            key_file.read(reinterpret_cast<char*>(static_key.data()), static_key.size());
        }
        __static_blowfish.initialize(static_key);

        if (!crypto_key.empty()) {
            __crypto_key = crypto_key;
            std::vector<uint8_t> empty_key;
            __init_session_blowfish__(empty_key);
        }
    }

    void __init_session_blowfish__(std::vector<uint8_t>& crypto_key) {
        if (!crypto_key.empty()) {
            __crypto_key = crypto_key;
        }
        __session_blowfish.initialize(__crypto_key);
    }

    std::vector<uint8_t> encode(Json::Value& _command) {
        if (!_command.isObject()) {
            throw std::invalid_argument("Command must be a JSON object");
        }

        if (!_command.isMember("data")) {
            throw std::invalid_argument("Command must contain 'data' field");
        }

        Json::Value command = _command;
        std::vector<uint8_t> encoded_data;

        if (command["data"].isObject()) {
            std::string compressed_data = __compress_data__(command["data"]);
            command["data"] = compressed_data;
            command["original_size"] = static_cast<int>(compressed_data.size());

            if (command["compress"].asBool() && !command["session_crypto"].asBool()) {
                std::vector<uint8_t> compressed_data_bytes(compressed_data.begin(), compressed_data.end());
                std::vector<uint8_t> compressed_data_zlib(compressed_data.size() + 1);
                uLongf compressed_data_zlib_size = compressed_data_zlib.size();
                int zlib_result = compress2(compressed_data_zlib.data(), &compressed_data_zlib_size, compressed_data_bytes.data(), compressed_data_bytes.size(), Z_BEST_COMPRESSION);
                if (zlib_result != Z_OK) {
                    throw std::runtime_error("Failed to compress data using zlib");
                }
                compressed_data_zlib.resize(compressed_data_zlib_size);
                command["data"] = std::string(compressed_data_zlib.begin(), compressed_data_zlib.end());

                std::string str = command["data"].asString();
                for (size_t pos = str.find('\n'); pos != std::string::npos; pos = str.find('\n', pos + 2)) {
                    str.replace(pos, 1, "\r\n");
                }
                command["data"] = str;
                //std::replace(command["data"].asString().begin(), command["data"].asString().end(), '\n', '\r\n');
            }

            if (command["session_crypto"].asBool()) {
                if (command["compress"].asBool()) {
                    std::vector<uint8_t> compressed_data_bytes(compressed_data.begin(), compressed_data.end());
                    std::vector<uint8_t> compressed_data_zlib(compressed_data.size() + 1);
                    uLongf compressed_data_zlib_size = compressed_data_zlib.size();
                    int zlib_result = compress2(compressed_data_zlib.data(), &compressed_data_zlib_size, compressed_data_bytes.data(), compressed_data_bytes.size(), Z_BEST_COMPRESSION);
                    if (zlib_result != Z_OK) {
                        throw std::runtime_error("Failed to compress data using zlib");
                    }
                    compressed_data_zlib.resize(compressed_data_zlib_size);
                    command["data"] = std::string(compressed_data_zlib.begin(), compressed_data_zlib.end());
                    __add_padding__(compressed_data_zlib);
                    command["data"] = std::string(compressed_data_zlib.begin(), compressed_data_zlib.end());
                } else {

                    std::vector<uint8_t> compressed_data_bytes;
                    compressed_data_bytes.insert(compressed_data_bytes.end(), compressed_data.begin(), compressed_data.end());
                    __add_padding__(compressed_data_bytes);
                    command["data"] = std::string(compressed_data_bytes.begin(), compressed_data_bytes.end());
                }
                std::vector<uint8_t> data_bytes = std::vector<uint8_t>(command["data"].asString().begin(), command["data"].asString().end());
                std::vector<uint8_t> encrypted_data = __encipher__(__session_blowfish, data_bytes);
                command["data"] = std::string(encrypted_data.begin(), encrypted_data.end());

                std::string str = command["data"].asString();
                for (size_t pos = str.find('\n'); pos != std::string::npos; pos = str.find('\n', pos + 2)) {
                    str.replace(pos, 1, "\r\n");
                }
                command["data"] = str;

                //std::replace(command["data"].asString().begin(), command["data"].asString().end(), '\n', '\r\n');
                command["data"] = command["data"].asString().substr(0, command["data"].asString().find_last_not_of("\r\n") + 1);
            }
        }

        std::string text = command.toStyledString();
        text.erase(std::remove(text.begin(), text.end(), ' '), text.end());
        std::vector<uint8_t> padded_text = std::vector<uint8_t>(text.begin(), text.end());
        __add_padding__(padded_text);
        std::vector<uint8_t> encrypted_text = __encipher__(__static_blowfish, padded_text);
        std::string encoded_text(encrypted_text.begin(), encrypted_text.end());

        for (size_t pos = encoded_text.find('\n'); pos != std::string::npos; pos = encoded_text.find('\n', pos + 2)) {
            encoded_text.replace(pos, 1, "\r\n");
        }

        //std::replace(encoded_text.begin(), encoded_text.end(), '\n', '\r\n');
        encoded_text = encoded_text.substr(0, encoded_text.find_last_not_of("\r\n") + 1);

        return std::vector<uint8_t>(encoded_text.begin(), encoded_text.end());
    }
};
