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
#include <fastac/adaptive_bit_model.hpp>

#include <fastac/constants.hpp>

#include <iostream>
// ============================================================================
adaptive_bit_model::adaptive_bit_model()
{
    reset();
}
// ----------------------------------------------------------------------------
adaptive_bit_model::~adaptive_bit_model()
{
}
// ----------------------------------------------------------------------------
void adaptive_bit_model::reset()
{
    // initialization to equiprobable model
    bit_0_count = 1;
    bit_count = 2;
    bit_0_prob = 1U << (BM__LengthShift - 1);

    // start with frequent updates
    update_cycle = bits_until_update = 4;
}
// ----------------------------------------------------------------------------
void adaptive_bit_model::update()
{
    // halve counts when a threshold is reached
    if ((bit_count += update_cycle) > BM__MaxCount) {
        bit_count = (bit_count + 1) >> 1;
        bit_0_count = (bit_0_count + 1) >> 1;
        if (bit_0_count == bit_count) {
            ++bit_count;
        }
    }

    // compute scaled bit 0 probability
    uint32_t scale = 0x80000000U / bit_count;
    bit_0_prob = (bit_0_count * scale) >> (31 - BM__LengthShift);

    // set frequency of model updates
    update_cycle = (5 * update_cycle) >> 2;
    if (update_cycle > 64) {
        update_cycle = 64;
    }

    bits_until_update = update_cycle;
}
// ----------------------------------------------------------------------------
size_t adaptive_bit_model::memory_usage() const
{
    return sizeof(*this);
}
// ============================================================================
