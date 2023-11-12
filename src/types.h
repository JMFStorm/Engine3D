#pragma once

#include <array>
#include <string_view>
#include <glm/glm.hpp>

static const unsigned int SHADOW_WIDTH = 1024 * 2;
static const unsigned int SHADOW_HEIGHT = 1024 * 2;

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

enum class TransformMode {
	Translate,
	Rotate,
	Scale
};

enum class ObjectType {
	None,
	Primitive,
	Pointlight,
	Spotlight
};

typedef struct SimpleShader {
	u32 id;
	u32 vao;
	u32 vbo;
} SimpleShader;

typedef struct Transforms {
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;
} Transforms;

constexpr const s64 E_Primitive_Plane = 0;
constexpr const s64 E_Primitive_Cube = 1;
constexpr const s64 E_Primitive_Sphere = 2;

