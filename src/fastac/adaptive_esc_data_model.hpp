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
// ============================================================================
// adaptive model for general data with escapes
class adaptive_esc_data_model
{
public:
    adaptive_esc_data_model();
    explicit adaptive_esc_data_model(uint32_t number_of_symbols);

    ~adaptive_esc_data_model();

    uint32_t model_symbols() const;

    // reset to base (p_esc = 1.0)
    void reset();

    bool has_symbol(uint32_t symbol) const;
    bool is_escape(uint32_t symbol) const;
    uint32_t get_escape() const;
    void add_symbol(uint32_t symbol);

    void set_alphabet(uint32_t number_of_symbols);

    size_t memory_usage() const;

private:
    void update(bool);

private:
    uint32_t* distribution;
    uint32_t* symbol_count;
    uint32_t* decoder_table;
    size_t data_memory_size;

    uint32_t total_count;
    uint32_t update_cycle;
    uint32_t symbols_until_update;

    uint32_t data_symbols;
    uint32_t last_symbol;
    uint32_t table_size;
    uint32_t table_shift;

private:
    friend class arithmetic_codec;
};
// ============================================================================
inline uint32_t adaptive_esc_data_model::model_symbols() const
{
    return data_symbols;
}
// ============================================================================
