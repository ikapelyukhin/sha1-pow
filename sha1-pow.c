/* sha1-pow -- compute proof-of-work for a given 64 byte string
 * Copyright (c) 2019 Ivan Kapelyukhin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#define ASCII_MIN 0x21
#define ASCII_MAX 0x7e

/* Function prototypes */
#define BLOCK_LEN 64  // In bytes
#define STATE_LEN 5  // In words
#define SUFFIX_LEN 7

// Link this program with an external C or x86 compression function
extern void sha1_compress(uint32_t state[static STATE_LEN], const uint8_t block[static BLOCK_LEN]);

void sha1_first_block(const uint8_t message[], size_t len, uint32_t hash[static STATE_LEN]);
void sha1_add_suffix(const uint8_t suffix[], size_t len, size_t suffix_len, uint32_t hash[static STATE_LEN]);

struct xorshift32_state {
  uint32_t a;
};

uint32_t xorshift32(struct xorshift32_state *state) {
	uint32_t x = state->a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return state->a = x;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: %s <PREFIX> <DIFFICULTY>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *endptr = NULL;

	errno = 0;
	long difficulty = strtol(argv[2], &endptr, 10);
	if (errno != 0 || *endptr != '\0') {
	    fprintf(stderr, "Difficulty must be a number");
	    exit(EXIT_FAILURE);
	}

	uint8_t *prefix;
	size_t prefix_len = strlen(argv[1]);
	if (prefix_len != 64) {
	    fprintf(stderr, "Prefix length has to be 64 bytes\n");
	    exit(EXIT_FAILURE);
	}

	assert(sizeof(uint8_t) == sizeof(char));
	prefix = (uint8_t*) argv[1];

	uint8_t *suffix = malloc(sizeof(uint8_t) * SUFFIX_LEN + 1);
	if (suffix == NULL) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
	}

	memset(suffix, 0, SUFFIX_LEN + 1);

	uint32_t hash[STATE_LEN];
	uint32_t hash_fb[STATE_LEN];

	sha1_first_block(prefix, prefix_len, hash_fb);

	struct xorshift32_state state = { .a = getpid() };

	uint8_t charset_size = ASCII_MAX - ASCII_MIN;
	for (size_t i = 0; i < SUFFIX_LEN; i++) {
		suffix[i] = ASCII_MIN + xorshift32(&state) % charset_size;
	}

	uint8_t lz;
	uint8_t chunk_lz;
	uint32_t rand;
	uint32_t counter = 0;
	clock_t start_time = clock();

	while(1) {
		counter++;

		rand = xorshift32(&state);
		suffix[rand % SUFFIX_LEN] = ASCII_MIN + xorshift32(&state) % charset_size;

		memcpy(hash, hash_fb, sizeof hash);
		sha1_add_suffix(suffix, prefix_len + SUFFIX_LEN, SUFFIX_LEN, hash);

		lz = 0;
		for (int i = 0; i < STATE_LEN; i++) {
			// __builtin_clz behavior is undefined if the argument is 0
			if (hash[i] == 0) {
				lz += 8;
			} else {
				int bits = __builtin_clz(hash[i]);
				chunk_lz = bits / 4;

				lz += chunk_lz;
				break;
			}
		}

		if (lz >= difficulty) {
			printf("%s\n", suffix);
			fprintf(stderr, "Full string: %s%s\n", prefix, suffix);
			fprintf(stderr, "Hash: %08x%08x%08x%08x%08x\n", hash[0], hash[1], hash[2], hash[3], hash[4]);
			break;
		}

		if (counter % 10000000 == 0) {
			fprintf(stderr, "Hashrate: %.1f MH/s\n", (double)counter / (clock() - start_time) * CLOCKS_PER_SEC / 1000000);
			counter = 0;
			start_time = clock();
		}
	}

	return EXIT_SUCCESS;
}

void sha1_first_block(const uint8_t message[], size_t len, uint32_t hash[static STATE_LEN]) {
	hash[0] = UINT32_C(0x67452301);
	hash[1] = UINT32_C(0xEFCDAB89);
	hash[2] = UINT32_C(0x98BADCFE);
	hash[3] = UINT32_C(0x10325476);
	hash[4] = UINT32_C(0xC3D2E1F0);

	size_t off;
	for (off = 0; len - off >= BLOCK_LEN; off += BLOCK_LEN) {
		sha1_compress(hash, &message[off]);
	}
}

void sha1_add_suffix(const uint8_t suffix[], size_t len, size_t suffix_len, uint32_t hash[static STATE_LEN]) {
	#define LENGTH_SIZE 8  // In bytes

	uint8_t block[BLOCK_LEN] = {0};
	size_t rem = suffix_len;
	memcpy(block, suffix, rem);

	block[rem] = 0x80;
	rem++;
	if (BLOCK_LEN - rem < LENGTH_SIZE) {
		sha1_compress(hash, block);
		memset(block, 0, sizeof(block));
	}

	block[BLOCK_LEN - 1] = (uint8_t)((len & 0x1FU) << 3);
	len >>= 5;
	for (int i = 1; i < LENGTH_SIZE; i++, len >>= 8)
		block[BLOCK_LEN - 1 - i] = (uint8_t)(len & 0xFFU);
	sha1_compress(hash, block);
}
