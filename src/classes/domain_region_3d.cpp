#include "domain_region_3d.h"
#include <godot_cpp/classes/node.hpp>

#include "math_constants.h"

using namespace godot;

void DomainRegion3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_navigation_mesh", "mesh"), &DomainRegion3D::set_navigation_mesh);
	ClassDB::bind_method(D_METHOD("get_navigation_mesh"), &DomainRegion3D::get_navigation_mesh);
	ClassDB::bind_method(D_METHOD("set_navigation_domain", "domain"), &DomainRegion3D::set_navigation_domain);
	ClassDB::bind_method(D_METHOD("get_navigation_domain"), &DomainRegion3D::get_navigation_domain);
	ClassDB::bind_method(D_METHOD("set_collision_relaxation", "domain"), &DomainRegion3D::set_collision_relaxation);
	ClassDB::bind_method(D_METHOD("get_collision_relaxation"), &DomainRegion3D::get_collision_relaxation);
	ClassDB::bind_method(D_METHOD("set_collision_iterations", "domain"), &DomainRegion3D::set_collision_iterations);
	ClassDB::bind_method(D_METHOD("get_collision_iterations"), &DomainRegion3D::get_collision_iterations);

	// ClassDB::bind_method(D_METHOD("build_domain", "navmesh"), &DomainRegion3D::build_domain);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "NavigationMesh"), "set_navigation_mesh", "get_navigation_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "domain", PROPERTY_HINT_RESOURCE_TYPE, "NavigationDomain"), "set_navigation_domain", "get_navigation_domain");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "collision_relaxation"), "set_collision_relaxation", "get_collision_relaxation");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_iterations"), "set_collision_iterations", "get_collision_iterations");
}

void DomainRegion3D::resize(int new_size) {
	DEBUG(DEBUG_DOMAINREGION, "resize ", agents_data.size(), " to ", new_size);

	free_slots.reserve(new_size);
	auto old_size = agents_data.size();
	agents_data.resize(new_size);
#ifndef INDOMAIN_SEQUENTIAL_ITERATION
	agents_list.resize(new_size);
#endif
	for (int i{ (int)old_size }; i < agents_data.size(); ++i) free_slots.push_back(i);
	domain->extend_grid(new_size);
}

void DomainRegion3D::update_position(int agent_index, Vector3 position) {
	auto& a = agents_data[agent_index];
	// position = inv_transform.xform(position);
	if (a.flags & DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_INDOMAIN) {
		auto s = domain->project(position, a.radius);
		a.pos = s.first;
		a.node = s.second;
	}
	else {
		a.pos = position;
	}
}

void DomainRegion3D::update_motion(int agent_index, Vector3 motion) {
	auto& a = agents_data[agent_index];
	a.motion = motion;
}

void DomainRegion3D::update_radius(int agent_index, float radius) {
	agents_data[agent_index].radius = radius;
}

void DomainRegion3D::update_flags(int agent_index, BitField<DomainAgent3D::AgentFlags> flags) {
	agents_data[agent_index].flags = (DomainAgent3D::AgentFlags)(int64_t)flags;
}

bool DomainRegion3D::is_landed(int agent_index) const { return agents_data[agent_index].node == -1; }

Vector3 DomainRegion3D::normal(int agent_index) const {
	auto n = agents_data[agent_index].node;
	if (n == -1) return Vector3{ 0.0f, 1.0f, 0.0f };
	return domain->normal(n);
}

int DomainRegion3D::add_agent(DomainAgent3D* agent) {
	TRACE(TRACE_DOMAINREGION, "add_agent");
	if (domain.is_null() || !domain->is_initialized()) return -1;
	if (agent == nullptr) return -1;
	TRACE(TRACE_DOMAINREGION, " agent index: ", agent->index);
	if (agent->index != -1) return -1;
	// [FIXME] not thread safe!
	TRACE(TRACE_DOMAINREGION, " resizing...");
	if (free_slots.size() == 0) {
		resize((int)(agents_data.size() * 1.25f) + 16);
	}
	TRACE(TRACE_DOMAINREGION, " inserting...");
	auto index = free_slots.back();
	free_slots.pop_back();
	agents_data[index] = AgentData{
		agent->get_global_position(),
		Vector3{},
		Vector3{},
		agent->radius,
		0.0f,
		(DomainAgent3D::AgentFlags)(int64_t)agent->flags,
		-1,
		agent
	};
#ifndef INDOMAIN_SEQUENTIAL_ITERATION
	// push to the front
	if (agents_list_head == -1) {
		agents_list_head = index;
		agents_list[index] = { -1, -1 };
	}
	else {
		agents_list[index] = { -1, agents_list_head };
		agents_list[agents_list_head].prev = index;
		agents_list_head = index;
	}
#endif
	return index;
}

void DomainRegion3D::remove_agent(DomainAgent3D* agent) {
	if (domain.is_null() || !domain->is_initialized()) return;
	if (agent == nullptr) return;
	if (agent->index == -1) return; // not attached
	if (agent->domain != this) return; // attached to different domain
	auto index = agent->index;
	if (index >= agents_data.size()) return; // failsafe
	agents_data[agent->index].agent = nullptr;
	free_slots.push_back(index);
#ifndef INDOMAIN_SEQUENTIAL_ITERATION
	auto& ref = agents_list[index];
	if (ref.next == ref.prev) { // -1
		// remove the only element
		agents_list_head = -1;
		return;
	}
	if (ref.prev != -1) {
		agents_list[ref.prev].next = ref.next;
		ref.prev = -1;
	}
	else {
		// is heading element
		agents_list_head = ref.next;
	}
	if (ref.next != -1) {
		agents_list[ref.next].prev = ref.prev;
		ref.next = -1;
	}
#endif
}

void DomainRegion3D::land(int index) {
	if (index == -1 || index >= agents_data.size()) return;
	auto& a = agents_data[index];
	if (a.node != -1) return;
	if (a.agent->domain != this) return;
	TRACE(TRACE_DOMAINREGION, "Land ", index);
	auto r = domain->project(a.pos, a.radius);
	TRACE(TRACE_DOMAINREGION, " projected to ", r.second, " at ", r.first);
	a.pos = r.first;
	a.node = r.second;
}

void DomainRegion3D::lift(int index) {
	if (index == -1 || index >= agents_data.size()) return;
	auto& a = agents_data[index];
	if (a.agent->domain != this) return;
	a.node = -1;
}

bool DomainRegion3D::project(Vector3& point, float radius, Vector3& normal) const {
	TRACE(TRACE_DOMAINREGION, "Projecting ", point, " within ", radius);
	auto r = domain->project(point, radius);
	TRACE(TRACE_DOMAINREGION, " projected to ", r.second, " at ", r.first);
	if (r.second == -1) return false;
	point = r.first;
	normal = domain->normal(r.second);
	return true;
}

void DomainRegion3D::set_navigation_mesh(const Ref<NavigationMesh>& p_navigation_mesh) {
	mesh = p_navigation_mesh;
}

Ref<NavigationMesh> DomainRegion3D::get_navigation_mesh() const { return mesh; }

void DomainRegion3D::set_navigation_domain(const Ref<NavigationDomain>& p_navigation_domain) {
	domain = p_navigation_domain;
}

Ref<NavigationDomain> DomainRegion3D::get_navigation_domain() const { return domain; }

void godot::DomainRegion3D::set_collision_relaxation(float value) { collision_relaxation = value; }

float godot::DomainRegion3D::get_collision_relaxation() const { return collision_relaxation; }

void godot::DomainRegion3D::set_collision_iterations(int value) { collision_iterations = value; }

int godot::DomainRegion3D::get_collision_iterations() const { return collision_iterations; }

void DomainRegion3D::_ready() {
	if (domain.is_null() || !domain->is_initialized()) {
		DEBUG(DEBUG_DOMAINREGION, "Uninitialized");
		return;
	}
	domain->print();
	// inv_transform = get_global_transform().inverse();
	// set_process_priority(-1);
	domain->init_grid(4096);
	resize(4096);
	DEBUG(DEBUG_DOMAINREGION, "Domain initialized");

	auto c = find_children("*", "DomainAgent3D", true, true);
	DEBUG(DEBUG_DOMAINREGION, "Initializing agents ", c.size());
	for (int i{}; i < c.size(); ++i) {
		auto a = Object::cast_to<DomainAgent3D>(c[i].get_validated_object());
		if (!a->is_visible_in_tree()) continue;
		if (a->flags.has_flag(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_AUTOATTACH)) {
			a->attach(this, a->flags.has_flag(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_AUTOSNAP));
		}
	}
	DEBUG(DEBUG_DOMAINREGION, "Agents initialized");
}

void DomainRegion3D::_exit_tree() {
	for (int i = 0; i < agents_data.size(); ++i) {
		auto& a = agents_data[i];
		if (a.agent == nullptr) continue;
		if (a.agent->flags.has_flag(DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_AUTOATTACH)) {
			a.agent->detach();
		}
	}
}

void DomainRegion3D::_physics_process(double delta) {
	if (domain.is_null() || !domain->is_initialized()) return;
	// inv_transform = get_global_transform().inverse();
	auto grid = domain->get_grid_lookup().ptr();
	// UtilityFunctions::print("process...");
	if (collision_iterations == 0) {
#ifdef INDOMAIN_SEQUENTIAL_ITERATION
		for (int i = 0; i < agents_data.size(); ++i)
#else
		for (int i = agents_list_head; i != -1; i = agents_list[i].next)
#endif
		{
			auto& a = agents_data[i];
			if (a.agent == nullptr) {
				grid->remove(i);
				continue;
			}
			a.step = 0.0f;
			a.agent->request_motion(delta, a.motion, a.node, a.pos);
			if (a.flags & DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_INDOMAIN && a.node != -1) { // indomain and landed
				auto n = a.node;
				a.node = domain->motion(a.node, a.pos, a.motion, a.motion.length());
			}
			else {
				a.pos += a.motion;
			}
			if (!std::isfinite(a.pos.x) || !std::isfinite(a.pos.y) || !std::isfinite(a.pos.z)) {
				UtilityFunctions::print("motion agent[", i, "].pos = ", a.pos);
			}
			grid->add_or_update(i, a.pos);
			a.motion = Vector3{};
		}
	}
	else {
		// do collision handling
#ifdef INDOMAIN_SEQUENTIAL_ITERATION
		for (int i = 0; i < agents_data.size(); ++i)
#else
		for (int i = agents_list_head; i != -1; i = agents_list[i].next)
#endif
		{
			auto& a = agents_data[i];
			if (a.agent == nullptr) {
				grid->remove(i);
				continue;
			}
			a.agent->request_motion(delta, a.motion, a.node, a.pos);
			a.step = a.motion.length();
			if (a.step > DomainAgent3D::movement_threshold) a.motion /= a.step;
			grid->add_or_update(i, a.pos);
		}
		for (int _iters = 0; _iters < collision_iterations; ++_iters) {
#ifdef INDOMAIN_TRACE_STATS
			int num_agents = 0, num_collisions = 0;
#endif
			for (int i = 0; i < agents_data.size(); ++i) {
				auto& a = agents_data[i];
				if (a.agent == nullptr) continue;
				if (a.step < DomainAgent3D::movement_threshold) continue;
				float mv = Math::min(a.step, 2 * a.radius);
				a.step -= mv;
#ifdef INDOMAIN_TRACE_STATS
				num_agents++;
#endif
				if (a.flags & DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_INDOMAIN && a.node != -1) { // indomain and landed
					auto n = a.node;
					a.node = domain->motion(a.node, a.pos, a.motion, mv);
				}
				else {
					a.pos += a.motion * mv;
				}
				a.correction = Vector3{};
			}

			grid->iterate_all_pairs(2, [&](int cur, int oth) {
				auto& ca = agents_data[cur];
				auto& oa = agents_data[oth];
				auto col_case = (
					((ca.flags & DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_OBSTACLE) << 1)
					| (oa.flags & DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_OBSTACLE)
					) >> DomainAgent3D::AgentFlagsShift::DOMAINAGENT_FLAGSHIFT_OBSTACLE;
				auto mov_case = (
					(ca.flags & DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_MOVEABLE)
					| ((oa.flags & DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_MOVEABLE) << 1)
					) >> DomainAgent3D::AgentFlagsShift::DOMAINAGENT_FLAGSHIFT_MOVEABLE;
				if ((mov_case & col_case) == 0) return; // no moveable against obstacle
				auto pp = oa.pos - ca.pos;
				auto len = pp.length_squared();
				auto rr = ca.radius + oa.radius;
				TRACE(TRACE_COLLISIONS, "test ", cur, " vs ", oth, " dist: ", len, " r0: ", ca.radius, " r1: ", oa.radius);
				if (len >= rr * rr) return; // too far
#ifdef INDOMAIN_TRACE_STATS
				num_collisions++;
#endif
				if (len < DomainAgent3D::movement_threshold) {
					pp = Vector3{ 1.0f, 0.0f, 0.0f };
					ca.correction -= Vector3{ 0.1f, 0.1f, 0.1f } *ca.radius;
					oa.correction += Vector3{ 0.1f, 0.1f, 0.1f } *oa.radius;
				}
				len = Math::sqrt(len);
				pp *= (rr - len + DomainAgent3D::movement_threshold) / rr / (len + Math_EpsSq);

				// magic
				// m  | o  -> r1 + r2 | 0
				// o  | m  -> 0       | r1 + r2
				// mo | mo -> r1      | r2
				TRACE(TRACE_COLLISIONS, "collision fact ", len, " displacement: ", pp);
				ca.correction -= pp * (((mov_case & 0x1) * oa.radius + (((~mov_case) & 0x2) >> 1) * ca.radius));
				oa.correction += pp * ((((~mov_case) & 0x1) * oa.radius + ((mov_case & 0x2) >> 1) * ca.radius));
				}
			);
#ifdef INDOMAIN_SEQUENTIAL_ITERATION
			for (int i = 0; i < agents_data.size(); ++i)
#else
			for (int i = agents_list_head; i != -1; i = agents_list[i].next)
#endif
			{
				auto& a = agents_data[i];
				if (a.agent == nullptr) {
					continue;
				}
				Vector3 motion = Vector3{ a.radius, a.radius, a.radius }.min(Vector3{ -a.radius, -a.radius, -a.radius }.max(a.correction * collision_relaxation));
				if (a.flags & DomainAgent3D::AgentFlags::DOMAINAGENT_FLAG_INDOMAIN && a.node != -1) { // indomain and landed
					auto n = a.node;
					a.node = domain->motion(a.node, a.pos, motion, motion.length());
				}
				else {
					a.pos += motion;
				}
				a.correction = Vector3{};
			}
#ifdef INDOMAIN_TRACE_STATS
			TRACE(TRACE_STATS, "agent calls. num_agents: ", num_agents, " collision calls: ", num_collisions);
#endif
		}
	}

#ifdef INDOMAIN_SEQUENTIAL_ITERATION
	for (int i = 0; i < agents_data.size(); ++i) {
#else
	for (int i = agents_list_head; i != -1; i = agents_list[i].next) {
#endif
		auto& a = agents_data[i];
		if (a.agent == nullptr) continue;
		a.agent->set_global_position(a.pos);
	}
}

