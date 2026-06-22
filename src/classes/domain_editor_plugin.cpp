#ifdef TOOLS_ENABLED
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

#include "domain_editor_plugin.h"
#include "domain_region_3d.h"

#define SNAME(m_arg) ([]() -> const StringName & { static StringName sname = StringName(m_arg, true); return sname; })()

godot::DomainEditorControl::DomainEditorControl()
{
	bake_hbox = memnew(HBoxContainer);

	button_bake = memnew(Button);
	button_bake->set_theme_type_variation("FlatButton");
    bake_hbox->add_child(button_bake);
	button_bake->set_toggle_mode(true);
	button_bake->set_text("Bake Domain");
    button_bake->set_tooltip_text("Bakes the NavigationDomain by taking polygons from inherited NabigationRegion.");
	button_bake->connect("pressed", 
        create_custom_callable_function_pointer(this, &DomainEditorControl::_bake_pressed));

	err_dialog = memnew(AcceptDialog);
	add_child(err_dialog);

	save_file_dialog = memnew(FileDialog);
	add_child(save_file_dialog);
	save_file_dialog->set_access(FileDialog::Access::ACCESS_RESOURCES);
	save_file_dialog->set_file_mode(FileDialog::FileMode::FILE_MODE_SAVE_FILE);
	save_file_dialog->add_filter("*.tres", "Text encoded resource");
	save_file_dialog->add_filter("*.res", "Binary encoded resource");
	save_file_dialog->connect("file_selected", create_custom_callable_function_pointer(this, &DomainEditorControl::_bake_confirmed));
}

void godot::DomainEditorControl::edit(NavigationRegion3D* domain)
{
    if (domain == nullptr) return;
    selected_domain = domain;
}

void godot::DomainEditorControl::_bake_confirmed(const godot::String& path) {
	Ref<NavigationDomain> domain;
	domain.instantiate();
	domain->create_from_navmesh(selected_domain->get_navigation_mesh().ptr());
	ResourceSaver::get_singleton()->save(domain, path, ResourceSaver::SaverFlags::FLAG_COMPRESS);
}

void godot::DomainEditorControl::_bake_pressed()
{
    if (selected_domain == nullptr) return;
	auto msh = selected_domain->get_navigation_mesh();
	if (msh.is_null()) {
        err_dialog->set_text("A NavigationDomain must have only one baked NavigationRegion3D child.");
        err_dialog->popup_centered();
		return;
	}
	save_file_dialog->set_text("Select NavigationDomain");
	if (save_file_dialog->get_current_file() == ""){
		save_file_dialog->set_current_file("new_navigation_domain.tres");
	}
	save_file_dialog->popup_centered();
}

void godot::DomainEditorControl::_node_removed(Node *p_node)
{
	NavigationRegion3D* region = Object::cast_to<NavigationRegion3D>(p_node);

    if(selected_domain == region) {
        selected_domain = nullptr;
        hide();
    }
}

void godot::DomainEditorControl::_notification(int p_what)
{
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			button_bake->set_button_icon(get_theme_icon(SNAME("Bake"), "EditorIcons"));
		} break;
	}
}

void godot::DomainEditorControl::_bind_methods() {}

void godot::DomainEditorPlugin::_edit(Object* p_object) {
    if (p_object == nullptr) return;
    NavigationRegion3D *domain = Object::cast_to<NavigationRegion3D>(p_object);
    if(domain) {
        domain_editor_control->edit(domain);
    }
}

bool godot::DomainEditorPlugin::_handles(Object* p_object) const {
	if (Object::cast_to<NavigationRegion3D>(p_object)) {
		return true;
	}
	return false;
}

void godot::DomainEditorPlugin::_make_visible(bool p_visible) {
	if (p_visible) {
		domain_editor_control->show();
		domain_editor_control->bake_hbox->show();
	} else {
		domain_editor_control->hide();
		domain_editor_control->bake_hbox->hide();
		domain_editor_control->edit(nullptr);
	}
}

godot::DomainEditorPlugin::DomainEditorPlugin() {
	domain_editor_control = memnew(DomainEditorControl);
    auto s = this->get_editor_interface()->get_base_control();
	auto ss = this->get_editor_interface();

	s->add_child(domain_editor_control);
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, domain_editor_control->bake_hbox);
	domain_editor_control->hide();
	domain_editor_control->bake_hbox->hide();

	// gizmo_plugin.instantiate();
	// Node3DEditor::get_singleton()->add_gizmo_plugin(gizmo_plugin);
}
#endif
