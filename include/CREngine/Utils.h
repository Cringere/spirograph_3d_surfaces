#ifndef CRENGINE_PUBLIC_HEADER_UTILS
#define CRENGINE_PUBLIC_HEADER_UTILS

#include <string>
#include <vector>

namespace CREngine {
	namespace Utils {
		std::string file_to_string(const std::string &file_path);
		std::vector<std::string> file_to_lines(const std::string &file_path);
		std::vector<std::string> split_string(const std::string &str, char split);

		class Nameable {
			private:
				std::string name;
				int id;
			public:
				Nameable(const std::string &name);

				std::string get_name() const;
		};
	}
}

#endif