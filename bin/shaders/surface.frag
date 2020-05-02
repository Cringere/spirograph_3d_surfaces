#version 330 core
out vec4 frag_color;

in vec3 _position;
in vec3 _normal;
in vec3 _color;

void main() {
	vec3 directional_lights[4] = vec3[4](vec3(1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, -1.0));
	float lighting_mult = 0.0f;

	for (int i = 0; i < directional_lights.length(); ++i)
		directional_lights[i] = normalize(directional_lights[i]);

	for (int i = 0; i < directional_lights.length(); ++i)
		lighting_mult += max(dot(directional_lights[i], _normal), 0.0f) * 0.7f;

	frag_color = vec4(_color * lighting_mult, 1.0);
}