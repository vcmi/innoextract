/*
 * Copyright (C) 2024 Daniel Scharrer
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the author(s) be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

// Based on Inno Setup's ChaCha20.pas which is in turn based on https://github.com/Ginurx/chacha20-c

#include "crypto/xchacha20.hpp"

#include <algorithm>
#include <cstring>

#include "util/endian.hpp"
#include "util/math.hpp"
#include "util/test.hpp"

namespace crypto {

void xchacha20::init(const char key[key_size], const char nonce[nonce_size]) {
	
	char subkey[key_size];
	derive_subkey(key, nonce, subkey);
	init_state(state, subkey);
	state[12] = 0;
	state[13] = 0;
	state[14] = util::little_endian::load<word>(nonce + 16);
	state[15] = util::little_endian::load<word>(nonce + 20);
	pos = sizeof(keystream);
	
}

void xchacha20::discard(size_t length) {
	
	// asume pos > 0 && pos <= sizeof(keystream)
	
	if(pos != sizeof(keystream)) {
		size_t remaining = std::min(length, size_t(64) - size_t(pos));
		pos = boost::uint8_t(pos + remaining);
		length -= remaining;
	}
	
	// assert length == 0 || pos == sizeof(keystream)
	
	increment_count(state, length / sizeof(keystream));
	
	if(length % sizeof(keystream)) {
		update();
		pos = boost::uint8_t(length % sizeof(keystream));
	}
	
}

void xchacha20::crypt(const char * in, char * out, size_t length) {
	
	// asume pos > 0 && pos <= 64
	
	for(size_t i = 0; i < length; i++, pos++) {
		if(pos == sizeof(keystream)) {
			update();
			pos = 0;
		}
		boost::uint8_t key = boost::uint8_t(keystream[pos / sizeof(word)] >> ((pos % sizeof(word)) * 8));
		out[i] = char(boost::uint8_t(in[i]) ^ key);
	}
	
}


void xchacha20::update() {
	
	std::memcpy(keystream, state, sizeof(state));
	run_rounds(keystream);
	
	for(size_t i = 0; i < 16; i++) {
		keystream[i] += state[i];
	}
	
	increment_count(state);
	
}

void xchacha20::derive_subkey(const char key[key_size], const char nonce[16], char subkey[key_size]) {
	
	word state[16];
	init_state(state, key);
	state[12] = util::little_endian::load<word>(nonce);
	state[13] = util::little_endian::load<word>(nonce + 4);
	state[14] = util::little_endian::load<word>(nonce + 8);
	state[15] = util::little_endian::load<word>(nonce + 12);
	run_rounds(state);
	util::little_endian::store<word>(state[0], subkey);
	util::little_endian::store<word>(state[1], subkey + 4);
	util::little_endian::store<word>(state[2], subkey + 8);
	util::little_endian::store<word>(state[3], subkey + 12);
	util::little_endian::store<word>(state[12], subkey + 16);
	util::little_endian::store<word>(state[13], subkey + 20);
	util::little_endian::store<word>(state[14], subkey + 24);
	util::little_endian::store<word>(state[15], subkey + 28);
	
}


void xchacha20::init_state(word state[16], const char key[key_size]) {
	
	state[0] = 0x61707865;
	state[1] = 0x3320646e;
	state[2] = 0x79622d32;
	state[3] = 0x6b206574;
	for(size_t i = 0; i < 8; i++) {
		state[4 + i] = util::little_endian::load<word>(key + i * sizeof(word));
	}
	
}

void xchacha20::run_rounds(word keystream[16]) {
	
	#define CHACHA20_QR(a, b, c, d) \
		a += b; \
		d = d ^ a; \
		d = util::rotl_fixed(d, 16); \
		c += d; \
		b = b ^ c; \
		b = util::rotl_fixed(b, 12); \
		a += b; \
		d = d ^ a; \
		d = util::rotl_fixed(d, 8); \
		c += d; \
		b = b ^ c; \
		b = util::rotl_fixed(b, 7);
	
	for(size_t i = 0; i < 10; i++) {
		CHACHA20_QR(keystream[0], keystream[4], keystream[8], keystream[12]);  // column 0
		CHACHA20_QR(keystream[1], keystream[5], keystream[9], keystream[13]);  // column 1
		CHACHA20_QR(keystream[2], keystream[6], keystream[10], keystream[14]); // column 2
		CHACHA20_QR(keystream[3], keystream[7], keystream[11], keystream[15]); // column 3
		CHACHA20_QR(keystream[0], keystream[5], keystream[10], keystream[15]); // diagonal 1 (main diagonal)
		CHACHA20_QR(keystream[1], keystream[6], keystream[11], keystream[12]); // diagonal 2
		CHACHA20_QR(keystream[2], keystream[7], keystream[8], keystream[13]);  // diagonal 3
		CHACHA20_QR(keystream[3], keystream[4], keystream[9], keystream[14]);  // diagonal 4
	}
	
	#undef CHACHA20_QR
	
}

void xchacha20::increment_count(word state[16], size_t increment) {
	
	boost::uint64_t count = (boost::uint64_t(state[13]) << (sizeof(word) * 8)) + state[12];
	count += increment;
	state[12] = word(count);
	state[13] = word(count >> (sizeof(word) * 8));
	
}

INNOEXTRACT_TEST(xchacha20,
	
	// Testcase from Inno Setup's TestHChaCha20
	
	const boost::uint8_t hkey[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
	};
	const boost::uint8_t hnonce[] = {
		0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x31, 0x41, 0x59, 0x27
	};
	const boost::uint8_t subkey[] = {
		0x82, 0x41, 0x3b, 0x42, 0x27, 0xb2, 0x7b, 0xfe, 0xd3, 0x0e, 0x42, 0x50, 0x8a, 0x87, 0x7d, 0x73,
		0xa0, 0xf9, 0xe4, 0xd5, 0x8a, 0x74, 0xa8, 0x53, 0xc1, 0x2e, 0xc4, 0x13, 0x26, 0xd3, 0xec, 0xdc
	};
	
	char buffer[xchacha20::key_size];
	xchacha20::derive_subkey(reinterpret_cast<const char *>(hkey), reinterpret_cast<const char*>(hnonce), buffer);
	test_equals("derive_subkey", buffer, subkey, sizeof(subkey));
	
	// Testcase from Inno Setup's TestXChaCha20
	
	const boost::uint8_t key[] = {
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f
	};
	const boost::uint8_t nonce[] = {
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
		0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x58
	};
	
	const boost::uint8_t ciphertext[] = {
		0x45, 0x59, 0xab, 0xba, 0x4e, 0x48, 0xc1, 0x61, 0x02, 0xe8, 0xbb, 0x2c, 0x05, 0xe6, 0x94, 0x7f,
		0x50, 0xa7, 0x86, 0xde, 0x16, 0x2f, 0x9b, 0x0b, 0x7e, 0x59, 0x2a, 0x9b, 0x53, 0xd0, 0xd4, 0xe9,
		0x8d, 0x8d, 0x64, 0x10, 0xd5, 0x40, 0xa1, 0xa6, 0x37, 0x5b, 0x26, 0xd8, 0x0d, 0xac, 0xe4, 0xfa,
		0xb5, 0x23, 0x84, 0xc7, 0x31, 0xac, 0xbf, 0x16, 0xa5, 0x92, 0x3c, 0x0c, 0x48, 0xd3, 0x57, 0x5d,
		0x4d, 0x0d, 0x2c, 0x67, 0x3b, 0x66, 0x6f, 0xaa, 0x73, 0x10, 0x61, 0x27, 0x77, 0x01, 0x09, 0x3a,
		0x6b, 0xf7, 0xa1, 0x58, 0xa8, 0x86, 0x42, 0x92, 0xa4, 0x1c, 0x48, 0xe3, 0xa9, 0xb4, 0xc0, 0xda,
		0xec, 0xe0, 0xf8, 0xd9, 0x8d, 0x0d, 0x7e, 0x05, 0xb3, 0x7a, 0x30, 0x7b, 0xbb, 0x66, 0x33, 0x31,
		0x64, 0xec, 0x9e, 0x1b, 0x24, 0xea, 0x0d, 0x6c, 0x3f, 0xfd, 0xdc, 0xec, 0x4f, 0x68, 0xe7, 0x44,
		0x30, 0x56, 0x19, 0x3a, 0x03, 0xc8, 0x10, 0xe1, 0x13, 0x44, 0xca, 0x06, 0xd8, 0xed, 0x8a, 0x2b,
		0xfb, 0x1e, 0x8d, 0x48, 0xcf, 0xa6, 0xbc, 0x0e, 0xb4, 0xe2, 0x46, 0x4b, 0x74, 0x81, 0x42, 0x40,
		0x7c, 0x9f, 0x43, 0x1a, 0xee, 0x76, 0x99, 0x60, 0xe1, 0x5b, 0xa8, 0xb9, 0x68, 0x90, 0x46, 0x6e,
		0xf2, 0x45, 0x75, 0x99, 0x85, 0x23, 0x85, 0xc6, 0x61, 0xf7, 0x52, 0xce, 0x20, 0xf9, 0xda, 0x0c,
		0x09, 0xab, 0x6b, 0x19, 0xdf, 0x74, 0xe7, 0x6a, 0x95, 0x96, 0x74, 0x46, 0xf8, 0xd0, 0xfd, 0x41,
		0x5e, 0x7b, 0xee, 0x2a, 0x12, 0xa1, 0x14, 0xc2, 0x0e, 0xb5, 0x29, 0x2a, 0xe7, 0xa3, 0x49, 0xae,
		0x57, 0x78, 0x20, 0xd5, 0x52, 0x0a, 0x1f, 0x3f, 0xb6, 0x2a, 0x17, 0xce, 0x6a, 0x7e, 0x68, 0xfa,
		0x7c, 0x79, 0x11, 0x1d, 0x88, 0x60, 0x92, 0x0b, 0xc0, 0x48, 0xef, 0x43, 0xfe, 0x84, 0x48, 0x6c,
		0xcb, 0x87, 0xc2, 0x5f, 0x0a, 0xe0, 0x45, 0xf0, 0xcc, 0xe1, 0xe7, 0x98, 0x9a, 0x9a, 0xa2, 0x20,
		0xa2, 0x8b, 0xdd, 0x48, 0x27, 0xe7, 0x51, 0xa2, 0x4a, 0x6d, 0x5c, 0x62, 0xd7, 0x90, 0xa6, 0x63,
		0x93, 0xb9, 0x31, 0x11, 0xc1, 0xa5, 0x5d, 0xd7, 0x42, 0x1a, 0x10, 0x18, 0x49, 0x74, 0xc7, 0xc5
	};
	
	xchacha20 cipher;
	
	cipher.init(reinterpret_cast<const char*>(key), reinterpret_cast<const char *>(nonce));
	char buffer0[sizeof(ciphertext)];
	cipher.crypt(testdata, buffer0, sizeof(ciphertext));
	test_equals("crypt", buffer0, ciphertext, sizeof(ciphertext));
	
	cipher.init(reinterpret_cast<const char*>(key), reinterpret_cast<const char *>(nonce));
	cipher.crypt(testdata, buffer0, 3);
	cipher.discard(129);
	char buffer1[sizeof(ciphertext) - 132];
	cipher.crypt(testdata + 132, buffer1, sizeof(ciphertext) - 132);
	test_equals("discard", buffer1, ciphertext + 132, sizeof(ciphertext) - 132);
	
)

} // namespace crypto
