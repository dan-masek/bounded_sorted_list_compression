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
#include <fastac/static_data_model.hpp>

#include <fastac/constants.hpp>
#include <fastac/error.hpp>

#include <iostream>
// ============================================================================
static_data_model::static_data_model()
    : distribution(nullptr)
    , decoder_table(nullptr)
    , data_memory_size(0)
    , data_symbols(0)
{
    // std::cout << "$ static_data_model : created\n";
}
// ----------------------------------------------------------------------------
static_data_model::static_data_model(uint32_t number_of_symbols
    , const double probability[])
    : distribution(nullptr)
    , decoder_table(nullptr)
    , data_symbols(0)
{
    // std::cout << "$ static_data_model : created\n";
    set_distribution(number_of_symbols, probability);
}
// ----------------------------------------------------------------------------
static_data_model::~static_data_model()
{
    delete[] distribution;
    // std::cout << "$ static_data_model : destroyed\n";
}
// ----------------------------------------------------------------------------
void static_data_model::set_distribution(uint32_t number_of_symbols
    , const double probability[])
{
    if ((number_of_symbols < 2) || (number_of_symbols > (1 << 11))) {
        AC_Error("Invalid number of data symbols");
    }

    if (data_symbols != number_of_symbols) {
        // assign memory for data model
        data_symbols = number_of_symbols;
        last_symbol = data_symbols - 1;
        delete[] distribution;

        // define size of table for fast decoding
        if (data_symbols > 16) {
            uint32_t table_bits = 3;
            while (data_symbols > (1U << (table_bits + 2))) {
                ++table_bits;
            }
            table_size = (1 << table_bits) + 4;
            table_shift = DM__LengthShift - table_bits;
            data_memory_size = data_symbols + table_size + 6;
            distribution = new uint32_t[data_memory_size];
            decoder_table = distribution + data_symbols;
        } else {
            // small alphabet: no table needed
            decoder_table = nullptr;
            table_size = table_shift = 0;
            data_memory_size = data_symbols;
            distribution = new uint32_t[data_memory_size];
        }

        if (distribution == nullptr) {
            AC_Error("Cannot assign model memory");
        }
    }

    // compute cumulative distribution, decoder table
    uint32_t s = 0;
    double sum = 0.0;
    double p = 1.0 / double(data_symbols);

    for (uint32_t k(0); k < data_symbols; ++k) {
        if (probability) {
            p = probability[k];
        }
        if ((p < 0.0001) || (p > 0.9999)) {
            AC_Error("Invalid symbol probability");
        }
        distribution[k] = static_cast<uint32_t>(sum * (1 << DM__LengthShift));
        sum += p;
        if (table_size == 0) {
            continue;
        }
        uint32_t w = distribution[k] >> table_shift;
        while (s < w) {
            decoder_table[++s] = k - 1;
        }
    }

    if (table_size != 0) {
        decoder_table[0] = 0;
        while (s <= table_size) {
            decoder_table[++s] = data_symbols - 1;
        }
    }

    if ((sum < 0.9999) || (sum > 1.0001)) {
        AC_Error("Invalid probabilities");
    }

    std::cout << "$ static_data_model : initialized (symbols="
        << data_symbols << ", memory=" << memory_usage() << ")\n";
}
// ----------------------------------------------------------------------------
size_t static_data_model::memory_usage() const
{
    return ((data_memory_size * sizeof(uint32_t)) + sizeof(*this));
}
// ============================================================================
