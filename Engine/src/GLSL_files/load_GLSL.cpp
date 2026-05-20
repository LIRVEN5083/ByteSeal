#include "load_GLSL.hpp"

MyFile::file_read::file_read(const std::string& file_path){
	std::ifstream file(file_path, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("ERROR:: file is not opened!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	readFile = buffer;
	file.close();
}

std::vector<char> MyFile::file_read::getSource() const{
	return readFile;
}