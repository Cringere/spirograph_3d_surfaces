#version 330 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;

out vec3 out_position;
out vec3 out_color;

uniform mat4 camera_view;		//camera position and orientation
uniform mat4 camera_projection;	//frustum to opengl space

void main() {
	gl_PointSize = 7.0;
	out_color = in_color;
	out_position = (camera_view * vec4(in_position, 1.0)).xyz;
	gl_Position = camera_projection * camera_view * vec4(in_position, 1.0);
}