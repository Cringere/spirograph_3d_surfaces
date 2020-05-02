#include <CREngine/RenderUtils.h>

#include <CREngine/MainSpace.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fstream>
#include <iostream>

//utils functions and variables
static std::string read_file(const std::string &filePath) {
	std::ifstream file;
	file.open(CREngine::MainSpace::EXE_PATH + filePath);
	std::string output;
	std::string line;
	if (file.is_open()) {
		while (file.good()) {
			getline(file, line);
			output.append(line + "\n");
		}
	} else {
		SDL_Log("Unable to load file: %s\n", filePath.c_str());
	}
	return output;
}

static std::vector<std::string> split_string(const std::string &text, const char split) {
	size_t start = 0, end = 0;
	const char *t = text.c_str();
	size_t len = text.length();
	
	std::vector<std::string> vec;
	
	while(t[start] == split && start < len) start++;
	end = start;
	
	while(end < len) {
		if(t[end] == split){
			vec.push_back(text.substr(start, end - start));
			while(t[end] == split){
				if(end == len - 1) return vec;
				end++;
			}
			start = end;
		}
		end++;
	}
	
	vec.push_back(text.substr(start, end - start));
	
	return vec;
}

using namespace CREngine::RenderUtils;

//Texture			
Texture::Texture(const std::string &name) : Nameable(name), texture_id(0), size{0, 0} {

}

Texture::Texture(const std::string &name, const std::string &path) : Texture(name) {
	init(path);
}

Texture::~Texture() {
	glDeleteTextures(1, &texture_id);
}

void Texture::init(const std::string &path) {
	SDL_Surface *texture_data = IMG_Load((CREngine::MainSpace::EXE_PATH + path).c_str());
	if (!texture_data) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not load texture: %s", path.c_str());
		return;
	}
	
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);//GL_LINEAR
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//GL_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int mode = GL_RGB;
	if (texture_data->format->BytesPerPixel == 4) mode = GL_RGBA;
	
	size[0] = texture_data->w;
	size[1] = texture_data->h;

	glTexImage2D(GL_TEXTURE_2D, 0, mode, texture_data->w, texture_data->h, 0, 
		mode, GL_UNSIGNED_BYTE, texture_data->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	
	SDL_FreeSurface(texture_data);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::use_as_0() const {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);
}

void Texture::use(unsigned int texture_number) const {
	glEnable(GL_TEXTURE_2D);
	switch (texture_number) {
		case 0: glActiveTexture(GL_TEXTURE0); break;
		case 1: glActiveTexture(GL_TEXTURE1); break;
		case 2: glActiveTexture(GL_TEXTURE2); break;
		case 3: glActiveTexture(GL_TEXTURE3); break;
		case 4: glActiveTexture(GL_TEXTURE4); break;
		case 5: glActiveTexture(GL_TEXTURE5); break;
	}

	glBindTexture(GL_TEXTURE_2D, texture_id);
}

int Texture::get_width() {
	return size[0];
}

int Texture::get_height() {
	return size[1];
}



//Font
Font::Font(const std::string &name) : Texture(name) {

}

Font::Font(const std::string &name, const std::string &path) : Font(name, path, 64) {

}

Font::Font(const std::string &name, const std::string &path, int generated_size) : Font(name) {
	init(path, generated_size);
}

void Font::init(const std::string &path) {
	init(path, 64);
}

void Font::init(const std::string &path, int generated_size) {
	FT_Face face;
	if (FT_New_Face(CREngine::MainSpace::ft, std::string(MainSpace::EXE_PATH + path).c_str(), 0, &face)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load font: %s", path.c_str());
	}

	FT_Set_Pixel_Sizes(face, 0, generated_size);

	int margin_x = 10;
	//int margin_y = 10;

	int w = 0, h = 0, max_h;
	int max_w = 0;
	FT_GlyphSlot g = face->glyph;
	for(int i = 32; i < 128; i++) {
		if(FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			fprintf(stderr, "Loading character %c failed!\n", i);
			continue;
		}

		w += g->bitmap.width;
		w += margin_x;

		if (max_w < g->bitmap.width) max_w = g->bitmap.width;
		if (h < g->bitmap.rows) h = g->bitmap.rows;
	}
	max_h = h;

	max_w += margin_x;
	//h += margin_y * 2;

	float tex_w = w;
	float tex_h = h;

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	//fill the texture with transparent pixels
	unsigned char *data = new unsigned char[w * h * 4];
	for (int i = 0; i < w * h * 4; ++i) data[i] = 0;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	delete[] data;
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	unsigned char pixel_data[max_w * max_h * 4];

	int x = 0;
	for(int i = 32; i < 128; i++) {
		if(FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			fprintf(stderr, "Loading character %c failed!\n", i);
			continue;
		}
		
		//glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, g->bitmap.width, g->bitmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap.buffer);

		int w = g->bitmap.width;
		int h = g->bitmap.rows;
		
		//for (int i = 0; i < max_w * max_h * 4; ++i) pixel_data[i] = 0;

		//load the glyph and 
		for (int i = 0; i < w; ++i) {
			for (int j = h - 1; j >= 0; --j) {
				unsigned char c = g->bitmap.buffer[i * h + j];
				pixel_data[i * h * 4 + j * 4 + 0] = 255;
				pixel_data[i * h * 4 + j * 4 + 1] = 255;
				pixel_data[i * h * 4 + j * 4 + 2] = 255;
				pixel_data[i * h * 4 + j * 4 + 3] = c;
			}
		}
		
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);

		glyphs[i].uv_pos = Math::Vector2D(x / tex_w, 0.0f / tex_h);
		glyphs[i].uv_size = Math::Vector2D(w / tex_w, h / tex_h);

		float font_size = generated_size;

		float g_w = face->glyph->bitmap.width;
		float g_h = face->glyph->bitmap.rows;
		//float temp = ;
		//std::cout<<(face->glyph->bitmap.width)<<std::endl;
		//std::cout<<(face->bbox.xMax - face->bbox.xMin) / 64<<std::endl;
		//std::cout<<(face->bbox.xMin) / 64<<std::endl;
		//float g_w = (face->bbox.xMax - face->bbox.xMin) / 72.0f;
		//float g_h = (face->bbox.yMax - face->bbox.yMin) / 72.0f;
		//std::cout<<(face->glyph->bitmap.width)<<std::endl;
		//std::cout<<(g_w)<<std::endl;

		float bearing_x = face->glyph->bitmap_left;
		float bearing_y = face->glyph->bitmap_top;

		glyphs[i].size = Math::Vector2D(g_w / font_size, g_h / font_size);
		glyphs[i].offset = Math::Vector2D(bearing_x / font_size, (bearing_y - g_h) / font_size);
		glyphs[i].advance = face->glyph->advance.x / 64.0f /  generated_size;

		if (glyphs[i].offset[1] < vertical_bounds[0]) {
			vertical_bounds[0] = glyphs[i].offset[1];
		}

		if (glyphs[i].offset[0] + glyphs[i].size[0] > vertical_bounds[1]) {
			vertical_bounds[1] = glyphs[i].offset[0] + glyphs[i].size[0];
		}
		
		x += margin_x;
		x += g->bitmap.width;
	}

	//scale
	//std::cout<<(vertical_bounds[1] - vertical_bounds[0])<<std::endl;
	float s = 1.0f / (vertical_bounds[1] - vertical_bounds[0]);
	for(int i = 32; i < 128; i++) {
		glyphs[i].size = glyphs[i].size * s;
		glyphs[i].offset = glyphs[i].offset * s;
		glyphs[i].advance = glyphs[i].advance * s;
	}


	glBindTexture(GL_TEXTURE_2D, 0);

	FT_Done_Face(face);
}

const Font::Glyph *Font::get_glyph(char c) const {
	return &glyphs[(int) c];
}

//Batcher
Batcher::Batcher() : Batcher("") {

}

Batcher::Batcher(const std::string &name) : Nameable(name), max_size(0), current_size(0), VAO(0), VBO(0), mode(GL_TRIANGLES) {}

Batcher::~Batcher() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

void Batcher::init(unsigned int max_number_of_elements, const std::vector<int> &layouts_sizes) {
	layouts_total_size = 0;
	for (int s : layouts_sizes) layouts_total_size += s;

	max_size = layouts_total_size * max_number_of_elements * sizeof(float);
	buffer = std::shared_ptr<float>(new float[max_size]);
	
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, max_size, nullptr, GL_DYNAMIC_DRAW);

	int previous = 0;
	for (int i = 0; i < layouts_sizes.size(); ++i) {
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, layouts_sizes[i], GL_FLOAT, GL_FALSE, layouts_total_size * sizeof(float), (void*) (previous * sizeof(float)));
		previous += layouts_sizes[i];
		
	}
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Batcher::add_data(float *data, unsigned int data_length) {
	std::copy(data, data + data_length, buffer.get() + current_size);
	current_size += data_length;
}

void Batcher::update() {
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
	glBufferData(GL_ARRAY_BUFFER, current_size * sizeof(float), buffer.get(), GL_DYNAMIC_DRAW);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Batcher::clear() {
	current_size = 0;
}

void Batcher::render() const {
	glBindVertexArray(VAO);
	glDrawArrays(mode, 0, current_size / layouts_total_size);
	glBindVertexArray(0);
}

//Shader
Shader::Shader(const std::string &name) : Nameable(name), programID(0) {}

Shader::Shader(const std::string &name, const std::string &vertex_shader, const std::string &fragment_shader, bool hardcoded) : Shader(name) {
	if (hardcoded) init_from_text(vertex_shader, fragment_shader);
	else init_from_file(vertex_shader, fragment_shader);
}

Shader::~Shader() {
	if (programID != 0)
		glDeleteProgram(programID);
}

void Shader::init_from_text(const std::string &vertex_shader, const std::string &fragment_shader) {
	const char *vsSource = vertex_shader.c_str(), *fsSource = fragment_shader.c_str();

	GLuint vertexShader;
	GLuint fragmentShader;
	int errorVert = 0;
	int errorFrag = 0;
	int errorProgram = 0;
	char infoLog[512];

	//vertex shader
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vsSource, nullptr);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &errorVert);
	if (!errorVert) {
		glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vertex Shader compilation failed: %s \n", infoLog);
	}

	//fragment shader
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fsSource, nullptr);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &errorFrag);
	if (!errorFrag) {
		glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Fragment Shader compilation failed: %s \n", infoLog);
	}

	//shader program
	programID = glCreateProgram();
	glAttachShader(programID, vertexShader);
	glAttachShader(programID, fragmentShader);
	glLinkProgram(programID);
	glGetProgramiv(programID, GL_LINK_STATUS, &errorProgram);
	if (!errorProgram) {
		glGetProgramInfoLog(programID, 512, nullptr, infoLog);
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Shader program linking failed: %s \n", infoLog);
	}

	//clean up
	glDetachShader(programID, vertexShader);
	glDetachShader(programID, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//read uniforms names
	std::vector<std::string> total_lines = split_string(vertex_shader + fragment_shader, '\n');
	for (std::string &line : total_lines) {
		std::vector<std::string> words = split_string(line, ' ');
		if (words[0].compare("uniform") == 0) {
			int l = words[0].length() + words[1].length() + 2;
			std::string ln = line.substr(l, line.length() - l);
			std::string name = "";
			for (char c: ln) {
				if (c == ' ') continue;
				if (c == ',' || c == ';') {
					uniforms[name]= glGetUniformLocation(programID, name.c_str());
					name = "";
				} else {
					name += c;
				}
			}
		}
	}
}

GLint Shader::get_uniform_location(const std::string &name) {
	std::map<std::string, GLint>::iterator it = uniforms.find(name);
	if (it == uniforms.end()) return -1;
	return it->second;
}

void Shader::init_from_file(const std::string &vertex_shader_path, const std::string &fragment_shader_path) {
	init_from_text(read_file(vertex_shader_path), read_file(fragment_shader_path));
}

void Shader::bind_vec(const std::string &name, const Math::Vector2D &v) {
	glUniform2f(get_uniform_location(name), v[0], v[1]);
}

void Shader::bind_vec(const std::string &name, const Math::Vector3D &v) {
	glUniform3f(get_uniform_location(name), v[0], v[1], v[2]);
}

void Shader::bind_mat(const std::string &name, const Math::Matrix2D &mat) {
	glUniformMatrix2fv(get_uniform_location(name), 1, GL_FALSE, &mat.rows[0].v[0]);
}

//void Shader::bind_mat(const std::string &name, const Math::Matrix3D &mat) {
//	glUniformMatrix3fv(get_uniform_location(name), 1, GL_FALSE, &mat.rows[0].v[0]);
//}

void Shader::bind_mat(const std::string &name, const Math::Matrix4D &mat) {
	//float data[16];
	float data[4][4];
	for (int i = 0, count = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			//data[count++] = mat[i][j];
			data[i][j] = mat[i][j];
	
	//glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, &mat.rows[0].v[0]);
	glUniformMatrix4fv(get_uniform_location(name), 1, GL_TRUE, &data[0][0]);
}


void Shader::use() {
	glUseProgram(programID);
}
