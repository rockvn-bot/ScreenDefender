#pragma once
#include <string>
#include <vector>

// Base64 decode helpers
std::vector<unsigned char> base64_decode(const std::string& encoded_string);
bool is_base64_string(const std::string& s);
bool is_image_data(const std::vector<unsigned char>& data);
