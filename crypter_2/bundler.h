#pragma once
#include <string>
#include <vector>

bool bundle_with_carrier(const std::string& carrier_file, const std::vector<unsigned char>& encrypted_data, const std::string& output_file);
