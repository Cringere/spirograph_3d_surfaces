#include <CREngine/AssetManager.h>

#include <vector>

using namespace CREngine;

static std::vector<RenderUtils::Texture *> textures;
static std::vector<RenderUtils::Font *> fonts;
static std::vector<RenderUtils::Shader *> shaders;

void AssetManager::add(RenderUtils::Texture *texture) {
	textures.push_back(texture);
}

void AssetManager::add(RenderUtils::Font *font) {
	fonts.push_back(font);
}

void AssetManager::add(RenderUtils::Shader *shader) {
	shaders.push_back(shader);
}


RenderUtils::Texture *AssetManager::get_texture(const std::string &name) {
	for (RenderUtils::Texture *s: textures)
		if (s->get_name().compare(name) == 0)
			return s;
	return nullptr;
}

RenderUtils::Font *AssetManager::get_font(const std::string &name) {
	for (RenderUtils::Font *s: fonts)
		if (s->get_name().compare(name) == 0)
			return s;
	return nullptr;
}

RenderUtils::Shader *AssetManager::get_shader(const std::string &name) {
	for (RenderUtils::Shader *s: shaders)
		if (s->get_name().compare(name) == 0)
			return s;
	return nullptr;
}

void AssetManager::init() {
	AssetManager::add(new RenderUtils::Shader("line_shader", "shaders/line.vert", "shaders/line.frag"));
	AssetManager::add(new RenderUtils::Shader("surface_shader", "shaders/surface.vert", "shaders/surface.frag"));
}

void AssetManager::dispose() {
	for (RenderUtils::Texture *texture : textures) delete texture;
	for (RenderUtils::Font *font : fonts) delete font;
	for (RenderUtils::Shader *shader: shaders) delete shader;
}

