#pragma once

#include <array>
#include <string_view>
#include <glm/glm.hpp>

#define KILOBYTES(x) (x * 1024)
#define MEGABYTES(x) (KILOBYTES(x) * 1024)

typedef unsigned char		byte;
typedef unsigned int		u32;
typedef int					s32;
typedef unsigned long long  u64;
typedef long long			s64;
typedef float				f32;
typedef double				f64;

// Custom hash function for char*
struct CharPtrHash {
	std::size_t operator()(const char* str) const { return std::hash<std::string_view>{}(str); }
};

// Custom equality operator for char*
struct CharPtrEqual {
	bool operator()(const char* a, const char* b) const { return std::strcmp(a, b) == 0; }
};

enum class Axis {
	X,
	Y,
	Z
};

enum class MeshType {
	Plane,
	Cube
};

enum class TransformMode {
	Translate,
	Rotate,
	Scale
};

enum class ObjectType {
	None,
	Plane,
	Cube,
	Pointlight,
	Spotlight
};
