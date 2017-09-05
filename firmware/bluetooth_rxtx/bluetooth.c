/*
 * Copyright 2012 Dominic Spill
 *
 * This file is part of Project Ubertooth.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "bluetooth.h"

/* index into whitening data array */
static const uint8_t INDICES[] = {99, 85, 17, 50, 102, 58, 108, 45, 92, 62, 32, 118, 88, 11, 80, 2, 37, 69, 55, 8, 20, 40, 74, 114, 15, 106, 30, 78, 53, 72, 28, 26, 68, 7, 39, 113, 105, 77, 71, 25, 84, 49, 57, 44, 61, 117, 10, 1, 123, 124, 22, 125, 111, 23, 42, 126, 6, 112, 76, 24, 48, 43, 116, 0};

static const bool WHITENING_DATA[] = {1, 1, 1, 0, 0, 0, 1, 1,
                                      1, 0, 1, 1, 0, 0, 0, 1,
                                      0, 1, 0, 0, 1, 0, 1, 1,
                                      1, 1, 1, 0, 1, 0, 1, 0,
                                      1, 0, 0, 0, 0, 1, 0, 1,
                                      1, 0, 1, 1, 1, 1, 0, 0,
                                      1, 1, 1, 0, 0, 1, 0, 1,
                                      0, 1, 1, 0, 0, 1, 1, 0,
                                      0, 0, 0, 0, 1, 1, 0, 1,
                                      1, 0, 1, 0, 1, 1, 1, 0,
                                      1, 0, 0, 0, 1, 1, 0, 0,
                                      1, 0, 0, 0, 1, 0, 0, 0,
                                      0, 0, 0, 1, 0, 0, 1, 0,
                                      0, 1, 1, 0, 1, 0, 0, 1,
                                      1, 1, 1, 0, 1, 1, 1, 0,
                                      0, 0, 0, 1, 1, 1, 1};

bdaddr   target;
uint64_t syncword;
bool     afh_enabled;
bool     afh_map[10];
uint8_t  used_channels;

/* these values for hop() can be precalculated (at leastin part) */
uint8_t  a1, b, c1, e;
uint16_t d1;
/* frequency register bank */
uint8_t bank[NUM_BREDR_CHANNELS];
uint8_t afh_bank[NUM_BREDR_CHANNELS];

/* count the number of 1 bits in a uint64_t */
static uint8_t count_bits(uint64_t n)
{
	uint8_t i = 0;
	for (i = 0; n != 0; i++)
		n &= n - 1;
	return i;
}

/* do all of the one time precalculation */
void precalc(void)
{
	uint8_t i, j, chan;
	uint32_t address;
	address = target.address & 0xffffffff;
	syncword = 0;

	/* populate frequency register bank*/
	for (i = 0; i < NUM_BREDR_CHANNELS; i++)
		bank[i] = ((i * 2) % NUM_BREDR_CHANNELS);
		/* actual frequency is 2402 + bank[i] MHz */


	/* precalculate some of next_hop()'s variables */
	a1 = (address >> 23) & 0x1f;
	b = (address >> 19) & 0x0f;
	c1 = ((address >> 4) & 0x10) +
		((address >> 3) & 0x08) +
		((address >> 2) & 0x04) +
		((address >> 1) & 0x02) +
		(address & 0x01);
	d1 = (address >> 10) & 0x1ff;
	e = ((address >> 7) & 0x40) +
		((address >> 6) & 0x20) +
		((address >> 5) & 0x10) +
		((address >> 4) & 0x08) +
		((address >> 3) & 0x04) +
		((address >> 2) & 0x02) +
		((address >> 1) & 0x01);

	if(afh_enabled) {
		used_channels = 0;
		for(i = 0; i < 10; i++)
			used_channels += count_bits((uint64_t) afh_map[i]);
		j = 0;
		for (i = 0; i < NUM_BREDR_CHANNELS; i++) {
			chan = (i * 2) % NUM_BREDR_CHANNELS;
			if(afh_map[chan/8] & (0x1 << (chan % 8)))
				bank[j++] = chan;
		}
	}
}

/* 5 bit permutation */
static uint8_t perm5(uint8_t z, uint8_t p_high, uint16_t p_low)
{
	/* z is constrained to 5 bits, p_high to 5 bits, p_low to 9 bits */
	z &= 0x1f;
	p_high &= 0x1f;
	p_low &= 0x1ff;

	int i;
	uint8_t tmp, output, z_bit[5], p[14];
	static const uint8_t index1[] = {0, 2, 1, 3, 0, 1, 0, 3, 1, 0, 2, 1, 0, 1};
	static const uint8_t index2[] = {1, 3, 2, 4, 4, 3, 2, 4, 4, 3, 4, 3, 3, 2};

	/* bits of p_low and p_high are control signals */
	for (i = 0; i < 9; i++)
		p[i] = (p_low >> i) & 0x01;
	for (i = 0; i < 5; i++)
		p[i+9] = (p_high >> i) & 0x01;

	/* bit swapping will be easier with an array of bits */
	for (i = 0; i < 5; i++)
		z_bit[i] = (z >> i) & 0x01;

	/* butterfly operations */
	for (i = 13; i >= 0; i--) {
		/* swap bits according to index arrays if control signal tells us to */
		if (p[i]) {
			tmp = z_bit[index1[i]];
			z_bit[index1[i]] = z_bit[index2[i]];
			z_bit[index2[i]] = tmp;
		}
	}

	/* reconstruct output from rearranged bits */
	output = 0;
	for (i = 0; i < 5; i++)
		output += z_bit[i] << i;

	return output;
}

uint16_t next_hop(uint32_t clock)
{
	uint8_t a, c, x, y1, perm, next_channel;
	uint16_t d, y2;
	uint32_t base_f, f, f_dash;

	clock &= 0xffffffff;
	/* Variable names used in Vol 2, Part B, Section 2.6 of the spec */
	x = (clock >> 2) & 0x1f;
	y1 = (clock >> 1) & 0x01;
	y2 = y1 << 5;
	a = (a1 ^ (clock >> 21)) & 0x1f;
	/* b is already defined */
	c = (c1 ^ (clock >> 16)) & 0x1f;
	d = (d1 ^ (clock >> 7)) & 0x1ff;
	/* e is already defined */
	base_f = (clock >> 3) & 0x1fffff0;
	f = base_f % 79;

	perm = perm5(
		((x + a) % 32) ^ b,
		(y1 * 0x1f) ^ c,
		d);
	/* hop selection */
	next_channel = bank[(perm + e + f + y2) % NUM_BREDR_CHANNELS];
	if(afh_enabled) {
		f_dash = base_f % used_channels;
		next_channel = afh_bank[(perm + e + f_dash + y2) % used_channels];
	}
	return (2402 + next_channel);

}

int find_access_code(uint8_t* idle_rxbuf)
{
	/* Looks for an AC in the stream */
	uint8_t bit_errors, curr_buf;
	int i = 0, count = 0;

	if (syncword == 0) {
		for (; i<8; i++) {
			syncword <<= 8;
			syncword = (syncword & 0xffffffffffffff00) | idle_rxbuf[i];
		}
		count = 64;
	}
	curr_buf = idle_rxbuf[i];

	// Search until we're 64 symbols from the end of the buffer
	for(; count < ((8 * DMA_SIZE) - 64); count++)
	{
		bit_errors = count_bits(syncword ^ target.syncword);

		if (bit_errors < MAX_SYNCWORD_ERRS)
			return count;

		if (count%8 == 0)
			curr_buf = idle_rxbuf[++i];

		syncword <<= 1;
		syncword = (syncword & 0xfffffffffffffffe) | ((curr_buf & 0x80) >> 7);
		curr_buf <<= 1;
	}
	return -1;
}

/* Remove the whitening from an air order array */
void whiten(bool* input, bool* output, uint32_t clkn, size_t length, size_t skip)
{
	static int index;
	size_t count;
	index = INDICES[(clkn >> 1) & 0x3f];
	index += skip;
	index %= 127;

	for(count = 0; count < length; count++)
	{
		/* unwhiten if whitened, otherwise just copy input to output */
		output[count] = input[count] ^ WHITENING_DATA[index];
		index += 1;
		index %= 127;
	}
}
