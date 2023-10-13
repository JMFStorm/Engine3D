#define KILOBYTES(x) (x * 1024)
#define MEGABYTES(x) (KILOBYTES(x) * 1024)

typedef unsigned char byte;

typedef struct MemoryArena {
	unsigned long max_size;
	unsigned long used;
	byte* memory;
	unsigned long free_space()
	{
		return max_size - used;
	}
} MemoryArena;

void assert_true(bool assertion, const char* assertion_title, const char* file, const char* func, int line);

#define ASSERT_TRUE(assertion, assertion_title) assert_true(assertion, assertion_title, __FILE__, __func__, __LINE__)

float normalize_screen_px_to_ndc(int value, int max);

float normalize_value(float value, float src_max, float dest_max);

inline float vw_into_screen_px(float value, float screen_width_px)
{
	return (float)screen_width_px * value * 0.01f;
}

inline float vh_into_screen_px(float value, float screen_height_px)
{
	return (float)screen_height_px * value * 0.01f;
}

inline void null_terminate_string(char* string, int str_length)
{
	string[str_length + 1] = '\0';
}

void memory_arena_init(MemoryArena* arena, unsigned long size_in_bytes);

MemoryArena memory_arena_create_subsection(MemoryArena* arena, unsigned long size_in_bytes);

void memory_arena_reset(MemoryArena* arena);

void memory_arena_wipe(MemoryArena* arena);

void memory_arena_free(MemoryArena* arena);
