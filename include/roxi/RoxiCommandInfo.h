#pragma once

#include <string>

extern const char *default_roxi_conf_path;
extern char *optarg;

namespace roxi {

class RoxiCommandInfo
{
public:
    RoxiCommandInfo() : path_(default_roxi_conf_path) {}

    const char *path() {
        return path_.c_str();
    }

    void parse_config( int argc, char* argv[] ) {
        int c;

        while ((c = getopt(argc, argv, "f:")) != -1) {
            switch(c) {
            case 'f':
                path_ = ((const char *)optarg);
                break;
            }
        }
    }

private:
    std::string path_;
};

}
