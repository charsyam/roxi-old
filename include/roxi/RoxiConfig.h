#pragma once

#include <vector>
#include <string>
#include <json/reader.h>
#include <json/value.h>

namespace roxi {

class RoxiServerPair 
{ 
public:
    RoxiServerPair( std::string _master, std::string _slave )
        : master_(_master), slave_(_slave) {}

    const char* master(){ 
        return master_.c_str();
    }

    const char* slave(){
        return slave_.c_str();
    }

private: 
    std::string master_; 
    std::string slave_; 
}; 
 
class RoxiConfig 
{ 
public: 
    int port() { 
        return port_; 
    } 

    const char* host() {
        return host_.c_str();
    }
 
    int getServerPairCount() { 
        return server_pairs_.size(); 
    } 
     
    RoxiServerPair &server_pairs(int idx) { 
        return server_pairs_[idx]; 
    } 
 
    int load_conf( const char *path ) {
        Json::Value root;
        Json::Reader reader;

        std::ifstream is;
        is.open(path);

        bool parsingSuccessful = reader.parse( is, root );
        if ( !parsingSuccessful )
        {
            // report to the user the failure and their locations in the document.
            std::cout  << "Failed to parse configuration\n" 
                        << reader.getFormatedErrorMessages() << std::endl;
            return -1;
        }

        port_ = root.get("port", 9999).asInt();
        host_ = root.get("host", "0.0.0.0").asString();

        const Json::Value server_pairs = root["server_pairs"];
        if( server_pairs.size() == 0 ) {
            std::cout << "No Server Pairs\n";
            return -1;
        }

        for ( int index = 0; index < server_pairs.size(); ++index ) {
            const Json::Value value = server_pairs[index];
            RoxiServerPair info( value["master"].asString(), value["slave"].asString() );
            server_pairs_.push_back( info );
        }

        return 0;
    }
 
private: 
    std::vector<RoxiServerPair> server_pairs_;  
    int port_; 
    std::string host_;
};

}
