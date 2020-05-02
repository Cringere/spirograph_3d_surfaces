#version 330 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;

out vec3 _position;
out vec3 _normal;
out vec3 _color;

uniform mat4 camera_view;		//camera position and orientation
uniform mat4 camera_projection;	//frustum to opengl space

void main() {
	gl_PointSize = 7.0;
	_color = in_color;
	_normal = in_normal;
	_position = (camera_view * vec4(in_position, 1.0)).xyz;
	gl_Position = camera_projection * camera_view * vec4(in_position, 1.0);
}
