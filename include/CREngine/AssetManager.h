#ifndef CRENGINE_PUBLIC_HEADER_ASSET_MANAGER
#define CRENGINE_PUBLIC_HEADER_ASSET_MANAGER

#include <string>

#include <CREngine/RenderUtils.h>

namespace CREngine {
	namespace AssetManager {
		void add(RenderUtils::Texture *texture);
		void add(RenderUtils::Font *font);
		void add(RenderUtils::Shader *shader);

		RenderUtils::Texture *get_texture(const std::string &name);
		RenderUtils::Font *get_font(const std::string &name);
		RenderUtils::Shader *get_shader(const std::string &name);
	
		void init();

		void dispose();
	}
}

#endif
