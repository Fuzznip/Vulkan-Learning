#pragma once

[[nodiscard]]
std::optional<std::vector<unsigned char>> read_file(const std::string& filePath);
