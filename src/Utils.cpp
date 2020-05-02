#include <CREngine/Utils.h>
#include <CREngine/MainSpace.h>

#include <fstream>
#include <sstream>
#include <iostream>

#include <SDL2/SDL.h>

using namespace CREngine::Utils;

//vars
static int nameable_cout = 0;

//namespce functions
std::string CREngine::Utils::file_to_string(const std::string &file_path) {
	std::ifstream file(MainSpace::EXE_PATH + file_path);
	if (file) {
		std::ostringstream c;
		c<< file.rdbuf();
		file.close();
		return c.str();
	} else {
		SDL_Log("Unable to load file: %s\n", file_path.c_str());
	}
	return "";
}

std::vector<std::string> CREngine::Utils::file_to_lines(const std::string &file_path) {
	std::ifstream file;
	file.open(MainSpace::EXE_PATH + file_path);
	std::vector<std::string> lines;
	std::string line;
	if (file.is_open()) {
		while (file.good()) {
			getline(file, line);
			lines.push_back(line);
		}
	} else {
		SDL_Log("Unable to load file: %s\n", file_path.c_str());
	}
	return lines;
}

std::vector<std::string> CREngine::Utils::split_string(const std::string &str, char split) {
	std::vector<std::string> parts;
	std::string part = "";
	for (char c: str) {
		if (c == split) {
			if (part.length() > 0) {
				parts.push_back(part);
			}
			part = "";
		} else {
			part += c;
		}
	}
	if (part.length() > 0) {
		parts.push_back(part);
	}
	
	return parts;
}

//Nameable
Nameable::Nameable(const std::string &name) : name(name) {
	if (name.compare("") == 0) this->name = "Nameable_" + std::to_string(nameable_cout++);
}

std::string Nameable::get_name() const {
	return name;
}
