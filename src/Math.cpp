#include <CREngine/Math.h>

#include <iostream>

using namespace CREngine::Math;

//Vector2D
Vector2D::Vector2D() : v{0, 0} {}

Vector2D::Vector2D(float x, float y) : v{x, y} {}

Vector2D::Vector2D(const Vector2D &vec) : v{vec.v[0], vec.v[1]} {}

Vector2D Vector2D::operator=(const Vector2D &vec) {
	if (&vec != this) {
		v[0] = vec.v[0];
		v[1] = vec.v[1];
	}
	return *this;
}

float Vector2D::get_angle() {
	/*
	using the dot product: let u = <1, 0>
		v * u = |v| |u| cos(t)
		t = arccos(v * u / (|v| |u|)) = arccos(v.x / |v|)
		t = -t if v.x < 0
	*/
	float t = acos(v[0] / length());
	if (v[0] < 0)
		return -t;
	return t;
}

std::string Vector2D::to_string() {
	return "<" + std::to_string(v[0]) + ", " + std::to_string(v[1]) + ">";
}

void Vector2D::print() {
	std::cout<<to_string()<<std::endl;
}

//Vector3D
Vector3D::Vector3D() : v{0, 0, 0} {};

Vector3D::Vector3D(float x, float y, float z) : v{x, y, z} {};

Vector3D::Vector3D(const Vector3D &vec) : v{vec.v[0], vec.v[1], vec.v[2]} {}

Vector3D Vector3D::operator=(const Vector3D &vec) {
	if (&vec != this) {
		v[0] = vec.v[0];
		v[1] = vec.v[1];
		v[2] = vec.v[2];
	}
	return *this;
}

std::string Vector3D::to_string() {
	return "<" + std::to_string(v[0]) + ", " + std::to_string(v[1]) + ", " + std::to_string(v[2]) + ">";
}

void Vector3D::print() {
	std::cout<<to_string()<<std::endl;
}

//Vector4D
Vector4D::Vector4D() : Vector4D(0.0f, 0.0f, 0.0f, 0.0f) {}

Vector4D::Vector4D(float x, float y, float z, float w) : v{x, y, z, w} {}

Vector4D::Vector4D(const Vector4D &vec) : v{vec.v[0], vec.v[1], vec.v[2], vec.v[3]} {}

Vector4D Vector4D::operator=(const Vector4D &vec) {
	if (&vec != this) {
		v[0] = vec.v[0];
		v[1] = vec.v[1];
		v[2] = vec.v[2];
		v[3] = vec.v[3];
	}
	return *this;
}

std::string Vector4D::to_string() {
	return "<" + std::to_string(v[0]) + ", " + std::to_string(v[1]) + ", " + std::to_string(v[2]) + ", " + std::to_string(v[3]) + ">";
}

void Vector4D::print() {
	std::cout<<to_string()<<std::endl;
}

//Matrix2D
Matrix2D::Matrix2D(float a, float b, float c, float d) : rows{Vector2D(a, b), Vector2D(c, d)} {}

Matrix2D::Matrix2D(const Vector2D &row_a, const Vector2D &row_b) : rows{row_a, row_b} {}

//matrix - vector operations
Vector2D Matrix2D::operator*(const Vector2D &vec) {
	return Vector2D(rows[0].dot(vec), rows[1].dot(vec));
}

//static creators
Matrix2D Matrix2D::identity() {
	return Matrix2D(
		1.0f, 0.0f,
		0.0f, 1.0f
	);
}
Matrix2D Matrix2D::rotation(float theta) {
	float c = cos(theta);
	float s = sin(theta);
	return Matrix2D(
		c, -s,
		s, c
	);
}


//Matrix3D
Matrix3D::Matrix3D() {

}

Matrix3D::Matrix3D(const Vector3D &row_a, const Vector3D &row_b, const Vector3D &row_c) : rows{row_a, row_b, row_c} {

}

//matrix - vector operations
Matrix3D Matrix3D::operator*(float s) const {
	return Matrix3D(rows[0] * s, rows[1] * s, rows[2] * s);
	return Matrix3D(rows[0] * s, rows[1] * s, rows[2] * s);
}

Vector3D Matrix3D::operator*(const Vector3D &vec) const {
	return Vector3D(rows[0].dot(vec), rows[1].dot(vec), rows[2].dot(vec));
}

Matrix3D Matrix3D::operator*(const Matrix3D &mat) const {
	Matrix3D out;
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			float sum = 0.0f;
			for (int k = 0; k < 3; ++k) {
				sum += rows[i][k] * mat.rows[k][j];
			}
			out[i][j] = sum;
		}
	}
	return out;
}

Matrix3D Matrix3D::operator+(const Matrix3D &mat) const {
	return Matrix3D(rows[0] + mat.rows[0], rows[1] + mat.rows[1], rows[2] + mat.rows[2]);
}

//static creators
Matrix3D Matrix3D::identity() {
	return Matrix3D(
		Vector3D(1.0f, 0.0f, 0.0f),
		Vector3D(0.0f, 1.0f, 0.0f),
		Vector3D(0.0f, 0.0f, 1.0f)
	);
}
Matrix3D Matrix3D::rotation(const Vector3D &axis) {
	Vector3D k = axis.normalize();
	Matrix3D K(
		Vector3D(0.0f, -k[2], k[1]),
		Vector3D(k[2], 0.0f, -k[0]),
		Vector3D(-k[1], k[0], 0.0f)
	);
	float theta = axis.length();
	return Matrix3D::identity() + (K * sin(theta)) + ((K * K) * (1 - cos(theta)));
}


//Matrix4D
Matrix4D::Matrix4D() {}

Matrix4D::Matrix4D(const Vector4D &row_a, const Vector4D &row_b, const Vector4D &row_c, const Vector4D &row_d) : rows{row_a, row_b, row_c, row_d} {}

Vector4D Matrix4D::operator*(const Vector4D &vec) {
	return Vector4D(rows[0].dot(vec), rows[1].dot(vec), rows[2].dot(vec), rows[3].dot(vec));
}

Matrix4D Matrix4D::operator*(const Matrix4D &mat) {
	Matrix4D out;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			float sum = 0.0f;
			for (int k = 0; k < 4; ++k) {
				sum += rows[i][k] * mat.rows[k][j];
			}
			out[i][j] = sum;
		}
	}
	return out;
}

//static creators
Matrix4D Matrix4D::identity() {
	return Matrix4D(
		Vector4D(1.0f, 0.0f, 0.0f, 0.0f),
		Vector4D(0.0f, 1.0f, 0.0f, 0.0f),
		Vector4D(0.0f, 0.0f, 1.0f, 0.0f),
		Vector4D(0.0f, 0.0f, 0.0f, 1.0f)
	);
}
Matrix4D Matrix4D::translate(const Vector3D &position) {
	return Matrix4D(
		Vector4D(1.0f, 0.0f, 0.0f, -position[0]),
		Vector4D(0.0f, 1.0f, 0.0f, -position[1]),
		Vector4D(0.0f, 0.0f, 1.0f, -position[2]),
		Vector4D(0.0f, 0.0f, 0.0f, 1.0f)
	);
}
Matrix4D Matrix4D::view(const Vector3D &right, const Vector3D &up, const Vector3D &forward) {
	/*return Matrix4D(
		Vector4D(right[0], up[0], forward[0], -position[0]),
		Vector4D(right[1], up[1], forward[1], -position[1]),
		Vector4D(right[2], up[2], forward[2], -position[2]),
		Vector4D(0.0f, 0.0f, 0.0f, 1.0)
	);*/

	return Matrix4D(
		Vector4D(right[0],		right[1],		right[2],		0.0f),
		Vector4D(up[0],			up[1],			up[2],			0.0f),
		Vector4D(-forward[0],	-forward[1],	-forward[2],	0.0f),
		Vector4D(0.0f,			0.0f,			0.0f,			1.0f)
	);
}
Matrix4D Matrix4D::orthographic_projection(const Vector2D &size, float n, float f) {
	/*return Matrix4D(
		Vector4D(1.0f/ size[0],	0.0f,			0.0f,				0.0f),
		Vector4D(0.0f, 			1.0f / size[1],	0.0f,				0.0f),
		Vector4D(0.0f, 			0.0f,			-2.0f / (f - n),	-(f + n) / (f - n)),
		Vector4D(0.0f, 			0.0f,			0.0f,				1.0)
	);*/

	return Matrix4D(
		Vector4D(1.0f/ size[0],	0.0f,			0.0f,				0.0f),
		Vector4D(0.0f, 			1.0f / size[1],	0.0f,				0.0f),
		Vector4D(0.0f, 			0.0f,			-1.0f / f,			0.0f),
		Vector4D(0.0f, 			0.0f,			0.0f,				1.0f)
	);
}
