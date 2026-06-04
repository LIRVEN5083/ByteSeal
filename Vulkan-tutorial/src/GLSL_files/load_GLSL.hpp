#ifndef _LOAD_GLSL_HPP
#define _LOAD_GLSL_HPP

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>

namespace MyFile {
	class file_read {
	public:
		file_read(const std::string& file_path);

		std::vector<char> getSource() const;
	private:
		std::vector<char> readFile;
	};
}

#endif