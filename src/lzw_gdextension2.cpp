#define LZW_EDDY_IMPLEMENTATION

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <vector>
#include <unordered_map>
#include <cmath>

using namespace godot;

namespace godot{

class LZWExtension2 : public Object {
    GDCLASS(LZWExtension2, Object)

protected:
    static void _bind_methods() {
        ClassDB::bind_static_method("LZWExtension", D_METHOD("compress", "src", "color_buf"),&LZWExtension2::compress);
    }

private:

struct VectorHasher {
    int operator()(const std::vector<uint8_t> &V) const {
        int hash = V.size();
        for(auto &i : V) {
            hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

class BitPacker {
public:
    int bit_index = 0;
    int stream = 0;
    std::vector<uint8_t> chunks;

    BitPacker(int size) {
        chunks.reserve(size);
    }

    void put_byte() {
        chunks.push_back(stream & 0xff);
        bit_index -= 8;
        stream >>= 8;
    }

public:
    void write_bits(int value, int bits_count) {
        value &= (1 << bits_count) - 1;
        value <<= bit_index;
        stream |= value;
        bit_index += bits_count;
        while (bit_index >= 8) {
            put_byte();
        }
    }

    PackedByteArray pack() {
        if (bit_index != 0) {
            put_byte();
        }
        chunks.shrink_to_fit();
        PackedByteArray result;
        result.resize(chunks.size());
        std::copy(chunks.begin(), chunks.end(), result.ptrw());
        return result;
    }

    void reset() {
        bit_index = 0;
        stream = 0;
        chunks.clear();
    }
};

class CodeTable {
public:
    std::unordered_map<std::vector<uint8_t>, int,VectorHasher> lookup;

public:
    int counter = 0;

    int add(const std::vector<uint8_t>& entry) {
        lookup[entry] = counter;
        counter++;
        return counter;
    }

    int find(const std::vector<uint8_t>& entry) {
        auto it = lookup.find(entry);
        if (it != lookup.end()) {
            return it->second;
        }
        return -1;
    }

    bool has(const std::vector<uint8_t>& entry) {
        return lookup.find(entry) != lookup.end();
    }
};

static float log2(float value) {
    return log(value) / log(2.0);
}

static int get_bits_number_for(int value) {
    if (value == 0) {
        return 1;
    }
    return static_cast<int>(ceil(log2(value + 1)));
}

static CodeTable initialize_color_code_table(const Array& colors) {
    CodeTable result_code_table;

    for (auto x = 0; x < colors.size(); ++x){
        auto color_id = colors[x];
        std::vector<uint8_t> b;
        b.push_back(color_id);
        result_code_table.add(b);
    }
    int last_color_index = colors.size() - 1;
    int clear_code_index = pow(2, get_bits_number_for(last_color_index));
    
    result_code_table.counter = clear_code_index + 2;
    return result_code_table;
}


static Dictionary compress_lzw(PackedByteArray index_stream, const Array& colors) {
    CodeTable code_table = initialize_color_code_table(colors);    
    int clear_code_index = pow(2, get_bits_number_for(colors.size() - 1));
    int current_code_size = get_bits_number_for(clear_code_index);
    const int size = index_stream.size();
    BitPacker binary_code_stream(size);

    binary_code_stream.write_bits(clear_code_index, get_bits_number_for(clear_code_index));
    
    std::vector<uint8_t> index_buffer = {index_stream[0]};
    int data_index = 1;

    while (data_index < size) {    
        uint8_t k = index_stream[data_index++];
        index_buffer.push_back(k);

        if (code_table.has(index_buffer)) {
            continue;
        } 

        index_buffer.pop_back();
        binary_code_stream.write_bits(code_table.find(index_buffer), current_code_size);

        if (code_table.counter == 4096) {
            //clear table
            binary_code_stream.write_bits(clear_code_index, current_code_size);
            code_table = initialize_color_code_table(colors);
            current_code_size = get_bits_number_for(clear_code_index);
        }
        else{
            index_buffer.push_back(k);
            code_table.add(index_buffer);
        }

        current_code_size = MAX(current_code_size,get_bits_number_for(code_table.counter - 1));
        
        index_buffer = {k};
    }

    binary_code_stream.write_bits(code_table.find(index_buffer), current_code_size);
    binary_code_stream.write_bits(clear_code_index + 1, current_code_size);

    int min_code_size = get_bits_number_for(clear_code_index) - 1;
    Dictionary out;
    out[String("min_code_size")] = min_code_size;
    out[String("stream")] = binary_code_stream.pack();
    return out;
}

public:
    static Dictionary compress(PackedByteArray src, PackedByteArray color_buf) {
        return compress_lzw(src, color_buf);
    }
};
}