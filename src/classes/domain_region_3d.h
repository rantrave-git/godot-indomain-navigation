#pragma once

#include <godot_cpp/classes/navigation_mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/core/mutex_lock.hpp>

#include <vector>
#include <map>
#include <set>

#include "navigation_domain.h"
#include "domain_agent_3d.h"

namespace godot {
	class DomainAgent3D;

	class DomainRegion3D : public Node3D {
		GDCLASS(DomainRegion3D, Node3D);

		struct AgentData {
			Vector3 pos;
			Vector3 motion;
			Vector3 correction;
			float radius;
			float step;
			DomainAgent3D::AgentFlags flags;
			int node = -1; // not landed if -1
			DomainAgent3D* agent; // not bound if nullptr
		};
		struct AgentList {
			int prev;
			int next;
		};
	private:
		std::vector<AgentData> agents_data;
#ifndef INDOMAIN_SEQUENTIAL_ITERATION
		std::vector<AgentList> agents_list;
#endif
		std::vector<int> free_slots;
		float collision_relaxation = 0.5f;
		int collision_iterations = 2;
#ifndef INDOMAIN_SEQUENTIAL_ITERATION
		int agents_list_head = -1;
#endif
		// Transform3D inv_transform;
	protected:
		static void _bind_methods();
		void resize(int new_size);

	public:
		Ref<NavigationMesh> mesh;
		Ref<NavigationDomain> domain;
		void update_position(int agent_index, Vector3 position);
		void update_motion(int agent_index, Vector3 motion);
		void update_radius(int agent_index, float radius);
		void update_flags(int agent_index, BitField<DomainAgent3D::AgentFlags> flags);

		inline Vector3 get_agent_position(int index) const {
			return //get_global_transform().xform(
				agents_data.at(index).pos
				// )
				;
		}

		bool is_landed(int agent_index) const;
		Vector3 normal(int agent_index) const;

		int add_agent(DomainAgent3D* agent);
		void remove_agent(DomainAgent3D* agent);
		void land(int index);
		void lift(int index);

		bool project(Vector3& point, float radius, Vector3& normal) const;

		void set_navigation_mesh(const Ref<NavigationMesh>& p_navigation_mesh);
		Ref<NavigationMesh> get_navigation_mesh() const;

		void set_navigation_domain(const Ref<NavigationDomain>& p_navigation_domain);
		Ref<NavigationDomain> get_navigation_domain() const;

		void set_collision_relaxation(float value);
		float get_collision_relaxation() const;

		void set_collision_iterations(int value);
		int get_collision_iterations() const;
		// void build_domain(Ref<NavigationMesh> navmesh);

		void _ready() override;
		void _exit_tree() override;
		void _physics_process(double delta) override;
	};
}