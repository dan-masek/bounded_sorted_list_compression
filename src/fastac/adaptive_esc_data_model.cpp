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
#include <fastac/adaptive_esc_data_model.hpp>

#include <fastac/constants.hpp>
#include <fastac/error.hpp>

#include <iostream>
// ============================================================================
adaptive_esc_data_model::adaptive_esc_data_model()
    : distribution(nullptr)
    , symbol_count(nullptr)
    , decoder_table(nullptr)
    , data_memory_size(0)
    , data_symbols(0)
{
}
// ----------------------------------------------------------------------------
adaptive_esc_data_model::adaptive_esc_data_model(uint32_t number_of_symbols)
    : distribution(nullptr)
    , symbol_count(nullptr)
    , decoder_table(nullptr)
    , data_symbols(0)
{
    set_alphabet(number_of_symbols);
}
// ----------------------------------------------------------------------------
adaptive_esc_data_model::~adaptive_esc_data_model()
{
    delete[] distribution;
}
// ----------------------------------------------------------------------------
void adaptive_esc_data_model::set_alphabet(uint32_t number_of_symbols)
{
    if ((number_of_symbols < 2) || (number_of_symbols > (1 << 11))) {
        AC_Error("invalid number of data symbols");
    }

    uint32_t real_number_of_symbols = number_of_symbols + 1;

    if (data_symbols != real_number_of_symbols) {
        // assign memory for data model
        data_symbols = real_number_of_symbols;
        last_symbol = data_symbols - 1;
        delete[] distribution;
        
        if (data_symbols > 16) {
            // define size of table for fast decoding
            uint32_t table_bits = 3;
            while (data_symbols > (1U << (table_bits + 2))) {
                ++table_bits;
            }
            table_size = (1 << table_bits) + 4;
            table_shift = DM__LengthShift - table_bits;
            data_memory_size = 2 * data_symbols + table_size + 6;
            distribution = new uint32_t[data_memory_size];
            decoder_table = distribution + 2 * data_symbols;
        } else {
            // small alphabet: no table needed
            decoder_table = nullptr;
            table_size = table_shift = 0;
            data_memory_size = 2 * data_symbols;
            distribution = new uint32_t[data_memory_size];
        }
        symbol_count = distribution + data_symbols;
        if (distribution == 0) {
            AC_Error("cannot assign model memory");
        }
    }

    // initialize model
    reset();

    std::cout << "$ adaptive_esc_data_model : initialized (symbols="
        << data_symbols << ", memory=" << memory_usage() << ")\n";
}
// ----------------------------------------------------------------------------
void adaptive_esc_data_model::update(bool from_encoder)
{
    total_count += update_cycle - symbols_until_update;

    // halve counts when a threshold is reached
    if (total_count > DM__MaxCount) {
        total_count = 0;
        for (uint32_t n(0); n < data_symbols; ++n) {
            total_count += (symbol_count[n] = (symbol_count[n] + 1) >> 1);
        }
    }

    // compute cumulative distribution, decoder table
    uint32_t scale(0x80000000U / total_count);

    if (from_encoder || (table_size == 0)) {
        uint32_t sum(0);
        for (uint32_t k(0); k < data_symbols; ++k) {
            distribution[k] = (scale * sum) >> (31 - DM__LengthShift);
            sum += symbol_count[k];
        }
    } else {
        uint32_t sum(0);
        uint32_t s(0);
        for (uint32_t k(0); k < data_symbols; ++k) {
            distribution[k] = (scale * sum) >> (31 - DM__LengthShift);
            sum += symbol_count[k];
            uint32_t w = distribution[k] >> table_shift;
            while (s < w) {
                decoder_table[++s] = k - 1;
            }
        }
        decoder_table[0] = 0;
        while (s <= table_size) {
            decoder_table[++s] = data_symbols - 1;
        }
    }
    // set frequency of model updates
    update_cycle = (5 * update_cycle) >> 2;
    uint32_t max_cycle = (data_symbols + 6) << 3;
    if (update_cycle > max_cycle) {
        update_cycle = max_cycle;
    }
    symbols_until_update = update_cycle;
}
// ----------------------------------------------------------------------------
void adaptive_esc_data_model::reset()
{
    if (data_symbols == 0) {
        return;
    }

    // Restore probability estimates of regular symbols to zeros
    total_count = 0;
    update_cycle = data_symbols;
    symbols_until_update = 0;

    for (unsigned k(0); k < (data_symbols - 1); ++k) {
        symbol_count[k] = 0;
    }
    // Escape symbol
    symbol_count[data_symbols - 1] = 1;

    update(false);

    symbols_until_update = update_cycle = (data_symbols + 6) >> 1;
}
// ----------------------------------------------------------------------------
void adaptive_esc_data_model::add_symbol(uint32_t symbol)
{
    if (symbol >= data_symbols) {
        AC_Error("Invalid symbol");
    }

    ++symbol_count[symbol];
    --symbols_until_update;
    update(false);
}
// ----------------------------------------------------------------------------
bool adaptive_esc_data_model::has_symbol(uint32_t symbol) const
{
    if (symbol >= data_symbols) {
        AC_Error("Invalid symbol");
    }

    return symbol_count[symbol] > 0;
}
// ----------------------------------------------------------------------------
bool adaptive_esc_data_model::is_escape(uint32_t symbol) const
{
    return symbol == (data_symbols - 1);
}
// ----------------------------------------------------------------------------
uint32_t adaptive_esc_data_model::get_escape() const
{
    return (data_symbols - 1);
}
// ----------------------------------------------------------------------------
size_t adaptive_esc_data_model::memory_usage() const
{
    return ((data_memory_size * sizeof(uint32_t)) + sizeof(*this));
}
// ============================================================================
