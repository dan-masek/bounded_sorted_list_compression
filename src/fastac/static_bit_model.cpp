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
#include <fastac/static_bit_model.hpp>

#include <fastac/constants.hpp>
#include <fastac/error.hpp>

#include <iostream>
// ============================================================================
static_bit_model::static_bit_model()
{
    // p0 = 0.5
    bit_0_prob = 1U << (BM__LengthShift - 1);
}
// ----------------------------------------------------------------------------
static_bit_model::~static_bit_model()
{
}
// ----------------------------------------------------------------------------
void static_bit_model::set_probability_0(double p0)
{
    if ((p0 < 0.0001) || (p0 > 0.9999)) {
        AC_Error("Invalid bit probability");
    }

    bit_0_prob = static_cast<uint32_t>(p0 * (1 << BM__LengthShift));
}
// ----------------------------------------------------------------------------
size_t static_bit_model::memory_usage() const
{
    return sizeof(*this);
}
// ============================================================================
