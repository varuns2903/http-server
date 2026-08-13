#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace http {

struct MultipartFile {
    std::string name;
    std::string filename;
    std::string content_type;
    std::string_view content;
};

class MultipartForm {
public:
    std::unordered_map<std::string, std::string> fields;
    std::vector<MultipartFile> files;

    static MultipartForm parse(std::string_view content_type_header, std::string_view body);
};

} // namespace http
