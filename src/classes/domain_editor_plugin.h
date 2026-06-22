#pragma once
#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/navigation_region3d.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/file_dialog.hpp>

#include "domain_agent_3d.h"

namespace godot {
class DomainEditorControl : public Control {
	// friend class DomainRegion3D;
    GDCLASS(DomainEditorControl, Control);

    friend class DomainEditorPlugin;
private:
    AcceptDialog* err_dialog = nullptr;
	HBoxContainer *bake_hbox = nullptr;
	FileDialog *save_file_dialog = nullptr;
    Button* button_bake = nullptr;
    NavigationRegion3D* selected_domain;
	void _bake_pressed();
	void _bake_confirmed(const godot::String& path);

protected:
	void _node_removed(Node *p_node);
	void _notification(int p_what);
	static void _bind_methods();
public:
    DomainEditorControl();
    // [TODO] make multinode editing
	void edit(NavigationRegion3D* domain);
};

class DomainEditorPlugin : public EditorPlugin {
	GDCLASS(DomainEditorPlugin, EditorPlugin);

	DomainEditorControl *domain_editor_control = nullptr;
    // [TODO] add gismo
	// Ref<DomainEditorGizmoPlugin> gizmo_plugin; 
public:
	virtual String _get_plugin_name() const override { return "NavigationDomain"; }
	bool _has_main_screen() const { return false; }
	virtual void _edit(Object *p_object) override;
	virtual bool _handles(Object *p_object) const override;
	virtual void _make_visible(bool p_visible) override;

	DomainEditorPlugin();
protected:
	static void _bind_methods() {}
};

}
#endif
