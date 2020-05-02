#include <GL/glew.h>

#include <iostream>
#include <tuple>

#include <CREngine/MainSpace.h>
#include <CREngine/AssetManager.h>
#include <CREngine/RenderUtils.h>
#include <CREngine/GUI.h>
#include <functional>
#include <CREngine/InputManager.h>

using namespace CREngine;

class Camera3D {
public:
	float n, f;						//near and far
	Math::Vector2D size;			//width and height
	Math::Vector3D position;		//location
	Math::Vector3D up, direction;	//orientation

	//calculatable objects
	Math::Vector3D right;
	Math::Matrix4D view, projection;	//the matrices that will go to the shader

	Camera3D() : n(0.1f), f(20.0f), size(3.0f, 3.0f), position(0.0f, 0.0f, 5.0f), up(0.0f, 1.0f, 0.0f), direction(0.0f, 0.0f, -1.0f) {}

	void look_at(const Math::Vector3D &target) {
		direction = (target - position).normalize();
	}

	void look_at(const Math::Vector3D &target, float distance) {
		direction = (target - position).normalize();
		position = target - direction * distance;
	}

	void update() {
		direction = direction.normalize();

		right = direction.cross(up).normalize();

		up = right.cross(direction).normalize();

		view = Math::Matrix4D::view(right, up, direction) * Math::Matrix4D::translate(position);
		projection = Math::Matrix4D::orthographic_projection(size, n, f);
	}
};

//camera and input
static Camera3D camera;
static Math::Vector2D mouse_p_frame, mouse_frame;
static Math::Vector2D camera_v;

//resources
static RenderUtils::Batcher b("spiro_batcher"), h("handle_batcher"), g("grid");
static RenderUtils::Shader *shader;

//spirograph parts
static std::vector<std::tuple<Math::Vector3D, float>> spiro_structure;	//axis (magnitude = anglular frequency), length
static std::vector<Math::Matrix3D> spiro_rotation_matrices;
static std::vector<Math::Vector3D> spiro_handles;


//spirograph trace
static std::vector<float> spiro_points;

//program runtime
static float total_time;


static Math::Matrix3D rotation_matrix(Math::Vector3D axis) {

}

static void spiro_step() {
	using namespace CREngine::Math;

	spiro_handles.clear();
	Vector3D origin;
	for (int i = 0; i < spiro_structure.size(); ++i) {
		spiro_handles.emplace_back(std::get<1>(spiro_structure[i]), 0.0f, 0.0f);
		origin += spiro_handles.back();
	}

	int m = spiro_structure.size();
	for (int i = m - 1; i >= 0; --i) {
		Matrix3D rotation_matrix = Math::Matrix3D::rotation(std::get<0>(spiro_structure[i]) * total_time);
		for (int j = i; j < m; ++j) {
			spiro_handles[j] = rotation_matrix * spiro_handles[j];
		}
	}

	Vector3D head;
	for (int i = 0; i < m; ++i)
		head += spiro_handles[i];

	spiro_points.push_back(head[0]);
	spiro_points.push_back(head[1]);
	spiro_points.push_back(head[2]);
	spiro_points.push_back(0.0f);
	spiro_points.push_back(0.0f);
	spiro_points.push_back(0.0f);
}

void init() {
	//create batchers
	b.init(100000, std::vector<int> {3, 3});
	b.mode = GL_LINE_STRIP;
	h.init(200, std::vector<int> {3, 3});
	h.mode = GL_LINE_STRIP;
	g.init(400, std::vector<int> {3, 3});
	g.mode = GL_LINES;

	//create the grid
	float l = 2.0f, dl = 0.5f;
	g.clear();
	{
		std::vector<float> data = {	//axis
			//x
			-l, 0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
			l, 0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
			//y
			0.0f, -l, 0.0f,		0.0f, 0.0f, 0.0f,
			0.0f, l, 0.0f,		0.0f, 0.0f, 0.0f,
			//y
			0.0f, 0.0f, -l,		0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, l,		0.0f, 0.0f, 0.0f,
		};
		g.add_data(&data[0], data.size());
	}
	float c_r = 0.4f, c_g = 0.4f, c_b = 0.4f;
	for (float i = -l; i <= l; i += dl) {
		//along x
		std::vector<float> data = {	//axis
			//x
			-l, 0.0f, i,		c_r, c_g, c_b,
			l, 0.0f, i,			c_r, c_g, c_b,
			//z
			i, 0.0f, -l,		c_r, c_g, c_b,
			i, 0.0f, l,			c_r, c_g, c_b
		};
		g.add_data(&data[0], data.size());
	}
	g.update();

	//load shader
	shader = AssetManager::get_shader("spiro_3d");

	//define the spirograph
	//spiro_structure.emplace_back(std::vector<float> {0.8f, 2.0f});
	//spiro_structure.emplace_back(std::vector<float> {0.2f, 1.0f});
	//spiro_structure.emplace_back(std::vector<float> {0.2f, -3.5f});

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.013f, 0.0f), 0.9f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f), 0.5f});

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.1f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});
	
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.1f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.1f, 1.0f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f).normalize() * 0.11f, 1.0f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f).normalize(), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 2.0f, 1.0f).normalize(), 0.5f});
	
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 1.0f, 1.0f).normalize() * 0.013f, 1.0f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f).normalize(), 0.5f});

	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f).normalize(), 1.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 2.0f).normalize(), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 3.0f).normalize() * sqrt(2), 0.5f});

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f).normalize(), 1.0f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(2.0f, 0.0f, 2.0f).normalize() * sqrt(3), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 3.0f).normalize() * sqrt(2), 0.5f});

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 0.0f), 0.5f});

	//create the spirograph's parts
	/*
	for (int i = 0; i < spiro_structure.size(); ++i) {
		spiro_handles.emplace_back(Math::Vector2D(spiro_structure[i][0], 0.0f));
		spiro_rotation_matrices.push_back(Math::Matrix2D::rotation(spiro_structure[i][1] * 0.01f));
	}
	*/
}


void handle_input() {
	mouse_p_frame = mouse_frame;
	mouse_frame = InputManager::mouse_position;
}

void update(float dt) {


	/*h.clear();
	Math::Vector2D trace;
	std::vector<float> data = {trace[0], trace[1], 0.0f, 0.0f, 0.0f};
	h.add_data(&data[0], data.size());
	for (int i = 0; i < spiro_structure.size(); ++i) {
		trace += spiro_handles[i];
		std::vector<float> data = {trace[0], trace[1], 0.0f, 0.0f, 0.0f};
		h.add_data(&data[0], data.size());
	}
	h.update();*/

	//update the spirograph
	int n = 10;
	float s = 10.0f;
	for (int i = 0; i < n; ++i) {
		total_time += s * dt;
		spiro_step();
	}
	

	//update the spirograph's visual data
	b.clear();
	b.add_data(&spiro_points[0], spiro_points.size());
	b.update();




	//update the handles
	h.clear();
	Math::Vector3D trace;
	std::vector<float> data = {trace[0], trace[1], trace[2], 0.0f, 0.0f, 0.0f};
	h.add_data(&data[0], data.size());
	for (int i = 0; i < spiro_structure.size(); ++i) {
		trace += spiro_handles[i];
		std::vector<float> data = {trace[0], trace[1], trace[2], 0.0f, 0.0f, 0.0f};
		h.add_data(&data[0], data.size());
	}
	h.update();

	
	using namespace InputManager;
	/*
	//keybaord movement
	float r = 0.0f, u = 0.0f;
	if (keys[KEYS::KEY_D] == KEY_STATE::DOWN) {
		r += 1.0f;
	}
	if (keys[KEYS::KEY_A] == KEY_STATE::DOWN) {
		r -= 1.0f;
	}
	if (keys[KEYS::KEY_W] == KEY_STATE::DOWN) {
		u += 1.0f;
	}
	if (keys[KEYS::KEY_S] == KEY_STATE::DOWN) {
		u -= 1.0f;
	}
	float s = 5.0f;
	camera.position += camera.right * dt * s * r;
	camera.position += camera.up * dt * s * u;*/

	//mouse_movement
	if (InputManager::mouse_keys[0] == InputManager::KEY_STATE::DOWN) {
		Math::Vector2D s(MainSpace::get_width() / 3.141f, MainSpace::get_height() / 3.141f);
		Math::Vector2D d = (mouse_frame - mouse_p_frame);
		camera_v += d * s * dt * 2.0f;
	}

	camera.position -= camera.right * camera_v[0];
	camera.position -= camera.up * camera_v[1];
	camera_v *= 0.7f;

	camera.look_at(Math::Vector3D(0.0f, 0.0f, 0.0f), 3.0f);
	camera.update();
}

void render() {
	glClearColor(0.8, 0.8, 0.8, 1.0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->use();
	shader->bind_mat("camera_view", camera.view);
	//shader->bind_mat("camera_view", Math::Matrix4D::identity());
	
	shader->bind_mat("camera_projection", camera.projection);
	//shader->bind_mat("camera_projection", Math::Matrix4D::identity());
	
	b.render();
	h.render();
	g.render();
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
