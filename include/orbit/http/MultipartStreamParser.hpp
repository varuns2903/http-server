#pragma once
#include <string>
#include <string_view>
#include <functional>
#include <fstream>
#include <filesystem>

namespace http {

/**
 * @brief Streaming parser for multipart/form-data.
 */
class MultipartStreamParser {
public:
    using OnFieldCallback = std::function<void(const std::string& name, const std::string& value)>;
    using OnFileCallback = std::function<void(const std::string& name, const std::string& filename, const std::string& content_type, const std::string& tmp_filepath)>;

    /**
     * @brief Constructs a new MultipartStreamParser.
     * @param boundary The boundary string from the Content-Type header.
     * @param on_field Callback invoked when a standard form field is parsed.
     * @param on_file Callback invoked when a file is parsed and saved to a temporary location.
     */
    MultipartStreamParser(const std::string& boundary, OnFieldCallback on_field, OnFileCallback on_file);
    ~MultipartStreamParser();

    /**
     * @brief Feeds a chunk of data into the parser.
     * @param chunk The data chunk to parse.
     */
    void feed(std::string_view chunk);

    /**
     * @brief Signals the end of the input stream.
     */
    void end();

private:
    std::string boundary_;
    std::string dash_boundary_;
    OnFieldCallback on_field_;
    OnFileCallback on_file_;

    enum class State {
        FINDING_BOUNDARY,
        READING_HEADERS,
        READING_BODY
    };

    State state_ = State::FINDING_BOUNDARY;
    std::string buffer_;
    
    std::string current_name_;
    std::string current_filename_;
    std::string current_content_type_;
    std::string current_field_value_;
    std::ofstream current_file_;
    std::string current_tmp_filepath_;
    
    void process_buffer();
    void parse_headers(std::string_view headers);
    void open_temp_file();
    void close_current_part();
};

} // namespace http
