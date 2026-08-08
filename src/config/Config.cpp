#include "Config.hpp"
#include <iostream>
#include <cstdlib>
#include <getopt.h>

namespace config {

ServerConfig ServerConfig::parse(int argc, char* argv[]) {
    ServerConfig cfg;
    
    const char* const short_opts = "p:t:l:s:h";
    const option long_opts[] = {
        {"port", required_argument, nullptr, 'p'},
        {"threads", required_argument, nullptr, 't'},
        {"log-level", required_argument, nullptr, 'l'},
        {"static-dir", required_argument, nullptr, 's'},
        {"max-body-size", required_argument, nullptr, 'm'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, no_argument, nullptr, 0}
    };

    while (true) {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
        if (-1 == opt) break;

        switch (opt) {
            case 'p':
                cfg.port = static_cast<uint16_t>(std::stoi(optarg));
                break;
            case 't':
                cfg.worker_threads = static_cast<size_t>(std::stoull(optarg));
                break;
            case 'l':
                cfg.log_level = optarg;
                break;
            case 's':
                cfg.static_dir = optarg;
                break;
            case 'm':
                cfg.max_body_size = static_cast<size_t>(std::stoull(optarg));
                break;
            case 'h':
            case '?':
            default:
                std::cout << "Usage: " << argv[0] << " [options]\n"
                          << "  -p, --port <port>           Port to listen on (default: 8080)\n"
                          << "  -t, --threads <count>       Number of worker threads (default: 4)\n"
                          << "  -l, --log-level <level>     Log level (DEBUG, INFO, WARN, ERROR) (default: INFO)\n"
                          << "  -s, --static-dir <dir>      Directory for static files (default: ./public)\n"
                          << "  -m, --max-body-size <bytes> Maximum HTTP request body size (default: 10485760 (10MB))\n"
                          << "  -h, --help                  Show this help message\n";
                std::exit(0);
        }
    }
    return cfg;
}

} // namespace config
