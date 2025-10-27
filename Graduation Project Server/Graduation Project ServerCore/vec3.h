#pragma once

struct vec3 {
	float x;
	float y;
	float z;

	bool operator==(const vec3& other) const
	{
		return (x == other.x) and (y == other.y) and (z == other.z);
	}

	vec3 operator+(const vec3& other) const
	{
		return vec3{ x + other.x, y + other.y, z + other.z };
	}

	vec3 operator-(const vec3& other) const
	{
		return vec3{ x - other.x, y - other.y, z - other.z };
	}

	vec3 operator*(float val) const
	{
		return vec3{ x * val, y * val, z * val };
	}

	vec3 operator/(float val) const
	{
		return vec3{ x / val, y / val, z / val };
	}

	vec3& operator+=(const vec3& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;

		return *this;
	}

	float DistanceSq() const
	{
		return x * x + y * y + z * z;
	}

	float DistanceSq(const vec3& other) const
	{
		vec3 deltaVec = *this - other;
		return powf(deltaVec.x, 2) + powf(deltaVec.y, 2) + powf(deltaVec.z, 2);
	}

	vec3 Cross(const vec3& other) const
	{
		return {
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		};
	}

	vec3 Normalize() const
	{
		float len = sqrtf(powf(x, 2) + powf(y, 2) + powf(z, 2));
		if (len == 0) return { 0, 0, 0 };
		return *this / len;
	}

	float Dot(const vec3& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}
};