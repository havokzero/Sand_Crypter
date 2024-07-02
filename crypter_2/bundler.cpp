#include "pch.h"
#include "bundler.h"
#include <fstream>
#include <vector>

bool bundle_with_carrier(const std::string& carrier_path, const std::vector<unsigned char>& payload, const std::string& output_path) {
    std::ifstream carrier(carrier_path, std::ios::binary);
    if (!carrier.is_open()) return false;

    std::ofstream output(output_path, std::ios::binary);
    if (!output.is_open()) return false;

    output << carrier.rdbuf(); // Copy carrier file to output file
    output.write(reinterpret_cast<const char*>(payload.data()), payload.size()); // Append the payload

    return true;
}
