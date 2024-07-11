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

class LZWExtension : public Object {
    GDCLASS(LZWExtension, Object)

protected:
    static void _bind_methods() {
        ClassDB::bind_static_method("LZWExtension", D_METHOD("compress", "src", "color_buf"),&LZWExtension::compress);
    }

private:

struct PackedByteArrayHasher {
    int operator()(const PackedByteArray &V) const {
        int hash = V.size();
        for(auto &i : V) {
            hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

class LSBLZWBitPacker {
private:
    int bit_index = 0;
    int stream = 0;
    PackedByteArray chunks;

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
        return chunks;
    }

    void reset() {
        bit_index = 0;
        stream = 0;
        chunks.clear();
    }
};

class CodeTable {
private:
    std::unordered_map<PackedByteArray, int,PackedByteArrayHasher> lookup;

public:
    int counter = 0;

    int add(const PackedByteArray& entry) {
        lookup[entry] = counter;
        counter++;
        return counter;
    }

    int find(const PackedByteArray& entry) {
        auto it = lookup.find(entry);
        if (it != lookup.end()) {
            return it->second;
        }
        return -1;
    }

    bool has(const PackedByteArray& entry) {
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
        PackedByteArray b;
        b.append(color_id);
        result_code_table.add(b);
    }
    int last_color_index = colors.size() - 1;
    int clear_code_index = pow(2, get_bits_number_for(last_color_index));
    
    result_code_table.counter = clear_code_index + 2;
    return result_code_table;
}

static Array compress_lzw(PackedByteArray image, const Array& colors) {
    CodeTable code_table = initialize_color_code_table(colors);
    
    int last_color_index = colors.size() - 1;
    int clear_code_index = pow(2, get_bits_number_for(last_color_index));
    PackedByteArray index_stream(image);
    int current_code_size = get_bits_number_for(clear_code_index);
    LSBLZWBitPacker binary_code_stream;

    binary_code_stream.write_bits(clear_code_index, get_bits_number_for(clear_code_index));
    
    PackedByteArray index_buffer = {index_stream[0]};
    int data_index = 1;

    while (data_index < index_stream.size()) {
        int k = index_stream.decode_s64(data_index);
        data_index ++;
        PackedByteArray new_index_buffer = {index_buffer};
        new_index_buffer.push_back(k);

        if (code_table.has(new_index_buffer)) {
            index_buffer = new_index_buffer;
        } 
        else {
            binary_code_stream.write_bits(code_table.find(index_buffer), current_code_size);

            int last_entry_index = code_table.counter - 1;
            if (last_entry_index != 4095) {
                code_table.add(new_index_buffer);
            }
            else{
                binary_code_stream.write_bits(clear_code_index, current_code_size);
                code_table = initialize_color_code_table(colors);
                current_code_size = get_bits_number_for(clear_code_index);
            }

            int new_code_size_candidate = get_bits_number_for(code_table.counter - 1);
			if (new_code_size_candidate > current_code_size){
				current_code_size = new_code_size_candidate;
            }
            
            PackedByteArray b;
            b.append(k);
            index_buffer = b;
        }
    }

    binary_code_stream.write_bits(code_table.find(index_buffer), current_code_size);
    binary_code_stream.write_bits(clear_code_index + 1, current_code_size);

    //binary_code_stream.pack();
    //int min_code_size = get_bits_number_for(clear_code_index) - 1;
    //binary_code_stream.write_bits(min_code_size, 64);
    return binary_code_stream.pack();
}

public:
    static PackedByteArray compress(PackedByteArray src, PackedByteArray color_buf) {
        return compress_lzw(src, color_buf);
    }
};
}