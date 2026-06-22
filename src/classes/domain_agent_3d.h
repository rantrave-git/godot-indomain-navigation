#pragma once

#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

#include <godot_cpp/classes/collision_shape3d.hpp>

namespace godot {
	class DomainRegion3D;

	class DomainAgent3D : public Node3D {
		GDCLASS(DomainAgent3D, Node3D);
	public:
		enum AgentFlags {
			DOMAINAGENT_FLAG_OBSTACLE = 0x1, // 
			DOMAINAGENT_FLAG_INDOMAIN = 0x2, // snaps to domain
			DOMAINAGENT_FLAG_MOVEABLE = 0x4, // can be moved during agents' collision step
			DOMAINAGENT_FLAG_MANUAL = 0x8, // use virtual `_request_motion` function for each frame movement
			DOMAINAGENT_FLAG_AUTOSNAP = 0x10, // snap to close domain at landing
			DOMAINAGENT_FLAG_CLIPVELOCITY = 0x20, // ensure that velocity never exceed the radius
			DOMAINAGENT_FLAG_AUTOATTACH = 0x40, // attach to parent on start
		};

		enum AgentFlagsShift {
			DOMAINAGENT_FLAGSHIFT_OBSTACLE = 0,
			DOMAINAGENT_FLAGSHIFT_INDOMAIN = 1,
			DOMAINAGENT_FLAGSHIFT_MOVEABLE = 2,
			DOMAINAGENT_FLAGSHIFT_MANUAL = 3,
			DOMAINAGENT_FLAGSHIFT_AUTOSNAP = 4,
			DOMAINAGENT_FLAGSHIFT_CLIPVELOCITY = 5,
			DOMAINAGENT_FLAGSHIFT_AUTOATTACH = 6,
		};

	protected:
		static void _bind_methods();

	public:
		constexpr static float movement_threshold = 1e-3f;
		DomainAgent3D();
		// properties
		Vector3 gravity;
		BitField<AgentFlags> flags;
		float radius;
		DomainRegion3D* domain;
		Ref<CollisionShape3D> colshape;

		// runtime
		int index = -1;
		Vector3 velocity;
		Vector3 destination;
		float speed = 0.0f;

		void set_radius(float value);
		float get_radius() const;
		void set_flags(BitField<DomainAgent3D::AgentFlags> value);
		BitField<DomainAgent3D::AgentFlags> get_flags() const;
		void set_gravity(const Vector3& value);
		Vector3 get_gravity() const;

		int get_runtime_index() const { return index; }

		void attach(DomainRegion3D* to_domain, bool try_land = false);
		void detach();
		void teleport(Vector3 to);
		void go_to(Vector3 destination, float speed);
		void jump_towards(Vector3 impulse);

		GDVIRTUAL2R_REQUIRED(Vector3, _request_motion, float, bool);

		virtual void request_motion(float delta, Vector3& res, int node, Vector3& pos);
	};
}
VARIANT_BITFIELD_CAST(DomainAgent3D::AgentFlags)