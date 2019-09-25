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
#include <fastac/arithmetic_codec.hpp>

#include <fastac/constants.hpp>
#include <fastac/error.hpp>

#include <fastac/adaptive_bit_model.hpp>
#include <fastac/adaptive_data_model.hpp>
#include <fastac/adaptive_esc_data_model.hpp>
#include <fastac/static_bit_model.hpp>
#include <fastac/static_data_model.hpp>
// ============================================================================
arithmetic_codec::arithmetic_codec()
{
    mode = buffer_size = 0;
    new_buffer = code_buffer = 0;
}
// ----------------------------------------------------------------------------
arithmetic_codec::arithmetic_codec(uint32_t max_code_bytes, uint8_t* user_buffer)
{
    mode = buffer_size = 0;
    new_buffer = code_buffer = 0;
    set_buffer(max_code_bytes, user_buffer);
}
// ----------------------------------------------------------------------------
arithmetic_codec::~arithmetic_codec()
{
    delete[] new_buffer;
}
// ============================================================================
void arithmetic_codec::set_buffer(uint32_t max_code_bytes, uint8_t* user_buffer)
{
    // test for reasonable sizes
    if ((max_code_bytes < 3) || (max_code_bytes > 0x1000000U)) {
        AC_Error("Invalid codec buffer size");
    }

    if (mode != 0) {
        AC_Error("Cannot set buffer while encoding or decoding");
    }

    // user provides memory buffer
    if (user_buffer != nullptr) {
        buffer_size = max_code_bytes;
        // set buffer for compressed data
        code_buffer = user_buffer;

        // free anything previously assigned
        delete[] new_buffer;
        new_buffer = nullptr;

        return;
    }

    if (max_code_bytes <= buffer_size) {
        return; // enough available
    }

    buffer_size = max_code_bytes; // assign new memory
    delete[] new_buffer; // free anything previously assigned
    if ((new_buffer = new uint8_t[buffer_size + 16]) == nullptr) {
        // 16 extra bytes
        AC_Error("cannot assign memory for compressed data buffer");
    }

    code_buffer = new_buffer; // set buffer for compressed data
}
// ============================================================================
inline void arithmetic_codec::propagate_carry()
{
    // carry propagation on compressed data buffer
    uint8_t* p;
    for (p = ac_pointer - 1; *p == 0xFFU; p--) {
        *p = 0;
    }
    ++*p;
}
// ----------------------------------------------------------------------------
inline void arithmetic_codec::renorm_enc_interval()
{
    do {                                          
        // output and discard top byte
        *ac_pointer++ = static_cast<uint8_t>(base >> 24);
        base <<= 8;
    } while ((length <<= 8) < AC__MinLength); // length multiplied by 256
}
// ----------------------------------------------------------------------------
inline void arithmetic_codec::renorm_dec_interval()
{
    do {
        // read least-significant byte
        value = (value << 8) | static_cast<uint32_t>(*++ac_pointer);
    } while ((length <<= 8) < AC__MinLength); // length multiplied by 256
}
// ============================================================================
void arithmetic_codec::put_bit(uint32_t bit)
{
#ifdef _DEBUG
    if (mode != 1) AC_Error("encoder not initialized");
#endif

    length >>= 1; // halve interval
    if (bit) {
        uint32_t init_base = base;
        base += length; // move base
        if (init_base > base) {
            propagate_carry(); // overflow = carry
        }
    }

    if (length < AC__MinLength) {
        renorm_enc_interval(); // renormalization
    }
}
// ----------------------------------------------------------------------------
uint32_t arithmetic_codec::get_bit()
{
#ifdef _DEBUG
    if (mode != 2) AC_Error("decoder not initialized");
#endif

    length >>= 1; // halve interval
    uint32_t bit = (value >= length); // decode bit
    if (bit) {
        value -= length; // move base
    }

    if (length < AC__MinLength) {
        renorm_dec_interval(); // renormalization
    }

    return bit; // return data bit value
}
// ----------------------------------------------------------------------------
void arithmetic_codec::put_bits(uint32_t data, uint32_t bits)
{
#ifdef _DEBUG
    if (mode != 1) AC_Error("encoder not initialized");
    if ((bits < 1) || (bits > 20)) AC_Error("invalid number of bits");
    if (data >= (1U << bits)) AC_Error("invalid data");
#endif

    uint32_t init_base = base;
    base += data * (length >>= bits); // new interval base and length

    if (init_base > base) {
        propagate_carry(); // overflow = carry
    }
    if (length < AC__MinLength) {
        renorm_enc_interval(); // renormalization
    }
}
// ----------------------------------------------------------------------------
uint32_t arithmetic_codec::get_bits(uint32_t bits)
{
#ifdef _DEBUG
    if (mode != 2) AC_Error("decoder not initialized");
    if ((bits < 1) || (bits > 20)) AC_Error("invalid number of bits");
#endif

    uint32_t s = value / (length >>= bits); // decode symbol, change length

    value -= length * s; // update interval
    if (length < AC__MinLength) {
        renorm_dec_interval(); // renormalization
    }

    return s;
}
// ============================================================================
void arithmetic_codec::encode(uint32_t bit, static_bit_model& M)
{
#ifdef _DEBUG
    if (mode != 1) AC_Error("encoder not initialized");
#endif

    uint32_t x = M.bit_0_prob * (length >> BM__LengthShift); // product l x p0

    // update interval
    if (bit == 0) {
        length = x;
    } else {
        uint32_t init_base = base;
        base += x;
        length -= x;
        if (init_base > base) {
            propagate_carry(); // overflow = carry
        }
    }

    if (length < AC__MinLength) {
        renorm_enc_interval(); // renormalization
    }
}
// ----------------------------------------------------------------------------
uint32_t arithmetic_codec::decode(static_bit_model& M)
{
#ifdef _DEBUG
    if (mode != 2) AC_Error("decoder not initialized");
#endif

    uint32_t x = M.bit_0_prob * (length >> BM__LengthShift); // product l x p0
    uint32_t bit = (value >= x); // decision

    // update & shift interval
    if (bit == 0) {
        length = x;
    } else {
        value -= x; // shifted interval base = 0
        length -= x;
    }

    if (length < AC__MinLength) {
        renorm_dec_interval(); // renormalization
    }

    return bit; // return data bit value
}
// ============================================================================
void arithmetic_codec::encode(uint32_t bit, adaptive_bit_model& M)
{
#ifdef _DEBUG
    if (mode != 1) AC_Error("encoder not initialized");
#endif

    uint32_t x = M.bit_0_prob * (length >> BM__LengthShift); // product l x p0

    // update interval
    if (bit == 0) {
        length = x;
        ++M.bit_0_count;
    } else {
        uint32_t init_base = base;
        base += x;
        length -= x;
        if (init_base > base) {
            propagate_carry(); // overflow = carry
        }
    }

    if (length < AC__MinLength) {
        renorm_enc_interval(); // renormalization
    }

    if (--M.bits_until_update == 0) {
        M.update(); // periodic model update
    }
}
// ----------------------------------------------------------------------------
unsigned arithmetic_codec::decode(adaptive_bit_model& M)
{
#ifdef _DEBUG
    if (mode != 2) AC_Error("decoder not initialized");
#endif

    uint32_t x = M.bit_0_prob * (length >> BM__LengthShift); // product l x p0
    uint32_t bit = (value >= x); // decision

    // update interval
    if (bit == 0) {
        length = x;
        ++M.bit_0_count;
    } else {
        value -= x;
        length -= x;
    }

    if (length < AC__MinLength) {
        renorm_dec_interval(); // renormalization
    }

    if (--M.bits_until_update == 0) {
        M.update(); // periodic model update
    }

    return bit; // return data bit value
}
// ============================================================================
void arithmetic_codec::encode(uint32_t data, static_data_model& M)
{
#ifdef _DEBUG
    if (mode != 1) AC_Error("encoder not initialized");
    if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

    uint32_t x, init_base = base;
    // compute products
    if (data == M.last_symbol) {
        x = M.distribution[data] * (length >> DM__LengthShift);
        base += x; // update interval
        length -= x; // no product needed
    } else {
        x = M.distribution[data] * (length >>= DM__LengthShift);
        base += x; // update interval
        length = M.distribution[data + 1] * length - x;
    }

    if (init_base > base) propagate_carry(); // overflow = carry

    if (length < AC__MinLength) renorm_enc_interval(); // renormalization
}
// ----------------------------------------------------------------------------
uint32_t arithmetic_codec::decode(static_data_model& M)
{
#ifdef _DEBUG
    if (mode != 2) AC_Error("decoder not initialized");
#endif

    uint32_t n, s, x, y = length;

    if (M.decoder_table) {
        // use table look-up for faster decoding

        uint32_t dv = value / (length >>= DM__LengthShift);
        uint32_t t = dv >> M.table_shift;

        s = M.decoder_table[t]; // initial decision based on table look-up
        n = M.decoder_table[t + 1] + 1;

        while (n > s + 1) { // finish with bisection search
            uint32_t m = (s + n) >> 1;
            if (M.distribution[m] > dv) {
                n = m;
            } else {
                s = m;
            }
        }
        // compute products
        x = M.distribution[s] * length;
        if (s != M.last_symbol) {
            y = M.distribution[s + 1] * length;
        }
    } else { 
        // decode using only multiplications

        x = s = 0;
        length >>= DM__LengthShift;
        uint32_t m = (n = M.data_symbols) >> 1;
        // decode via bisection search
        do {
            uint32_t z = length * M.distribution[m];
            if (z > value) {
                n = m;
                y = z; // value is smaller
            } else {
                s = m;
                x = z; // value is larger or equal
            }
        } while ((m = (s + n) >> 1) != s);
    }

    value -= x; // update interval
    length = y - x;

    if (length < AC__MinLength) {
        renorm_dec_interval(); // renormalization
    }

    return s;
}
// ============================================================================
void arithmetic_codec::encode(uint32_t data, adaptive_data_model& M)
{
#ifdef _DEBUG
    if (mode != 1) AC_Error("encoder not initialized");
    if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

    uint32_t x, init_base = base;
    // compute products
    if (data == M.last_symbol) {
        x = M.distribution[data] * (length >> DM__LengthShift);
        base += x; // update interval
        length -= x; // no product needed
    } else {
        x = M.distribution[data] * (length >>= DM__LengthShift);
        base += x; // update interval
        length = M.distribution[data + 1] * length - x;
    }

    if (init_base > base) {
        propagate_carry(); // overflow = carry
    }

    if (length < AC__MinLength) {
        renorm_enc_interval(); // renormalization
    }

    ++M.symbol_count[data];
    if (--M.symbols_until_update == 0) {
        M.update(true); // periodic model update
    }
}
// ----------------------------------------------------------------------------
uint32_t arithmetic_codec::decode(adaptive_data_model& M)
{
#ifdef _DEBUG
    if (mode != 2) AC_Error("decoder not initialized");
#endif

    uint32_t n, s, x, y = length;

    if (M.decoder_table) { 
        // use table look-up for faster decoding

        uint32_t dv = value / (length >>= DM__LengthShift);
        uint32_t t = dv >> M.table_shift;

        s = M.decoder_table[t]; // initial decision based on table look-up
        n = M.decoder_table[t + 1] + 1;

        while (n > s + 1) { // finish with bisection search
            uint32_t m = (s + n) >> 1;
            if (M.distribution[m] > dv) {
                n = m;
            } else {
                s = m;
            }
        }
        // compute products
        x = M.distribution[s] * length;
        if (s != M.last_symbol) {
            y = M.distribution[s + 1] * length;
        }
    } else {
        // decode using only multiplications

        x = s = 0;
        length >>= DM__LengthShift;
        uint32_t m = (n = M.data_symbols) >> 1;
        // decode via bisection search
        do {
            uint32_t z = length * M.distribution[m];
            if (z > value) {
                n = m;
                y = z; // value is smaller
            } else {
                s = m;
                x = z; // value is larger or equal
            }
        } while ((m = (s + n) >> 1) != s);
    }

    value -= x; // update interval
    length = y - x;

    if (length < AC__MinLength) {
        renorm_dec_interval(); // renormalization
    }

    ++M.symbol_count[s];
    if (--M.symbols_until_update == 0) {
        M.update(false); // periodic model update
    }

    return s;
}
// ============================================================================
void arithmetic_codec::encode(uint32_t data, adaptive_esc_data_model& M)
{
#ifdef _DEBUG
    if (mode != 1) AC_Error("encoder not initialized");
    if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

    if (!M.has_symbol(data)) {
        AC_Error("Symbol not in model");
    }

    uint32_t x, init_base = base;
    // compute products
    if (data == M.last_symbol) {
        x = M.distribution[data] * (length >> DM__LengthShift);
        base += x; // update interval
        length -= x; // no product needed
    } else {
        x = M.distribution[data] * (length >>= DM__LengthShift);
        base += x; // update interval
        length = M.distribution[data + 1] * length - x;
    }

    if (init_base > base) {
        propagate_carry(); // overflow = carry
    }

    if (length < AC__MinLength) {
        renorm_enc_interval(); // renormalization
    }

    ++M.symbol_count[data];
    if (--M.symbols_until_update == 0) {
        M.update(true); // periodic model update
    }
}
// ----------------------------------------------------------------------------
uint32_t arithmetic_codec::decode(adaptive_esc_data_model& M)
{
#ifdef _DEBUG
    if (mode != 2) AC_Error("decoder not initialized");
#endif

    uint32_t n, s, x, y = length;

    if (M.decoder_table) {
        // use table look-up for faster decoding

        uint32_t dv = value / (length >>= DM__LengthShift);
        uint32_t t = dv >> M.table_shift;

        s = M.decoder_table[t]; // initial decision based on table look-up
        n = M.decoder_table[t + 1] + 1;

        while (n > s + 1) { // finish with bisection search
            uint32_t m = (s + n) >> 1;
            if (M.distribution[m] > dv) {
                n = m;
            } else {
                s = m;
            }
        }
        // compute products
        x = M.distribution[s] * length;
        if (s != M.last_symbol) {
            y = M.distribution[s + 1] * length;
        }
    } else {
        // decode using only multiplications

        x = s = 0;
        length >>= DM__LengthShift;
        uint32_t m = (n = M.data_symbols) >> 1;
        // decode via bisection search
        do {
            uint32_t z = length * M.distribution[m];
            if (z > value) {
                n = m;
                y = z; // value is smaller
            } else {
                s = m;
                x = z; // value is larger or equal
            }
        } while ((m = (s + n) >> 1) != s);
    }

    value -= x; // update interval
    length = y - x;

    if (length < AC__MinLength) {
        renorm_dec_interval(); // renormalization
    }

    ++M.symbol_count[s];
    if (--M.symbols_until_update == 0) {
        M.update(false); // periodic model update
    }

    return s;
}
// ============================================================================
void arithmetic_codec::start_encoder()
{
    if (mode != 0) {
        AC_Error("cannot start encoder");
    }
    if (buffer_size == 0) {
        AC_Error("no code buffer set");
    }

    mode = 1;
    base = 0;  // initialize encoder variables: interval and pointer
    length = AC__MaxLength;
    ac_pointer = code_buffer; // pointer to next data byte
}
// ----------------------------------------------------------------------------
void arithmetic_codec::start_decoder()
{
    if (mode != 0) {
        AC_Error("cannot start decoder");
    }
    if (buffer_size == 0) {
        AC_Error("no code buffer set");
    }

    // initialize decoder: interval, pointer, initial code value
    mode = 2;
    length = AC__MaxLength;
    ac_pointer = code_buffer + 3;
    value = (static_cast<uint32_t>(code_buffer[0]) << 24)
        | (static_cast<uint32_t>(code_buffer[1]) << 16)
        | (static_cast<uint32_t>(code_buffer[2]) << 8)
        | static_cast<uint32_t>(code_buffer[3]);
}
// ----------------------------------------------------------------------------
uint32_t arithmetic_codec::stop_encoder()
{
    if (mode != 1) {
        AC_Error("invalid to stop encoder");
    }

    mode = 0;

    uint32_t init_base = base; // done encoding: set final data bytes

    if (length > 2 * AC__MinLength) {
        base += AC__MinLength; // base offset
        length = AC__MinLength >> 1; // set new length for 1 more byte
    } else {
        base += AC__MinLength >> 1; // base offset
        length = AC__MinLength >> 9; // set new length for 2 more bytes
    }

    // overflow = carry
    if (init_base > base) {
        propagate_carry();
    }

    renorm_enc_interval(); // renormalization = output last bytes

    uint32_t code_bytes = static_cast<uint32_t>(ac_pointer - code_buffer);
    if (code_bytes > buffer_size) {
        AC_Error("code buffer overflow");
    }

    return code_bytes; // number of bytes used
}
// ----------------------------------------------------------------------------
void arithmetic_codec::stop_decoder()
{
    if (mode != 2) {
        AC_Error("invalid to stop decoder");
    }

    mode = 0;
}
// ============================================================================
void arithmetic_codec::read_from_file(FILE* code_file)
{
    uint32_t shift = 0, code_bytes = 0;
    int file_byte;

    // read variable-length header with number of code bytes
    do {
        if ((file_byte = getc(code_file)) == EOF) {
            AC_Error("cannot read code from file");
        }
        code_bytes |= static_cast<uint32_t>(file_byte & 0x7F) << shift;
        shift += 7;
    } while (file_byte & 0x80);
    // read compressed data
    if (code_bytes > buffer_size) {
        AC_Error("code buffer overflow");
    }
    if (fread(code_buffer, 1, code_bytes, code_file) != code_bytes) {
        AC_Error("cannot read code from file");
    }

    start_decoder(); // initialize decoder
}
// ----------------------------------------------------------------------------
uint32_t arithmetic_codec::write_to_file(FILE* code_file)
{
    uint32_t header_bytes = 0, code_bytes = stop_encoder(), nb = code_bytes;

    // write variable-length header with number of code bytes
    do {
        int file_byte = int(nb & 0x7FU);
        if ((nb >>= 7) > 0) {
            file_byte |= 0x80;
        }
        if (putc(file_byte, code_file) == EOF) {
            AC_Error("cannot write compressed data to file");
        }
        header_bytes++;
    } while (nb);

    // write compressed data
    if (fwrite(code_buffer, 1, code_bytes, code_file) != code_bytes) {
        AC_Error("cannot write compressed data to file");
    }

    // bytes used
    return code_bytes + header_bytes;
}
// ============================================================================
