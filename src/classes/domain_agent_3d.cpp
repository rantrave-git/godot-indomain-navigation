#include <godot_cpp/classes/world3d.hpp>

#include "domain_agent_3d.h"
#include "domain_region_3d.h"

using namespace godot;

DomainAgent3D::DomainAgent3D() : radius{ 1.0f }, flags{
    DOMAINAGENT_FLAG_OBSTACLE |
    DOMAINAGENT_FLAG_INDOMAIN |
    DOMAINAGENT_FLAG_MOVEABLE |
    DOMAINAGENT_FLAG_AUTOSNAP |
    DOMAINAGENT_FLAG_CLIPVELOCITY |
    DOMAINAGENT_FLAG_AUTOATTACH
} { }

void DomainAgent3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_radius", "radius"), &DomainAgent3D::set_radius);
    ClassDB::bind_method(D_METHOD("get_radius"), &DomainAgent3D::get_radius);
    ClassDB::bind_method(D_METHOD("set_flags", "flags"), &DomainAgent3D::set_flags);
    ClassDB::bind_method(D_METHOD("get_flags"), &DomainAgent3D::get_flags);
    ClassDB::bind_method(D_METHOD("set_gravity", "gravity"), &DomainAgent3D::set_gravity);
    ClassDB::bind_method(D_METHOD("get_gravity"), &DomainAgent3D::get_gravity);

    ClassDB::bind_method(D_METHOD("attach", "domain", "try_land"), &DomainAgent3D::attach);
    ClassDB::bind_method(D_METHOD("detach"), &DomainAgent3D::detach);
    ClassDB::bind_method(D_METHOD("teleport", "to"), &DomainAgent3D::teleport);
    ClassDB::bind_method(D_METHOD("go_to", "destination", "speed"), &DomainAgent3D::go_to);
    ClassDB::bind_method(D_METHOD("jump_towards", "direction"), &DomainAgent3D::jump_towards);
    ClassDB::bind_method(D_METHOD("get_runtime_index"), &DomainAgent3D::get_runtime_index);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius"), "set_radius", "get_radius");
    ADD_PROPERTY(PropertyInfo(
        Variant::INT, "flags", PROPERTY_HINT_FLAGS, "Obstacle,In-domain,Moveable,Manual,Autosnap,Clip velocity,Autoattach"
    ), "set_flags", "get_flags");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gravity"), "set_gravity", "get_gravity");

    // DOMAINAGENT_FLAG_OBSTACLE = 0x1, // 
    // DOMAINAGENT_FLAG_INDOMAIN = 0x2, // snaps to domain
    // DOMAINAGENT_FLAG_MOVEABLE = 0x4, // can be moved during agents' collision step
    // DOMAINAGENT_FLAG_MANUAL = 0x8, // use virtual `_request_motion` function for each frame movement
    // DOMAINAGENT_FLAG_AUTOSNAP = 0x16, // snap to close domain at landing
    // DOMAINAGENT_FLAG_CLIPVELOCITY = 0x32, // ensure that velocity never exceed the radius
    // DOMAINAGENT_FLAG_AUTOATTACH = 0x64, // attach to parent on start

    ::godot::ClassDB::bind_integer_constant(get_class_static(), ::godot::_gde_constant_get_bitfield_name(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_OBSTACLE, "DOMAINAGENT_FLAG_OBSTACLE"), "DOMAINAGENT_FLAG_OBSTACLE", DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_OBSTACLE, true);
    ::godot::ClassDB::bind_integer_constant(get_class_static(), ::godot::_gde_constant_get_bitfield_name(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_INDOMAIN, "DOMAINAGENT_FLAG_INDOMAIN"), "DOMAINAGENT_FLAG_INDOMAIN", DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_INDOMAIN, true);
    ::godot::ClassDB::bind_integer_constant(get_class_static(), ::godot::_gde_constant_get_bitfield_name(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_MOVEABLE, "DOMAINAGENT_FLAG_MOVEABLE"), "DOMAINAGENT_FLAG_MOVEABLE", DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_MOVEABLE, true);
    ::godot::ClassDB::bind_integer_constant(get_class_static(), ::godot::_gde_constant_get_bitfield_name(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_MANUAL, "DOMAINAGENT_FLAG_MANUAL"), "DOMAINAGENT_FLAG_MANUAL", DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_MANUAL, true);
    ::godot::ClassDB::bind_integer_constant(get_class_static(), ::godot::_gde_constant_get_bitfield_name(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_AUTOSNAP, "DOMAINAGENT_FLAG_AUTOSNAP"), "DOMAINAGENT_FLAG_AUTOSNAP", DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_AUTOSNAP, true);
    ::godot::ClassDB::bind_integer_constant(get_class_static(), ::godot::_gde_constant_get_bitfield_name(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_CLIPVELOCITY, "DOMAINAGENT_FLAG_CLIPVELOCITY"), "DOMAINAGENT_FLAG_CLIPVELOCITY", DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_CLIPVELOCITY, true);
    ::godot::ClassDB::bind_integer_constant(get_class_static(), ::godot::_gde_constant_get_bitfield_name(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_AUTOATTACH, "DOMAINAGENT_FLAG_AUTOATTACH"), "DOMAINAGENT_FLAG_AUTOATTACH", DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_AUTOATTACH, true);
    // BIND_BITFIELD_FLAG(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_OBSTACLE);
    // BIND_BITFIELD_FLAG(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_INDOMAIN);
    // BIND_BITFIELD_FLAG(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_MOVEABLE);
    // BIND_BITFIELD_FLAG(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_MANUAL);
    // BIND_BITFIELD_FLAG(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_AUTOSNAP);
    // BIND_BITFIELD_FLAG(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_CLIPVELOCITY);
    // BIND_BITFIELD_FLAG(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_AUTOATTACH);

    GDVIRTUAL_BIND(_request_motion, "delta", "is_landed");
}

void DomainAgent3D::set_radius(float value) {
    radius = value;
    if (domain != nullptr && index != -1) domain->update_radius(index, radius);
}

float DomainAgent3D::get_radius() const { return radius; }

void DomainAgent3D::set_flags(BitField<DomainAgent3D::AgentFlags> value) {
    flags = value;
    if (domain != nullptr && index != -1) domain->update_flags(index, flags);
}

BitField<DomainAgent3D::AgentFlags> DomainAgent3D::get_flags() const { return flags; }

void DomainAgent3D::set_gravity(const Vector3& value) { gravity = value; }

Vector3 DomainAgent3D::get_gravity() const { return gravity; }

void DomainAgent3D::attach(DomainRegion3D* to_domain, bool try_land) {
	TRACE(TRACE_DOMAINAGENT, "attach");
    domain = to_domain;
    index = domain->add_agent(this);
    if (index == -1) return;
    if (try_land) {
	    TRACE(TRACE_DOMAINAGENT, "landing..");
        domain->land(index);
	    TRACE(TRACE_DOMAINAGENT, "landed");
        auto pos = domain->get_agent_position(index);
        set_global_position(pos);
    }
}

void DomainAgent3D::detach() {
    if (domain == nullptr) return;
    domain->remove_agent(this);
    domain = nullptr;
    index = -1;
}

void DomainAgent3D::teleport(Vector3 to) {
    set_global_position(to);
    if (domain != nullptr && index != -1) domain->update_position(index, to);
}

void DomainAgent3D::go_to(Vector3 destination, float speed) {
    if (domain != nullptr && index != -1) {
        domain->land(index);
    }
    this->destination = destination;
    this->speed = speed;
    if (flags & DOMAINAGENT_FLAG_CLIPVELOCITY) {
        this->speed = MIN(this->speed, radius / get_physics_process_delta_time());
    }
    DEBUG(DEBUG_DOMAINAGENT, "Updated destination: ", destination, " with speed: ", speed);
}

void DomainAgent3D::jump_towards(Vector3 impulse) {
    domain->lift(index);
    velocity = impulse;
}

void DomainAgent3D::request_motion(float delta, Vector3& res, int node, Vector3& pos) {
    TRACE(TRACE_DOMAINAGENT, "requested motion");
    pos = get_global_position();
    if (flags.has_flag(DOMAINAGENT_FLAG_MANUAL)) {
        _gdvirtual__request_motion_call(delta, node == -1, res);
    }
    else {
        Vector3 dest = pos;
        if (node == -1 && flags.has_flag(DOMAINAGENT_FLAG_INDOMAIN)) { // in air
            velocity += delta * gravity;
            if (flags.has_flag(DOMAINAGENT_FLAG_CLIPVELOCITY)) {
                auto vl = velocity.length();
                if (vl > radius) {
                    velocity = velocity / vl * radius;
                }
            }
            if (flags.has_flag(DOMAINAGENT_FLAG_AUTOSNAP)) {
                Vector3 vec;
                if (domain->project(dest, radius, vec)) {
                    if (vec.dot(velocity) < 0.0f) { // move against the domain surface
                        vec = dest - pos;
                        if (vec.length_squared() < radius * radius) {
                            // move towards closest point at projected velocity
                            velocity = velocity.project(vec);
                            if (velocity.length_squared() * delta > vec.length_squared()) { // should land during this frame
                                domain->land(index);
                                res = vec;
                                return;
                            }
                        }
                    }
                }
            }
            res = velocity * delta;
        }
        else { // if not in-domain just move towards target
            if (speed < movement_threshold) { // beyond move threshold
                res = Vector3{};
                return;
            }
            auto s = speed * delta;
            res = destination - pos;

            auto len = res.length_squared();
            if (len < s * s) {
                destination = pos;
                speed = 0.0f;
                return; // end at the destination during this frame
            }
            res = res * (s / Math::sqrt(len));
        }
    }
}
