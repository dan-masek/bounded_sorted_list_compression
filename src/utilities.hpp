#pragma once
// ============================================================================
#include <cstdint>
#include <map>
#include <memory>
#include <vector>
// ============================================================================
template <typename T>
std::map<T, uint32_t> create_alphabet(std::vector<T> const& buf)
{
    std::map<T, uint32_t> alphabet;
    create_alphabet(buf, alphabet);
    return alphabet;
}
// ----------------------------------------------------------------------------
template <typename T>
double create_alphabet(std::vector<T> const& buf
    , std::map<T, uint32_t>& alphabet)
{
    alphabet.clear();
    for (auto c : buf) {
        if (alphabet.find(c) != alphabet.end()) {
            ++alphabet[c];
        } else {
            alphabet[c] = 1;
        }
    }
    return static_cast<double>(buf.size());
}
// ============================================================================
template <typename T>
double calculate_entropy(std::vector<T> const& buf)
{
    std::map<T, uint32_t> alphabet;
    return calculate_entropy(buf, alphabet);
}
// ----------------------------------------------------------------------------
template <typename T>
double calculate_entropy(std::map<T, uint32_t>& alphabet)
{
    double t(0.0);
    for (auto& f : alphabet) {
        t += f.second;
    }
    return calculate_entropy(alphabet, t);
}
// ----------------------------------------------------------------------------
template <typename T>
double calculate_entropy(std::map<T, uint32_t>& alphabet, double t)
{
    double entropy(0.0);
    for (auto& f : alphabet) {
        if (f.second == 0) continue;
        double r(double(f.second) / t);
        static double const log_2(std::log10(2.0));
        entropy += -r * (std::log10(r) / log_2);
    }
    return entropy;
}
// ----------------------------------------------------------------------------
template <typename T>
double calculate_entropy(std::vector<T> const& buf
    , std::map<T, uint32_t>& alphabet)
{
    double t(create_alphabet(buf, alphabet));
    return calculate_entropy(alphabet, t);
}
// ============================================================================
template <typename T>
void dump_alphabet(std::map<T, uint32_t>& alphabet)
{
    for (auto& f : alphabet) {
        std::cout << int16_t(f.first) << "\t" << f.second << "\n";
    }
    std::cout << "\nUnique symbols = " << alphabet.size() << "\n";
}
// ============================================================================
template<typename ValueT
    , typename ModelT
    , typename CodecT = arithmetic_codec>
inline void encode_symbol(ValueT value
    , ModelT& model
    , CodecT& encoder)
{
    encoder.encode(value, model);
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT_O0
    , typename ModelT_ONeg1
    , typename CodecT = arithmetic_codec>
inline void encode_symbol(ValueT value
    , ModelT_O0& model
    , ModelT_ONeg1& model_neg1
    , CodecT& encoder)
{
    if (model.has_symbol(value)) {
        encoder.encode(value, model);
    } else {
        encoder.encode(model.get_escape(), model);
        model.add_symbol(value);

        encode_symbol(value, model_neg1, encoder);
    }
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT_O1
    , typename ModelT_O0
    , typename ModelT_ONeg1
    , typename CodecT = arithmetic_codec>
    inline void encode_symbol(ValueT value
        , std::vector<ValueT>& context
        , std::vector<std::unique_ptr<ModelT_O1>>& model_o1
        , ModelT_O0& model_o0
        , ModelT_ONeg1& model_neg1
        , CodecT& encoder)
{
    uint32_t const symbol_count(model_neg1.model_symbols());

    uint32_t const index_c0(static_cast<uint32_t>(context.size() - 1));
    uint32_t ctx(context[index_c0]);

    if (!model_o1[ctx]) {
        std::cout << "New context L1 " << ctx << "\n";
        model_o1[ctx] = std::make_unique<ModelT_O1>(symbol_count);
    }
    ModelT_O1& current_model(*model_o1[ctx]);

    if (current_model.has_symbol(value)) {
        encoder.encode(value, current_model);
    } else {
        encoder.encode(current_model.get_escape(), current_model);
        current_model.add_symbol(value);

        encode_symbol(value
            , model_o0
            , model_neg1
            , encoder);
    }
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT_O2
    , typename ModelT_O1
    , typename ModelT_O0
    , typename ModelT_ONeg1
    , typename CodecT = arithmetic_codec>
    inline void encode_symbol(ValueT value
        , std::vector<ValueT>& context
        , std::vector<std::unique_ptr<ModelT_O2>>& model_o2
        , std::vector<std::unique_ptr<ModelT_O1>>& model_o1
        , ModelT_O0& model_o0
        , ModelT_ONeg1& model_neg1
        , CodecT& encoder)
{
    uint32_t const symbol_count(model_neg1.model_symbols());

    uint32_t const index_c0(static_cast<uint32_t>(context.size() - 1));
    uint32_t ctx(context[index_c0]);
    ctx = ctx * symbol_count + context[index_c0 - 1];

    if (!model_o2[ctx]) {
        std::cout << "New context L2 " << ctx
            << " (...," << uint16_t(context[index_c0 - 1])
            << "," << uint16_t(context[index_c0])<< ")\n";
        model_o2[ctx] = std::make_unique<ModelT_O2>(symbol_count);
    }
    ModelT_O2& current_model(*model_o2[ctx]);

    if (current_model.has_symbol(value)) {
        encoder.encode(value, current_model);
    } else {
        encoder.encode(current_model.get_escape(), current_model);
        current_model.add_symbol(value);

        encode_symbol(value
            , context
            , model_o1
            , model_o0
            , model_neg1
            , encoder);
    }
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT_O3
    , typename ModelT_O2
    , typename ModelT_O1
    , typename ModelT_O0
    , typename ModelT_ONeg1
    , typename CodecT = arithmetic_codec>
    inline void encode_symbol(ValueT value
        , std::vector<ValueT>& context
        , std::vector<std::unique_ptr<ModelT_O3>>& model_o3
        , std::vector<std::unique_ptr<ModelT_O2>>& model_o2
        , std::vector<std::unique_ptr<ModelT_O1>>& model_o1
        , ModelT_O0& model_o0
        , ModelT_ONeg1& model_neg1
        , CodecT& encoder)
{
    uint32_t const symbol_count(model_neg1.model_symbols());

    uint32_t const index_c0(static_cast<uint32_t>(context.size() - 1));
    uint32_t ctx(context[index_c0]);
    ctx = ctx * symbol_count + context[index_c0 - 1];
    ctx = ctx * symbol_count + context[index_c0 - 2];

    if (!model_o3[ctx]) {
        std::cout << "New context L3 " << ctx
            << " (...," << uint16_t(context[index_c0 - 2])
            << "," << uint16_t(context[index_c0 - 1])
            << "," << uint16_t(context[index_c0]) << ")\n";
        model_o3[ctx] = std::make_unique<ModelT_O3>(symbol_count);
    }
    ModelT_O3& current_model(*model_o3[ctx]);

    if (current_model.has_symbol(value)) {
        encoder.encode(value, current_model);
    } else {
        encoder.encode(current_model.get_escape(), current_model);
        current_model.add_symbol(value);

        encode_symbol(value
            , context
            , model_o2
            , model_o1
            , model_o0
            , model_neg1
            , encoder);
    }
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT_O4
    , typename ModelT_O3
    , typename ModelT_O2
    , typename ModelT_O1
    , typename ModelT_O0
    , typename ModelT_ONeg1
    , typename CodecT = arithmetic_codec>
    inline void encode_symbol(ValueT value
        , std::vector<ValueT>& context
        , std::vector<std::unique_ptr<ModelT_O4>>& model_o4
        , std::vector<std::unique_ptr<ModelT_O3>>& model_o3
        , std::vector<std::unique_ptr<ModelT_O2>>& model_o2
        , std::vector<std::unique_ptr<ModelT_O1>>& model_o1
        , ModelT_O0& model_o0
        , ModelT_ONeg1& model_neg1
        , CodecT& encoder)
{
    uint32_t const symbol_count(model_neg1.model_symbols());

    uint32_t const index_c0(static_cast<uint32_t>(context.size() - 1));
    uint32_t ctx(context[index_c0]);
    ctx = ctx * symbol_count + context[index_c0 - 1];
    ctx = ctx * symbol_count + context[index_c0 - 2];
    ctx = ctx * symbol_count + context[index_c0 - 3];

    if (!model_o4[ctx]) {
        std::cout << "New context L4 " << ctx
            << " (...," << uint16_t(context[index_c0 - 3])
            << "," << uint16_t(context[index_c0 - 2])
            << "," << uint16_t(context[index_c0 - 1])
            << "," << uint16_t(context[index_c0]) << ")\n";
        model_o4[ctx] = std::make_unique<ModelT_O4>(symbol_count);
    }
    ModelT_O4& current_model(*model_o4[ctx]);

    if (current_model.has_symbol(value)) {
        encoder.encode(value, current_model);
    } else {
        encoder.encode(current_model.get_escape(), current_model);
        current_model.add_symbol(value);

        encode_symbol(value
            , context
            , model_o3
            , model_o2
            , model_o1
            , model_o0
            , model_neg1
            , encoder);
    }
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT_O5
    , typename ModelT_O4
    , typename ModelT_O3
    , typename ModelT_O2
    , typename ModelT_O1
    , typename ModelT_O0
    , typename ModelT_ONeg1
    , typename CodecT = arithmetic_codec>
    inline void encode_symbol(ValueT value
        , std::vector<ValueT>& context
        , std::vector<std::unique_ptr<ModelT_O5>>& model_o5
        , std::vector<std::unique_ptr<ModelT_O4>>& model_o4
        , std::vector<std::unique_ptr<ModelT_O3>>& model_o3
        , std::vector<std::unique_ptr<ModelT_O2>>& model_o2
        , std::vector<std::unique_ptr<ModelT_O1>>& model_o1
        , ModelT_O0& model_o0
        , ModelT_ONeg1& model_neg1
        , CodecT& encoder)
{
    uint32_t const symbol_count(model_neg1.model_symbols());

    uint32_t const index_c0(static_cast<uint32_t>(context.size() - 1));
    uint32_t ctx(context[index_c0]);
    ctx = ctx * symbol_count + context[index_c0 - 1];
    ctx = ctx * symbol_count + context[index_c0 - 2];
    ctx = ctx * symbol_count + context[index_c0 - 3];
    ctx = ctx * symbol_count + context[index_c0 - 4];

    if (!model_o5[ctx]) {
        std::cout << "New context L5 " << ctx
            << " (...," << uint16_t(context[index_c0 - 4])
            << "," << uint16_t(context[index_c0 - 3])
            << "," << uint16_t(context[index_c0 - 2])
            << "," << uint16_t(context[index_c0 - 1])
            << "," << uint16_t(context[index_c0]) << ")\n";
        model_o5[ctx] = std::make_unique<ModelT_O5>(symbol_count);
    }
    ModelT_O5& current_model(*model_o5[ctx]);

    if (current_model.has_symbol(value)) {
        encoder.encode(value, current_model);
    } else {
        encoder.encode(current_model.get_escape(), current_model);
        current_model.add_symbol(value);

        encode_symbol(value
            , context
            , model_o4
            , model_o3
            , model_o2
            , model_o1
            , model_o0
            , model_neg1
            , encoder);
    }
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT
    , typename CodecT = arithmetic_codec>
inline void decode_symbol(ValueT& value
    , ModelT& model
    , CodecT& decoder)
{
    value = decoder.decode(model);
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT_O0
    , typename ModelT_ONeg1
    , typename CodecT = arithmetic_codec>
void decode_symbol(ValueT& value
    , ModelT_O0& model
    , ModelT_ONeg1& model_neg1
    , CodecT& decoder)
{
    unsigned temp_value(decoder.decode(model));
    if (model.is_escape(temp_value)) {
        temp_value = decoder.decode(model_neg1);
        model.add_symbol(temp_value);
    }
    value = temp_value;
}
// ============================================================================
template<typename ValueT
    , typename ModelT
    , typename CodecT = arithmetic_codec>
uint32_t encode(std::vector<ValueT> const& buffer
    , ModelT& model
    , CodecT& encoder)
{
    encoder.start_encoder();
    for (ValueT v : buffer) {
        encode_symbol(v, model, encoder);
    }
    return 8 * encoder.stop_encoder();
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT_O0
    , typename ModelT_ONeg1
    , typename CodecT = arithmetic_codec>
uint32_t encode(std::vector<ValueT> const& buffer
    , ModelT_O0& model
    , ModelT_ONeg1& model_neg1
    , CodecT& encoder)
{
    encoder.start_encoder();
    for (ValueT v : buffer) {
        encode_symbol(v, model, model_neg1, encoder);
    }
    return 8 * encoder.stop_encoder();
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT
    , typename CodecT = arithmetic_codec>
void decode(std::vector<ValueT>& buffer
    , ModelT& model
    , CodecT& decoder)
{
    decoder.start_decoder();
    for (size_t i(0); i < buffer.size(); ++i) {
        decode_symbol(buffer[i], model, decoder);
    }
    decoder.stop_decoder();
}
// ----------------------------------------------------------------------------
template<typename ValueT
    , typename ModelT_O0
    , typename ModelT_ONeg1
    , typename CodecT = arithmetic_codec>
void decode(std::vector<ValueT>& buffer
    , ModelT_O0& model
    , ModelT_ONeg1& model_neg1
    , CodecT& decoder)
{
    decoder.start_decoder();
    for (size_t i(0); i < buffer.size(); ++i) {
        decode_symbol(buffer[i], model, model_neg1, decoder);
    }
    decoder.stop_decoder();
}
// ============================================================================
