#include <GL/glew.h>

#include <iostream>

#include <CREngine/MainSpace.h>
#include <CREngine/AssetManager.h>
#include <CREngine/RenderUtils.h>
#include <CREngine/GUI.h>
#include <functional>
#include <CREngine/InputManager.h>

using namespace CREngine;

static RenderUtils::Batcher b("spiro_batcher"), h("handle_batcher");
static RenderUtils::Shader *shader;

static std::vector<std::vector<float>> spiro_structure;	//length, angular frequency
static std::vector<Math::Matrix2D> spiro_rotation_matrices;
static std::vector<Math::Vector2D> spiro_handles;

static std::vector<float> spiro_points;
static float total_time;

static void spiro_step() {
	using namespace CREngine::Math;

	//Vector2D head;
	//for (int i = 0; i < spiro_structure.size(); ++i) {
	//	head[0] += spiro_structure[i][0];
	//}

	int m = spiro_structure.size();
	for (int i = m - 1; i >= 0; --i) {
		Matrix2D &rotation_matrix = spiro_rotation_matrices[i];
		for (int j = i; j < m; ++j) {
			spiro_handles[j] = rotation_matrix * spiro_handles[j];
		}

		//origin[0] -= spiro_structure[i][0];

		/*head -= origin;
		head = rotation_matrix * head;
		head += origin;*/
	}

	Vector2D head;
	for (int i = 0; i < m; ++i)
		head += spiro_handles[i];

	spiro_points.push_back(head[0]);
	spiro_points.push_back(head[1]);
	spiro_points.push_back(0.0f);
	spiro_points.push_back(0.0f);
	spiro_points.push_back(0.0f);
}

void init() {
	b.init(10000, std::vector<int> {2, 3});
	b.mode = GL_LINE_STRIP;

	h.init(10000, std::vector<int> {2, 3});
	h.mode = GL_LINE_STRIP;

	shader = AssetManager::get_shader("spiro_2d");

	spiro_structure.emplace_back(std::vector<float> {0.2f, 2.0f});
	spiro_structure.emplace_back(std::vector<float> {0.1f, 1.0f});
	spiro_structure.emplace_back(std::vector<float> {0.1f, -3.5f});

	for (int i = 0; i < spiro_structure.size(); ++i) {
		spiro_handles.emplace_back(Math::Vector2D(spiro_structure[i][0], 0.0f));
		spiro_rotation_matrices.push_back(Math::Matrix2D::rotation(spiro_structure[i][1] * 0.01f));
	}
}


void handle_input() {}

void update(float dt) {
	total_time += dt;

	spiro_step();

	h.clear();
	Math::Vector2D trace;
	std::vector<float> data = {trace[0], trace[1], 0.0f, 0.0f, 0.0f};
	h.add_data(&data[0], data.size());
	for (int i = 0; i < spiro_structure.size(); ++i) {
		trace += spiro_handles[i];
		std::vector<float> data = {trace[0], trace[1], 0.0f, 0.0f, 0.0f};
		h.add_data(&data[0], data.size());
	}
	h.update();

	b.clear();
	b.add_data(&spiro_points[0], spiro_points.size());
	b.update();
}

void render() {
	glClearColor(0.8, 0.8, 0.8, 1.0);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_COLOR_BUFFER_BIT);

	shader->use();
	shader->bind_vec("camera_position",Math::Vector2D(0.0f, 0.0f));
	shader->bind_vec("camera_size",Math::Vector2D(1.0f, 1.0f));

	b.render();
	h.render();
}

void dispose() {

}

int main(int argc, char const *argv[]) {
	CREngine::MainSpace::set_main_function_pointers(
		&init,
		&handle_input,
		&update,
		&render,
		&dispose
	);

	CREngine::MainSpace::Configuration conf("spiro_2d", 800, 800, 60);

	if (CREngine::MainSpace::run(conf) != 0) return -1;

	return 0;
}
