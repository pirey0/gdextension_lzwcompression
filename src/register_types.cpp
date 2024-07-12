#include "register_types.h"
#include <godot_cpp/core/class_db.hpp>
#include "lzw_gdextension.cpp"
#include "lzw_gdextension2.cpp"

void initialize_lzw_extension(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    ClassDB::register_class<LZWExtension>();
    ClassDB::register_class<LZWExtension2>();
}

void uninitialize_lzw_extension(ModuleInitializationLevel p_level) {
    // Optional cleanup
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT example_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_lzw_extension);
	init_obj.register_terminator(uninitialize_lzw_extension);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}