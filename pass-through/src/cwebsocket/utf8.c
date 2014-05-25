#include "utf8.h"

int utf8_count_code_points(uint8_t* s, size_t* count) {

	uint32_t codepoint;
	uint32_t state = 0;

	for(*count = 0; *s; ++s)
		if(!utf8_decode(&state, &codepoint, *s))
			*count += 1;

	return state != UTF8_ACCEPT;
}

uint32_t utf8_decode(uint32_t* state, uint32_t* codep, uint32_t byte) {

	uint32_t type = utf8d[byte];
	*codep = (*state != UTF8_ACCEPT) ?
			(byte & 0x3fu) | (*codep << 6) :
			(0xff >> type) & (byte);

	*state = utf8d[256 + *state*16 + type];
	return *state;
}
