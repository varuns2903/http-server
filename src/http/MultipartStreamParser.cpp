#include <orbit/http/MultipartStreamParser.hpp>
#include <iostream>
#include <sstream>

namespace http {

MultipartStreamParser::MultipartStreamParser(const std::string& boundary, OnFieldCallback on_field, OnFileCallback on_file)
    : boundary_(boundary), on_field_(std::move(on_field)), on_file_(std::move(on_file)) {
    dash_boundary_ = "--" + boundary_;
}

MultipartStreamParser::~MultipartStreamParser() {
    close_current_part();
}

void MultipartStreamParser::feed(std::string_view chunk) {
    buffer_.append(chunk);
    process_buffer();
}

void MultipartStreamParser::end() {
    // Flush any remaining
    process_buffer();
    close_current_part();
}

void MultipartStreamParser::process_buffer() {
    while (!buffer_.empty()) {
        if (state_ == State::FINDING_BOUNDARY) {
            size_t pos = buffer_.find(dash_boundary_);
            if (pos == std::string::npos) {
                if (buffer_.length() > dash_boundary_.length()) {
                    buffer_.erase(0, buffer_.length() - dash_boundary_.length());
                }
                break;
            }
            
            buffer_.erase(0, pos + dash_boundary_.length());
            
            // Check for \r\n or --
            if (buffer_.length() >= 2) {
                if (buffer_.substr(0, 2) == "--") {
                    // EOF
                    buffer_.clear();
                    break;
                } else if (buffer_.substr(0, 2) == "\r\n") {
                    buffer_.erase(0, 2);
                    state_ = State::READING_HEADERS;
                } else {
                    // Malformed, just skip
                    buffer_.erase(0, 2);
                }
            } else {
                break; // wait for more
            }
            
        } else if (state_ == State::READING_HEADERS) {
            size_t pos = buffer_.find("\r\n\r\n");
            if (pos == std::string::npos) {
                if (buffer_.length() > 8192) {
                    buffer_.clear(); // protection
                }
                break;
            }
            
            parse_headers(buffer_.substr(0, pos));
            buffer_.erase(0, pos + 4);
            
            if (!current_filename_.empty()) {
                open_temp_file();
            } else {
                current_field_value_.clear();
            }
            state_ = State::READING_BODY;
            
        } else if (state_ == State::READING_BODY) {
            std::string boundary_sig = "\r\n" + dash_boundary_;
            size_t pos = buffer_.find(boundary_sig);
            
            if (pos == std::string::npos) {
                if (buffer_.length() > boundary_sig.length()) {
                    size_t write_len = buffer_.length() - boundary_sig.length();
                    std::string_view to_write = std::string_view(buffer_).substr(0, write_len);
                    
                    if (current_file_.is_open()) {
                        current_file_.write(to_write.data(), static_cast<std::streamsize>(to_write.length()));
                    } else {
                        current_field_value_.append(to_write);
                    }
                    buffer_.erase(0, write_len);
                }
                break;
            } else {
                std::string_view to_write = std::string_view(buffer_).substr(0, pos);
                if (current_file_.is_open()) {
                    current_file_.write(to_write.data(), static_cast<std::streamsize>(to_write.length()));
                } else {
                    current_field_value_.append(to_write);
                }
                
                close_current_part();
                
                buffer_.erase(0, pos + 2); // Erase \r\n
                state_ = State::FINDING_BOUNDARY;
            }
        }
    }
}

void MultipartStreamParser::parse_headers(std::string_view headers) {
    current_name_.clear();
    current_filename_.clear();
    current_content_type_.clear();
    
    size_t h_start = 0;
    while (h_start < headers.length()) {
        size_t h_end = headers.find("\r\n", h_start);
        if (h_end == std::string::npos) h_end = headers.length();
        
        std::string_view header_line = headers.substr(h_start, h_end - h_start);
        
        if (header_line.starts_with("Content-Disposition:")) {
            size_t name_pos = header_line.find("name=\"");
            if (name_pos != std::string::npos) {
                size_t name_end = header_line.find("\"", name_pos + 6);
                if (name_end != std::string::npos) {
                    current_name_ = header_line.substr(name_pos + 6, name_end - (name_pos + 6));
                }
            }
            size_t fn_pos = header_line.find("filename=\"");
            if (fn_pos != std::string::npos) {
                size_t fn_end = header_line.find("\"", fn_pos + 10);
                if (fn_end != std::string::npos) {
                    current_filename_ = header_line.substr(fn_pos + 10, fn_end - (fn_pos + 10));
                }
            }
        } else if (header_line.starts_with("Content-Type:")) {
            size_t ct_pos = header_line.find(":");
            if (ct_pos != std::string::npos) {
                current_content_type_ = header_line.substr(ct_pos + 1);
                while (!current_content_type_.empty() && current_content_type_.front() == ' ') {
                    current_content_type_.erase(0, 1);
                }
            }
        }
        h_start = h_end + 2;
    }
}

void MultipartStreamParser::open_temp_file() {
    std::filesystem::create_directories("/tmp/orbit_uploads");
    
    // Generate simple random/temp filename
    static int counter = 0;
    current_tmp_filepath_ = "/tmp/orbit_uploads/upload_" + std::to_string(time(nullptr)) + "_" + std::to_string(counter++);
    
    current_file_.open(current_tmp_filepath_, std::ios::binary);
}

void MultipartStreamParser::close_current_part() {
    if (current_file_.is_open()) {
        current_file_.close();
        if (on_file_ && !current_name_.empty()) {
            on_file_(current_name_, current_filename_, current_content_type_, current_tmp_filepath_);
        }
    } else {
        if (on_field_ && !current_name_.empty()) {
            on_field_(current_name_, current_field_value_);
        }
    }
    current_name_.clear();
    current_filename_.clear();
    current_content_type_.clear();
    current_field_value_.clear();
    current_tmp_filepath_.clear();
}

} // namespace http
