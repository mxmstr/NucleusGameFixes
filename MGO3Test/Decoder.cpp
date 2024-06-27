#include "Settings.h"
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
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
using namespace std;

#define DECODE_PACK ">l"

class Decoder {
public:
    Blowfish __static_blowfish;
    Blowfish* __session_blowfish;
    vector<uint8_t> __crypto_key;

    Decoder(const vector<uint8_t>& static_key = {}, const vector<uint8_t>& crypto_key = {}) {
        __static_blowfish = Blowfish();
        if (static_key.empty()) {
            ifstream file(Settings::STATIC_KEY_FILE_PATH, ios::binary);
            vector<uint8_t> key((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            key.resize(16);
            __static_blowfish.initialize(key);
        }
        else {
            __static_blowfish.initialize(static_key);
        }

        __session_blowfish = nullptr;
        __crypto_key = {};

        if (!crypto_key.empty()) {
            __crypto_key = crypto_key;
            __init_session_blowfish__();
        }
    }

    std::string base64_decode(const std::string& encoded_string) {
        using namespace boost::archive::iterators;
        typedef transform_width<binary_from_base64<std::string::const_iterator>, 8, 6> It;
        return std::string(It(encoded_string.begin()), It(encoded_string.end()));
    }

    std::string decompress(const std::string& str) {
        z_stream zs;                        // z_stream is zlib's control structure
        memset(&zs, 0, sizeof(zs));

        if (inflateInit(&zs) != Z_OK)
            throw(std::runtime_error("inflateInit failed while decompressing."));

        zs.next_in = (Bytef*)str.data();
        zs.avail_in = str.size();

        int ret;
        char outbuffer[32768];
        std::string outstring;

        // get the decompressed bytes blockwise using repeated calls to inflate
        do {
            zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
            zs.avail_out = sizeof(outbuffer);

            ret = inflate(&zs, 0);

            if (outstring.size() < zs.total_out) {
                outstring.append(outbuffer,
                    zs.total_out - outstring.size());
            }

        } while (ret == Z_OK);

        inflateEnd(&zs);

        if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
            std::ostringstream oss;
            oss << "Exception during zlib decompression: (" << ret << ") " << zs.msg;
            throw(std::runtime_error(oss.str()));
        }

        return outstring;
    }

    Json::Value decode(const string& data) {
        string data_encoded_str;
        vector<uint8_t> data_encoded;
        vector<uint8_t> data_decoded;

        bool exceptionThrown = false;
        try {
            data_encoded_str = base64_decode(data);
            //data_encoded = vector<uint8_t>(data_encoded_str.begin(), data_encoded_str.end());
        }
        catch (const std::exception& e) {
            exceptionThrown = true;
            throw e;
        }
        if (!exceptionThrown) {
            data_decoded = __decipher__(__static_blowfish, data_encoded_str);
            cout << "Decoded data: " << string(data_decoded.begin(), data_decoded.end()) << endl;
        }

        Json::Value data_json;
        try {
            data_json = __get_json__(string(data_decoded.begin(), data_decoded.end()));
            cout << "Decoded JSON: " << data_json.toStyledString() << endl;
        }
        catch (const exception& e) {
            cout << "Failed to parse JSON: " << e.what() << endl;
            throw e;
        }

        //if (__session_blowfish == nullptr) {
        //    __get_crypto_key__(data_json);
        //    if (!__crypto_key.empty()) {
        //        __init_session_blowfish__();
        //    }
        //}

        //if (data_json["session_crypto"].asBool()) {
        //    if (__session_blowfish) {
        //        string embedded_str = base64_decode(data_json["data"].asString());
        //        vector<uint8_t> embedded = vector<uint8_t>(embedded_str.begin(), embedded_str.end());
        //        vector<uint8_t> text_bytes = __decipher__(*__session_blowfish, embedded);
        //        data_json["data"] = string(text_bytes.begin(), text_bytes.end());
        //        if (data_json["compress"].asBool()) {
        //            data_json["data"] = decompress(data_json["data"].asString());
        //        }
        //    }
        //}
        //else {
        //    if (data_json["compress"].asBool()) {
        //        data_json["data"] = decompress(base64_decode(data_json["data"].asString()));
        //    }
        //}

        //if (data_json["data"].isString()) {
        //    data_json["data"] = data_json["data"].asString();
        //}

        //if (data_json.isMember("original_size") && data_json["data"].isString()) {
        //    data_json["data"] = data_json["data"].asString().substr(0, data_json["original_size"].asUInt());
        //    try {
        //        Json::Value j;
        //        Json::CharReaderBuilder reader;
        //        string errs;
        //        istringstream s(data_json["data"].asString());
        //        if (Json::parseFromStream(reader, s, &j, &errs)) {
        //            data_json["data"] = j;
        //        }
        //    }
        //    catch (const exception& e) {
        //        // not json, skipping
        //    }
        //}
        return data_json;
    }

//private:

    void __get_crypto_key__(Json::Value& data) {
        if (data.isMember("data") && data["data"].isObject() && data["data"].isMember("crypto_key")) {
            if (!data["data"]["crypto_key"].asString().empty()) {
                string __crypto_key_str = base64_decode(data["data"]["crypto_key"].asString());
                __crypto_key = vector<uint8_t>(__crypto_key_str.begin(), __crypto_key_str.end());
            }
        }
    }

    void __init_session_blowfish__(const vector<uint8_t>& crypto_key = {}) {
        __session_blowfish = new Blowfish();
        if (!crypto_key.empty()) {
            __crypto_key = crypto_key;
        }
        __session_blowfish->initialize(__crypto_key);
    }

    std::string replace_all(std::string str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        return str;
    }

    Json::Value __get_json__(const string& text) {
        string modified_text = text.substr(0, text.rfind('}') + 1);
        modified_text = replace_all(modified_text, "\\\\r\\\\n", "");
        modified_text = replace_all(modified_text, "\\r\\n", "");
        modified_text = replace_all(modified_text, "\\", "");
        modified_text = replace_all(modified_text, "\"{", "{");
        modified_text = replace_all(modified_text, "}\"", "}");
        Json::Value json;
        Json::CharReaderBuilder reader;
        string errs;
        istringstream s(modified_text);
        if (Json::parseFromStream(reader, s, &json, &errs)) {
            return json;
        }
        throw runtime_error("Failed to parse JSON");
    }

    template <typename T>
    T struct_unpack(const void* buffer) {
        T ret;
        memcpy(&ret, buffer, sizeof(T));
        return ret;
    }

    template <typename T>
    std::vector<uint8_t> struct_pack(T value) {
        std::vector<uint8_t> bytes(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));
        return bytes;
    }

    std::vector<uint8_t> __decipher__(Blowfish& blow, std::string& data) {
        const size_t chunkSize = 8;
        size_t offset = 0;
        std::vector<uint8_t> fullText;

        while (offset < data.size()) {
            std::string chunk = data.substr(offset, chunkSize);
            uint32_t x, y;

            // Assuming DECODE_PACK is a valid format string for unpacking
            std::memcpy(&x, chunk.data(), sizeof(uint32_t));
            std::memcpy(&y, chunk.data() + sizeof(uint32_t), sizeof(uint32_t));

            blow.blowfish_decipher(x, y);

            fullText.insert(fullText.end(), reinterpret_cast<uint8_t*>(&x), reinterpret_cast<uint8_t*>(&x) + sizeof(uint32_t));
            fullText.insert(fullText.end(), reinterpret_cast<uint8_t*>(&y), reinterpret_cast<uint8_t*>(&y) + sizeof(uint32_t));

            offset += chunkSize;
        }

        return fullText;
    }

    /*vector<uint8_t> __decipher__(Blowfish& blow, const vector<uint8_t>& data) {
        size_t offset = 0;
        vector<uint8_t> full_text;
        while (offset != data.size()) {
            vector<uint8_t> chunk(data.begin() + offset, data.begin() + offset + 8);
            uint32_t x = struct_unpack<uint32_t>(chunk.data());
            uint32_t y = struct_unpack<uint32_t>(chunk.data() + 4);
            tie(x, y) = blow.blowfish_decipher(x, y);

            vector<uint8_t> x_text = struct_pack<uint32_t>(x);
            vector<uint8_t> y_text = struct_pack<uint32_t>(y);

            full_text.insert(full_text.end(), x_text.begin(), x_text.end());
            full_text.insert(full_text.end(), y_text.begin(), y_text.end());
            offset += 8;
        }
        return full_text;
    }*/
};


