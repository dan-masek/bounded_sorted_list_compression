// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Fast arithmetic coding implementation                                     -
// -> 32-bit variables, 32-bit product, periodic updates, table decoding     -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Version 1.01  -  November 28, 2005                                        -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                  WARNING                                  -
//                                 =========                                 -
//                                                                           -
// The only purpose of this program is to demonstrate the basic principles   -
// of arithmetic coding. It is provided as is, without any express or        -
// implied warranty, without even the warranty of fitness for any particular -
// purpose, or that the implementations are correct.                         -
//                                                                           -
// Permission to copy and redistribute this code is hereby granted, provided -
// that this warning and copyright notices are not removed or altered.       -
//                                                                           -
// Copyright (c) 2004 by Amir Said (said@ieee.org) &                         -
//                       William A. Pearlman (pearlw@ecse.rpi.edu)           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// A description of the arithmetic coding method used here is available in   -
//                                                                           -
// Lossless Compression Handbook, ed. K. Sayood                              -
// Chapter 5: Arithmetic Coding (A. Said), pp. 101-152, Academic Press, 2003 -
//                                                                           -
// A. Said, Introduction to Arithetic Coding Theory and Practice             -
// HP Labs report HPL-2004-76  -  http://www.hpl.hp.com/techreports/         -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#pragma once
// ============================================================================
#include <cstdint>
#include <cstdio>
// ============================================================================
class static_bit_model;
class static_data_model;
class adaptive_bit_model;
class adaptive_data_model;
class adaptive_esc_data_model;
// ============================================================================
// Class with both the arithmetic encoder and decoder.  All compressed data is
// saved to a memory buffer
class arithmetic_codec
{
public:
    arithmetic_codec();
    ~arithmetic_codec();

    // 0 = assign new
    arithmetic_codec(uint32_t max_code_bytes, uint8_t* user_buffer = nullptr);

    uint8_t* buffer();
    uint8_t const* buffer() const;

    void set_buffer(uint32_t max_code_bytes, uint8_t* user_buffer = nullptr);

    void start_encoder();
    void start_decoder();
    void read_from_file(FILE * code_file); // read code data, start decoder

    uint32_t stop_encoder(); // returns number of bytes used
    uint32_t write_to_file(FILE * code_file); // stop encoder, write code data
    void stop_decoder();

    void put_bit(uint32_t bit);
    uint32_t get_bit();

    void put_bits(uint32_t data, uint32_t number_of_bits);
    uint32_t get_bits(uint32_t number_of_bits);

    void encode(uint32_t bit,static_bit_model &);
    uint32_t decode(static_bit_model &);

    void encode(uint32_t data, static_data_model &);
    uint32_t decode(static_data_model &);

    void encode(uint32_t bit, adaptive_bit_model &);
    uint32_t decode(adaptive_bit_model &);

    void encode(uint32_t data, adaptive_data_model &);
    uint32_t decode(adaptive_data_model &);

    void encode(uint32_t data, adaptive_esc_data_model &);
    uint32_t decode(adaptive_esc_data_model &);

private:
    void propagate_carry();
    void renorm_enc_interval();
    void renorm_dec_interval();

private:
    uint8_t* code_buffer;
    uint8_t* new_buffer;
    uint8_t* ac_pointer;
    uint32_t base, value, length; // arithmetic coding state
    uint32_t buffer_size, mode; // mode: 0 = undef, 1 = encoder, 2 = decoder
};
// ============================================================================
inline uint8_t* arithmetic_codec::buffer()
{
    return code_buffer;
}
// ----------------------------------------------------------------------------
inline uint8_t const* arithmetic_codec::buffer() const
{
    return code_buffer;
}
// ============================================================================
