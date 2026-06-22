#include "register_types.h"

#include "classes/navigation_domain.h"
#include "classes/domain_region_3d.h"
#ifdef TOOLS_ENABLED
#include "classes/domain_editor_plugin.h"
#endif
#include "classes/domain_agent_3d.h"
#include <godot_cpp/core/engine_ptrcall.hpp>

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_in_domain_movement_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(BspTree);
		GDREGISTER_CLASS(GridLookup);
		GDREGISTER_CLASS(NavigationDomain);
		GDREGISTER_RUNTIME_CLASS(DomainRegion3D);
		GDREGISTER_RUNTIME_CLASS(DomainAgent3D);
	} 
	#ifdef TOOLS_ENABLED
	else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_CLASS(DomainEditorControl);
		GDREGISTER_CLASS(DomainEditorPlugin);
		EditorPlugins::add_by_type<DomainEditorPlugin>();
	}
	#endif
}

void uninitialize_in_domain_movement_module(ModuleInitializationLevel p_level) {
	return;
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT in_domain_movement_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_in_domain_movement_module);
	init_obj.register_terminator(uninitialize_in_domain_movement_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}