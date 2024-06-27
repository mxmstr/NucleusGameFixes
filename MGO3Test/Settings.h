#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <vector>

class Settings {
public:
    // you need a static key to decode communications between client and server
    // where to get: inside mgsvtpp.exe
    static const std::string STATIC_KEY_FILE_PATH;
    //STATIC_KEY_FILE_PATH = '/var/www/mgsv_server/static_key.bin';
    //STATIC_KEY_FILE_PATH = "C:\\Users\\lynch\\Documents\\mgsv_emulator-2\\files\\static_key.bin";

    // logging
    static const std::string LOG_FILE_PATH;
    static const std::string LOGGER_NAME;

    // proxy
    static const bool PROXY_ALL;
    static const std::vector<std::string> PROXY_ALWAYS;

    // client authentication
    // in general, there are two cases:
    //  - if you want to emulate the client, set your steamid and magic_hash
    //  - if you want to run a server, set them empty - this is important.
    //     In case you set them non-empty, run the server and database goes down, these
    //     credentials will be used for other players - thus sending wrong data about 
    //     your account
    //
    //  you can also set them to yours, enable PROXY_ALL (or add commands into PROXY_ALWAYS)
    //  and run server locally to capture commands for debugging
    //
    // STEAM_ID is client's steamid, duh
    static const std::string STEAM_ID;

    // MAGIC_HASH is a hash returned from konami in response to CMD_REQAUTH_HTTPS
    // essentially a password
    static const std::string MAGIC_HASH;
};

#endif // SETTINGS_H
