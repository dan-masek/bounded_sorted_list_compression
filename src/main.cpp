#include "utilities.hpp"

#include <fastac/arithmetic_codec.hpp>
#include <fastac/static_bit_model.hpp>
#include <fastac/static_data_model.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <vector>

#include <bzlib.h>
#undef max
#undef min
#include <zlib.h>
#include <snappy-c.h>
// ============================================================================
typedef std::vector<uint8_t> match_symbols_t;
typedef std::vector<uint32_t> match_list_t;
typedef std::set<uint32_t> match_set_t;
typedef std::vector<uint8_t> buffer_t;
// ----------------------------------------------------------------------------
static uint32_t const NUM_VALUES(1000000);
// ============================================================================
match_list_t make_random_matches(uint32_t n)
{
    static std::default_random_engine generator;
    static std::uniform_int_distribution<uint32_t> distribution(0, NUM_VALUES - 1);

    match_set_t result;
    while (result.size() < n) {
        result.insert(distribution(generator));
    }

    return match_list_t(result.begin(), result.end());
}
// ----------------------------------------------------------------------------
size_t symbol_count(uint8_t bits)
{
    size_t count(NUM_VALUES / bits);
    if (NUM_VALUES % bits > 0) {
        return count + 1;
    }
    return count;
}
// ----------------------------------------------------------------------------
void set_symbol(match_symbols_t& symbols, uint8_t bits, uint32_t match, bool state)
{
    size_t index(match / bits);
    size_t offset(match % bits);
    if (state) {
        symbols[index] |= 1 << offset;
    } else {
        symbols[index] &= ~(1 << offset);
    }
}
// ----------------------------------------------------------------------------
bool get_symbol(match_symbols_t const& symbols, uint8_t bits, uint32_t match)
{
    size_t index(match / bits);
    size_t offset(match % bits);
    return (symbols[index] & (1 << offset)) != 0;
}
// ----------------------------------------------------------------------------
match_symbols_t make_symbols(match_list_t const& matches, uint8_t bits)
{
    assert((bits > 0) && (bits <= 8));

    match_symbols_t symbols(symbol_count(bits), 0);
    for (auto match : matches) {
        set_symbol(symbols, bits, match, true);
    }

    return symbols;
}
// ----------------------------------------------------------------------------
match_list_t make_matches(match_symbols_t const& symbols, uint8_t bits)
{
    match_list_t result;
    for (uint32_t i(0); i < 1000000; ++i) {
        if (get_symbol(symbols, bits, i)) {
            result.push_back(i);
        }
    }
    return result;
}
// ----------------------------------------------------------------------------
std::vector<uint32_t> gen_test_sizes()
{
    std::vector<uint32_t> result;
    for (uint32_t n(0); n < 100; n += 1) {
        result.push_back(n);
    }
    for (uint32_t n(100); n < 500; n += 10) {
        result.push_back(n);
    }
    for (uint32_t n(500); n < 3000; n += 100) {
        result.push_back(n);
    }
    for (uint32_t n(3000); n < 15000; n += 1000) {
        result.push_back(n);
    }
    for (uint32_t n(15000); n < 50000; n += 5000) {
        result.push_back(n);
    }
    for (uint32_t n(50000); n <= 500000; n += 50000) {
        result.push_back(n);
    }
    return result;
}
// ----------------------------------------------------------------------------
uint32_t estimate_size(match_list_t const& matches)
{
    match_symbols_t symbols = make_symbols(matches, 1);

    std::map<uint8_t, uint32_t> alphabet;
    double entropy = calculate_entropy(symbols, alphabet);
    double best_size(entropy * NUM_VALUES);

    return static_cast<uint32_t>(std::ceil(best_size / 8.0));
}
// ============================================================================
class arithmetic_codec_v1
{
public:
    buffer_t compress(match_list_t const& matches)
    {
        uint32_t match_count(static_cast<uint32_t>(matches.size()));

        arithmetic_codec codec(static_cast<uint32_t>(NUM_VALUES / 4));
        codec.start_encoder();

        // Store the number of matches (1000000 needs only 20 bits)
        codec.put_bits(match_count, 20);

        if (match_count > 0) {
            // Initialize the model
            static_bit_model model;
            model.set_probability_0(get_probability_0(match_count));

            // Create a bitmap and code all the bitmap entries
            // NB: This is lazy and inefficient, but simple
            match_symbols_t symbols = make_symbols(matches, 1);
            for (auto entry : symbols) {
                codec.encode(entry, model);
            }
        }

        uint32_t compressed_size = codec.stop_encoder();
        return buffer_t(codec.buffer(), codec.buffer() + compressed_size);
    }

    match_list_t decompress(buffer_t& compressed)
    {
        arithmetic_codec codec(static_cast<uint32_t>(compressed.size()), &compressed[0]);
        codec.start_decoder();

        // Read number of matches (20 bits)
        uint32_t match_count(codec.get_bits(20));

        match_list_t result;
        if (match_count > 0) {
            static_bit_model model;
            model.set_probability_0(get_probability_0(match_count));

            result.reserve(match_count);
            for (uint32_t i(0); i < NUM_VALUES; ++i) {
                uint32_t entry = codec.decode(model);
                if (entry == 1) {
                    result.push_back(i);
                }
            }
        }

        codec.stop_decoder();
        return result;
    }

private:
    double get_probability_0(uint32_t match_count, uint32_t num_values = NUM_VALUES)
    {
        double probability_0(double(num_values - match_count) / num_values);
        // Limit probability to match FastAC limitations...
        return std::max(0.0001, std::min(0.9999, probability_0));
    }
};
// ----------------------------------------------------------------------------
class arithmetic_codec_v2
{
public:
    buffer_t compress(match_list_t const& matches)
    {
        uint32_t match_count(static_cast<uint32_t>(matches.size()));
        uint32_t total_count(NUM_VALUES);

        arithmetic_codec codec(static_cast<uint32_t>(NUM_VALUES / 4));
        codec.start_encoder();

        // Store the number of matches (1000000 needs only 20 bits)
        codec.put_bits(match_count, 20);

        if (match_count > 0) {
            static_bit_model model;

            // Create a bitmap and code all the bitmap entries
            // NB: This is lazy and inefficient, but simple
            match_symbols_t symbols = make_symbols(matches, 1);
            for (auto entry : symbols) {
                model.set_probability_0(get_probability_0(match_count, total_count));
                codec.encode(entry, model);
                --total_count;
                if (entry) {
                    --match_count;
                }
                if (match_count == 0) {
                    break;
                }
            }
        }

        uint32_t compressed_size = codec.stop_encoder();
        return buffer_t(codec.buffer(), codec.buffer() + compressed_size);
    }

    match_list_t decompress(buffer_t& compressed)
    {
        arithmetic_codec codec(static_cast<uint32_t>(compressed.size()), &compressed[0]);
        codec.start_decoder();

        // Read number of matches (20 bits)
        uint32_t match_count(codec.get_bits(20));
        uint32_t total_count(NUM_VALUES);

        match_list_t result;
        if (match_count > 0) {
            static_bit_model model;
            result.reserve(match_count);
            for (uint32_t i(0); i < NUM_VALUES; ++i) {
                model.set_probability_0(get_probability_0(match_count, NUM_VALUES - i));
                if (codec.decode(model) == 1) {
                    result.push_back(i);
                    --match_count;
                }
                if (match_count == 0) {
                    break;
                }
            }
        }

        codec.stop_decoder();
        return result;
    }

private:
    double get_probability_0(uint32_t match_count, uint32_t num_values = NUM_VALUES)
    {
        double probability_0(double(num_values - match_count) / num_values);
        // Limit probability to match FastAC limitations...
        return std::max(0.0001, std::min(0.9999, probability_0));
    }
};
// ----------------------------------------------------------------------------
class zlib_codec
{
public:
    zlib_codec(uint32_t bits_per_symbol) : bits_per_symbol(bits_per_symbol) {}

    buffer_t compress(match_list_t const& matches)
    {
        match_symbols_t symbols(make_symbols(matches, bits_per_symbol));

        z_stream defstream;
        defstream.zalloc = nullptr;
        defstream.zfree = nullptr;
        defstream.opaque = nullptr;

        deflateInit(&defstream, Z_BEST_COMPRESSION);
        size_t max_compress_size = deflateBound(&defstream, static_cast<uLong>(symbols.size()));

        buffer_t compressed(max_compress_size);

        defstream.avail_in = static_cast<uInt>(symbols.size());
        defstream.next_in = &symbols[0];
        defstream.avail_out = static_cast<uInt>(max_compress_size);
        defstream.next_out = &compressed[0];

        deflate(&defstream, Z_FINISH);
        deflateEnd(&defstream);

        compressed.resize(defstream.total_out);
        return compressed;
    }

    match_list_t decompress(buffer_t& compressed)
    {
        z_stream infstream;
        infstream.zalloc = nullptr;
        infstream.zfree = nullptr;
        infstream.opaque = nullptr;

        inflateInit(&infstream);

        match_symbols_t symbols(symbol_count(bits_per_symbol));

        infstream.avail_in = static_cast<uInt>(compressed.size());
        infstream.next_in = &compressed[0];
        infstream.avail_out = static_cast<uInt>(symbols.size());
        infstream.next_out = &symbols[0];

        inflate(&infstream, Z_FINISH);
        inflateEnd(&infstream);

        return make_matches(symbols, bits_per_symbol);
    }
private:
    uint32_t bits_per_symbol;
};
// ----------------------------------------------------------------------------
class bzip2_codec
{
public:
    bzip2_codec(uint32_t bits_per_symbol) : bits_per_symbol(bits_per_symbol) {}

    buffer_t compress(match_list_t const& matches)
    {
        match_symbols_t symbols(make_symbols(matches, bits_per_symbol));

        uint32_t compressed_size = symbols.size() * 2;
        buffer_t compressed(compressed_size);

        int err = BZ2_bzBuffToBuffCompress((char*)&compressed[0]
            , &compressed_size
            , (char*)&symbols[0]
            , symbols.size()
            , 9
            , 0
            , 30);
        if (err != BZ_OK) {
            throw std::runtime_error("Compression error.");
        }

        compressed.resize(compressed_size);
        return compressed;
    }

    match_list_t decompress(buffer_t& compressed)
    {
        match_symbols_t symbols(symbol_count(bits_per_symbol));

        uint32_t decompressed_size = symbols.size();
        int err = BZ2_bzBuffToBuffDecompress((char*)&symbols[0]
            , &decompressed_size
            , (char*)&compressed[0]
            , compressed.size()
            , 0
            , 0);
        if (err != BZ_OK) {
            throw std::runtime_error("Compression error.");
        }
        if (decompressed_size != symbols.size()) {
            throw std::runtime_error("Size mismatch.");
        }

        return make_matches(symbols, bits_per_symbol);
    }
private:
    uint32_t bits_per_symbol;
};
// ----------------------------------------------------------------------------
class snappy_codec
{
public:
    snappy_codec(uint32_t bits_per_symbol) : bits_per_symbol(bits_per_symbol) {}

    buffer_t compress(match_list_t const& matches)
    {
        match_symbols_t symbols(make_symbols(matches, bits_per_symbol));

        size_t compressed_size = snappy_max_compressed_length(symbols.size());
        buffer_t compressed(compressed_size);

        snappy_status err = snappy_compress((char const*)&symbols[0]
            , symbols.size()
            , (char*)&compressed[0]
            , &compressed_size);

        if (err != SNAPPY_OK) {
            throw std::runtime_error("Decompression error.");
        }

        compressed.resize(compressed_size);
        return compressed;
    }

    match_list_t decompress(buffer_t& compressed)
    {
        match_symbols_t symbols(symbol_count(bits_per_symbol));

        size_t decompressed_size(symbols.size());
        snappy_status err = snappy_uncompress((char const*)&compressed[0]
            , compressed.size()
            , (char*)&symbols[0]
            , &decompressed_size);

        if (err != SNAPPY_OK) {
            throw std::runtime_error("Decompression error.");
        }

        return make_matches(symbols, bits_per_symbol);
    }
private:
    uint32_t bits_per_symbol;
};
// ============================================================================
void run_estimate(uint32_t match_count)
{
    std::vector<uint32_t> est_size;

    uint32_t const ITER_COUNT(16);
    for (uint32_t i(0); i < ITER_COUNT; ++i) {
        match_list_t matches = make_random_matches(match_count);

        est_size.push_back(estimate_size(matches));
    }

    if (std::set<uint32_t>(est_size.begin(), est_size.end()).size() != 1) {
        throw std::runtime_error("This shoudn't happen.");
    }

    std::cout << match_count << "," << est_size.front() << "\n";
}
// ============================================================================
template<typename Codec>
void run_test(Codec& codec, uint32_t match_count)
{
    std::vector<uint32_t> comp_size;

    uint32_t const ITER_COUNT(16);
    for (uint32_t i(0); i < ITER_COUNT; ++i) {
        match_list_t matches = make_random_matches(match_count);

        buffer_t compressed = codec.compress(matches);
        comp_size.push_back(static_cast<uint32_t>(compressed.size()));

        match_list_t decompressed = codec.decompress(compressed);
        if (!(matches == decompressed)) {
            throw std::runtime_error("Codec error.");
        }
    }

    double sum = std::accumulate(comp_size.begin(), comp_size.end(), uint32_t(0));
    std::cout << match_count << "," << (sum / ITER_COUNT) << "\n";
}
// ============================================================================
void run_tests_zlib()
{
    zlib_codec codec(4);
    std::vector<uint32_t> test_sizes = gen_test_sizes();
    for (auto n : test_sizes) {        
        run_test(codec, n);
    }
}

void run_tests_bzip()
{
    bzip2_codec codec(4);
    std::vector<uint32_t> test_sizes = gen_test_sizes();
    for (auto n : test_sizes) {
        run_test(codec, n);
    }
}

void run_tests_snappy()
{
    snappy_codec codec(1);
    std::vector<uint32_t> test_sizes = gen_test_sizes();
    for (auto n : test_sizes) {
        run_test(codec, n);
    }
}

void run_tests_arith_v1()
{
    arithmetic_codec_v2 codec;
    std::vector<uint32_t> test_sizes = gen_test_sizes();
    for (auto n : test_sizes) {
        run_test(codec, n);
    }
}


int main()
{
    run_tests_arith_v1();


    return 0;
}
// ============================================================================
