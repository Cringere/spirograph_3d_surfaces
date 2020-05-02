#ifndef CRENGINE_PUBLIC_HEADER_RENDER_UTILS
#define CRENGINE_PUBLIC_HEADER_RENDER_UTILS

#include <CREngine/Math.h>
#include <CREngine/Utils.h>

#include <GL/glew.h>

#include <map>
#include <vector>
#include <string>
#include <memory>

namespace CREngine {
	namespace RenderUtils {
		class Texture : public Utils::Nameable {
			friend class Shader;
			friend class Font;
			private:
				int size[2];
				GLuint texture_id;
			
			public:
				Texture(const std::string &name);

				Texture(const std::string &name, const std::string &path);
				
				~Texture();
				
				virtual void init(const std::string &path);

				void use_as_0() const;
				void use(unsigned int texture_number) const;

				int get_width();
				int get_height();
		};

		class Font : public Texture {
		public:		
			struct Glyph {
				Math::Vector2D uv_pos, uv_size;
				Math::Vector2D size;
				Math::Vector2D offset;
				float advance;
			};
			std::array<Glyph, 128> glyphs;
			Math::Vector2D vertical_bounds;

			Font(const std::string &name);

			Font(const std::string &name, const std::string &path);
			
			Font(const std::string &name, const std::string &path, int generated_size);

			void init(const std::string &path);
			
			void init(const std::string &path, int generated_size);

			const Glyph *get_glyph(char c) const;
		};

		class Batcher : public Utils::Nameable {
			protected:
				unsigned int max_size, current_size;	//in bytes
				unsigned int layouts_total_size;
				std::shared_ptr<float> buffer;
				GLuint VAO, VBO;
				
			public:
				GLenum mode;
				
				Batcher();

				Batcher(const std::string &name);
				
				~Batcher();

				void init(unsigned int max_number_of_elements, const std::vector<int> &layouts_sizes);
				
				void add_data(float *data, unsigned int data_length);

				void update();

				void clear();

				virtual void render() const;
		};

		class Shader : public Utils::Nameable {
			private:
				GLuint programID;
				std::map<std::string, GLint> uniforms;

			public:
				Shader(const std::string &name);
				Shader(const std::string &name, const std::string &vertex_shader, const std::string &fragment_shader, bool hardcoded = false);
				
				~Shader();

				void init_from_text(const std::string &vertex_shader, const std::string &fragment_shader);
				void init_from_file(const std::string &vertex_shader_path, const std::string &fragment_shader_path);
				
				GLint get_uniform_location(const std::string &name);

				void bind_vec(const std::string &name, const Math::Vector2D &v);
				void bind_vec(const std::string &name, const Math::Vector3D &v);
				
				void bind_mat(const std::string &name, const Math::Matrix2D &mat);
				void bind_mat(const std::string &name, const Math::Matrix3D &mat);
				void bind_mat(const std::string &name, const Math::Matrix4D &mat);

				void use();
		};
	}
}

#endif
