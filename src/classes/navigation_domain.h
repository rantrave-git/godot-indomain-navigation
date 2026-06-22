#pragma once

#include <godot_cpp/classes/navigation_mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <godot_cpp/variant/packed_byte_array.hpp>

#include <vector>
#include <set>

#include "rw_lock.h"
#include "trace.h"

namespace godot {
	enum ClassifyFlags : unsigned int {
		INDICENT = 0x0,
		BACK = 0x1,
		FRONT = 0x2,
		CROSSING = 0x3,
	};

	struct Edge {
		Vector3 a; // (end - start).normalized()
		Vector3 p; // start
		Vector3 n; // normal of the edge within the polygon plane, facing inside
		float len;
		inline Edge flipped() const { return { -a, p + a * len, -n, len }; }
		static inline Edge create(Vector3 v0, Vector3 v1, Vector3 face_normal) {
			auto a = v1 - v0;
			auto l = a.length();
			return Edge{ a / l, v0, face_normal.cross(a).normalized(), l };
		}
	};

	struct EdgeRef {
		int edge = -1; // index of an edge
		int link = -1; // index of the opposite polygon through the edge
	};

	struct Polygon {
		Plane plane;
		EdgeRef edges[3];
		AABB box;
		inline int classify(Plane p, const std::vector<Edge>& e) const;
		inline Vector3 project(Vector3 pos, const std::vector<Edge>& e) const;
		int move_along(int index, Vector3& pos, Vector3& dir, float& distance, const std::vector<Edge>& e, const std::vector<Polygon>& p) const;
	};

	struct BspNode {
		Plane plane;
		int next[2]; // 0 - back, 1 - front
	};

	class BspSplitterFinder {
		std::vector<Polygon>& polys;
		std::vector<Edge>& edges;

	public:
		typedef struct {
			AABB leaf_size;
		} build_data_t;

		BspSplitterFinder(std::vector<Polygon>& polys, std::vector<Edge>& edges) : polys{ polys }, edges{ edges } { }
		// get all polys
		std::set<int> initial_leaf(build_data_t& data) const;

		// find the best splitter and move leaf's polys to front if they are in front of the splitter's plane and add to front if they are crossing
		bool estimate(int depth, std::set<int>& leaf, build_data_t& data, Plane& splitter, std::set<int>& front, build_data_t& dfront) const;

	private:
		AABB box(const std::set<int>& data) const;
		void do_split(Plane splitter, std::set<int>& leaf, build_data_t& data, std::set<int>& front, build_data_t& dfront) const;
		Vector2i count_sides(Plane plane, std::set<int>& inds) const;
	};

	class BspTree : public Resource {
		GDCLASS(BspTree, Resource);
		// leaf1_index, ...leaf0_data, leaf2_index, ...leaf1_data, ... , size(), ...leafN_data
		std::vector<int> leafs;
		std::vector<BspNode> nodes;
		int max_depth = 0;

		mutable RWLock lock;
		mutable PackedByteArray data; // cache for data updates
		mutable bool is_dirty = true; // true when data is out of sync with leafs & nodes

		void recompute_max_depth();
	public:
		void set_data(const PackedByteArray& ndata);
		PackedByteArray get_data() const;

		inline int get_max_depth() const { return max_depth; }

		Vector2i find_leaf(const Vector3& pos) const;

		void print() const;

		inline int get_content(int index) const;

		// template<class _SplitterFinder>
		void build(const BspSplitterFinder& s);

		class ProjectionLeafVisitor {
			float distance_cache;

		public:
			Vector3 point;
			Vector3 projection;
			int poly;
			const BspTree& tree;
			const std::vector<Polygon>& polys;
			const std::vector<Edge>& edges;

			ProjectionLeafVisitor(
				Vector3 pos, const BspTree& t, const std::vector<Polygon>& p, const std::vector<Edge>& e) : point{ pos }, tree{ t }, polys{ p }, edges{ e }, distance_cache{ 1e10f }, poly{ -1 } { }
			void operator()(int data);
		};

		template <class _Visitor>
		void inline sphere_visit(Vector3 pos, float radius, _Visitor& visitor) const {
			if (max_depth == 0) {
				TRACE(TRACE_BSP, "Only one leaf");
				for (int i{ 1 }; i < leafs[0]; ++i)
					visitor(leafs[i]);
				return;
			}
			TRACE(TRACE_BSP, "multileaf ", max_depth);
			int size{ 1 }, * stack{ (int*)alloca(max_depth + 1) };

			stack[0] = 0;
			while (size > 0) {
				int cur = stack[--size];
				TRACE(TRACE_BSP, "bsp cur:", cur);
				if (cur < 0) {
					int s{ ~cur };
					int e{ leafs[s] };
					for (int i{ s + 1 }; i < e; ++i)
						visitor(leafs[i]);
				}
				else {
					auto& node = nodes[cur];
					float dist = node.plane.distance_to(pos);
					if (dist > -radius)
						stack[size++] = node.next[1];
					if (dist < radius)
						stack[size++] = node.next[0];
				}
			}
		}

		inline std::pair<Vector3, int> project(Vector3 pos, float radius, const std::vector<Polygon>& polys, const std::vector<Edge>& edges) const {
			ProjectionLeafVisitor v{ pos, *this, polys, edges };
			sphere_visit(pos, radius, v);
			return std::make_pair(v.projection, v.poly);
		}

	protected:
		static void _bind_methods();
	};

	alignas(16) typedef struct {
		int x, y, z;
	} coord_t;

	constexpr inline int to_index(coord_t coord, coord_t size) {
		return coord.x + size.x * (coord.y + size.y * coord.z);
	}
	constexpr inline coord_t from_index(coord_t size, int i) {
		return { i % size.x, (i / size.x) % size.y,  i / size.x / size.y };
	}
	constexpr inline int to_index_clip(coord_t coord, coord_t size) {
		auto cx = std::min(coord.x, size.x);
		auto cy = std::min(coord.y, size.y);
		auto cz = std::min(coord.z, size.z);
		cx = std::max(0, cx);
		cy = std::max(0, cy);
		cz = std::max(0, cz);
		return cx + size.x * (cy + size.y * cz);
	}
	constexpr inline int shift(int cell, coord_t size, coord_t shift) {
		coord_t c{
			(cell % size.x + size.x + shift.x) % size.x,
			((cell / size.x) % size.y + size.y + shift.y) % size.y,
			((cell / size.x / size.y) + size.z + shift.z) % size.z,
		};
		return to_index(c, size);
	}

	class GridLookup : public Resource {
		GDCLASS(GridLookup, Resource);
		struct alignas(16) Member {
			int cell = -1;
			int prev = -1;
			int next = -1;
		};
		struct alignas(8) GridRef {
			int prev;
			int next;
		};
		coord_t grid_size;
		Vector3 vec_grid_size;
		Vector3 cell_size;
		mutable RWLock lock;

	public:
		friend class GridIterator;
		// [NOTE] not safe! Has no explicit checks for an invalid grid_lookup nor same grid_lookup check at comparison.
		class GridIterator {
			const GridLookup& grid_lookup;
			int current;

		public:
			GridIterator() = delete;
			GridIterator(const GridIterator& it) = default;
			GridIterator(GridIterator&& it) = default;
			GridIterator(const GridLookup& grid_lookup) : grid_lookup{ grid_lookup }, current{ grid_lookup.grid_head.next } { }
			GridIterator(const GridLookup& grid_lookup, int current) : grid_lookup{ grid_lookup }, current{ current } { }
			GridIterator& operator=(const GridIterator& it) = default;
			GridIterator& operator=(GridIterator&& it) = default;
			inline int operator*() const { return current; }
			inline GridIterator operator++() {
				current = grid_lookup.grid_list[current].next;
				return *this;
			}
			inline GridIterator operator--() {
				current = grid_lookup.grid_list[current].prev;
				return *this;
			}
			inline GridIterator operator++(int) {
				auto temp = *this;
				current = grid_lookup.grid_list[current].next;
				return temp;
			}
			inline GridIterator operator--(int) {
				auto temp = *this;
				current = grid_lookup.grid_list[current].prev;
				return temp;
			}
			inline friend bool operator==(const GridIterator& lhs, const GridIterator& rhs) {
				return lhs.current == rhs.current; // &lhs.grid_lookup == &rhs.grid_lookup
			}
			inline friend bool operator!=(const GridIterator& lhs, const GridIterator& rhs) {
				return lhs.current != rhs.current; // &lhs.grid_lookup == &rhs.grid_lookup
			}
		};
		friend class GridMemberIterator;
		// [NOTE] not safe! Has no explicit checks for an invalid grid_lookup nor same grid_lookup check at comparison.
		class GridMemberIterator {
			const GridLookup& grid_lookup;
			int current;

		public:
			GridMemberIterator() = delete;
			GridMemberIterator(const GridMemberIterator& it) = default;
			GridMemberIterator(GridMemberIterator&& it) = default;
			GridMemberIterator(const GridLookup& grid_lookup, int cell) : grid_lookup{ grid_lookup }, current{ grid_lookup.grid_heads[cell] } { }
			GridMemberIterator(const GridLookup& grid_lookup, int cell, int current) : grid_lookup{ grid_lookup }, current{ current } { }
			GridMemberIterator& operator=(const GridMemberIterator& it) = default;
			GridMemberIterator& operator=(GridMemberIterator&& it) = default;
			inline int operator*() const { return current; }
			inline GridMemberIterator operator++() {
				current = grid_lookup.members[current].next;
				return *this;
			}
			inline GridMemberIterator operator--() {
				current = grid_lookup.members[current].prev;
				return *this;
			}
			inline GridMemberIterator operator++(int) {
				auto temp = *this;
				current = grid_lookup.members[current].next;
				return temp;
			}
			inline GridMemberIterator operator--(int) {
				auto temp = *this;
				current = grid_lookup.members[current].prev;
				return temp;
			}
			inline constexpr GridMemberIterator operator+(int shift) const {
				auto res = *this;
				for (int i = 0; i < shift; ++i, ++res);
				return res;
			}
			inline friend bool operator==(const GridMemberIterator& lhs, const GridMemberIterator& rhs) {
				return lhs.current == rhs.current; // &lhs.grid_lookup == &rhs.grid_lookup
			}
			inline friend bool operator!=(const GridMemberIterator& lhs, const GridMemberIterator& rhs) {
				return lhs.current != rhs.current; // &lhs.grid_lookup == &rhs.grid_lookup
			}
		};

		AABB box;
		Vector3 approximate_cell_size = Vector3{ 2.0f, 2.0f, 2.0f };
		// runtime data
		GridRef grid_head; // next - a head of the heads list, prev - a tail of the heads list
		std::vector<int> grid_heads;
		std::vector<GridRef> grid_list;
		std::vector<Member> members;

		void init(int size = 65536);

		void extend_members(int size);
		void init(const AABB& containing_box, const Vector3& approximate_cell_dimensions, int size = 65536);
		void init(const AABB& containing_box, float max_radius, int size = 65536);

		void set_cell_size(const Vector3& size);
		Vector3 get_cell_size() const;

		void set_box(const AABB& b);
		AABB get_box() const;

		void add_or_update(int index, const Vector3& position);
		bool remove(int index);

		inline void print_heads() const {
			int cur = grid_head.next;
			if (cur == -1) {
				UtilityFunctions::print("Grid heads: empty");
			}
			int i = 0;
			while (cur != -1) {
				UtilityFunctions::print("Grid heads: ", cur);
				cur = grid_list[cur].next;
			}
		}

		inline void print_grid() const {
			int cell = 0;
			int xy = grid_size.x * grid_size.y;
			int x = grid_size.x;
			UtilityFunctions::print("Grid cells <", grid_size.x, ", ", grid_size.y, ", ", grid_size.z, "> sz:", grid_heads.size());
			for (int k = 0; k < grid_size.z; ++k) {
				int offsetz = xy * k;
				for (int j = 0; j < grid_size.y; ++j) {
					int offsety = x * j;
					for (int i = 0; i < grid_size.x; ++i) {
						int cell = i + offsety + offsetz;
						int cur = grid_heads[cell];
						UtilityFunctions::print(" Cell[", cell, "]: ", cur);
						for (int cur = grid_heads[cell], maxi = 100; cur != -1; --maxi, cur = members[cur].next) {
							UtilityFunctions::print("  [", cur, "]");
						}
					}
				}
			}
		}
		inline void print_members() const {
			UtilityFunctions::print("Grid members ", members.size());
			for (int k = 0; k < members.size(); ++k) {
				auto m = members[k];
				if (m.cell == -1 && m.next == -1 && m.prev == -1) continue;

				UtilityFunctions::print("  [", k, "]: ", m.cell, " p: ", m.prev, " n: ", m.next);
			}
		}

		template <class _Func2>
		inline void iterate_pairs(int cell, _Func2 func) const {
			int cur0 = grid_heads[cell];
			Member a{-1, -1, -1};
			Member b{-1, -1, -1};
			for (int cur0 = grid_heads[cell]; cur0 != -1; cur0 = a.next) {
				a = members[cur0];
				for(int cur1 = a.next; cur1 != -1; cur1 = b.next) {
					b = members[cur1];
					func(cur0, cur1);
				}
			}
		}

		template <class _Func2>
		inline void iterate_pairs(int cell0, int cell1, _Func2 func) const {
			int cur0 = grid_heads[cell0];
			int beg1 = grid_heads[cell1];
			while (cur0 != -1) {
				auto a = members[cur0];
				int cur1 = beg1;
				while (cur1 != -1) {
					auto b = members[cur1];
					func(cur0, cur1);
					cur1 = b.next;
				}
				cur0 = a.next;
			}
		}

		template <class _Func2>
		void constexpr iterate_all_pairs(int radius_x, int radius_y, int radius_z, _Func2 func) const {
			int cur_cell = grid_head.next;
			while (cur_cell != -1) {
				auto coords = from_index(grid_size, cur_cell);
				iterate_pairs<_Func2>(cur_cell, func);
				int maxx = Math::min(grid_size.x, coords.x + radius_x) - coords.x;
				int maxy = Math::min(grid_size.y, coords.y + radius_y) - coords.y;
				int maxz = Math::min(grid_size.z, coords.z + radius_z) - coords.z;

				for (int i = 1; i < maxx; ++i) {
					iterate_pairs<_Func2>(cur_cell, cur_cell + i, func);
				}

				for (int j = 1; j < maxy; ++j) {
					int offset_y = grid_size.x * j;
					for (int i = 0; i < maxx; ++i) {
						iterate_pairs<_Func2>(cur_cell, cur_cell + i + offset_y, func);
					}
				}
				for (int k = 1; k < maxz; ++k) {
					int offset_z = grid_size.x * grid_size.y * k;
					for (int j = 0; j < maxy; ++j) {
						int offset_y = grid_size.x * j;
						for (int i = 0; i < maxx; ++i) {
							iterate_pairs<_Func2>(cur_cell, cur_cell + i + offset_y + offset_z, func);
						}
					}
				}

				cur_cell = grid_list[cur_cell].next;
			}
		}
		template <class _Func2>
		void constexpr iterate_all_pairs(int radius, _Func2 func) const {
			iterate_all_pairs<_Func2>(radius, radius, radius, func);
		}
		inline constexpr int cell_shift(int index, int x, int y, int z) const {
			return shift(index, grid_size, { x, y, z });
		}
		inline GridIterator cbegin() const { return GridIterator{ *this }; }
		inline GridIterator cend() const { return GridIterator{ *this, -1 }; }
		inline GridMemberIterator cmembegin(int cell) const { return GridMemberIterator{ *this, cell }; }
		inline GridMemberIterator cmemend(int cell) const { return GridMemberIterator{ *this, cell, -1 }; }
	private:
		inline int locate(Vector3 pos) const {
			auto c = Vector3{}.max(vec_grid_size.min((pos - box.position) / cell_size));
			return to_index({ (int)c.x, (int)c.y, (int)c.z }, grid_size);
		}
		void ensure_size(int new_index);

	protected:
		static void _bind_methods();
	};

	class NavigationDomain : public Resource {
		GDCLASS(NavigationDomain, Resource);
		RWLock rwlock;

		std::vector<Polygon> polys;
		// [NOTE] edges have duplicates for both sides of an edge
		std::vector<Edge> edges;
		Ref<BspTree> tree_lookup;
		Ref<GridLookup> grid_lookup;

		mutable RWLock lock;
		mutable PackedByteArray data;
		mutable bool is_dirty = true;

	public:

		bool is_initialized() const;
		void set_data(const PackedByteArray& ndata);
		PackedByteArray get_data() const;

		void set_tree_lookup(Ref<BspTree> tree_lookup);
		Ref<BspTree> get_tree_lookup() const;
		void set_grid_lookup(Ref<GridLookup> grid_lookup);
		Ref<GridLookup> get_grid_lookup() const;

		inline int get_tree_max_depth() const { return tree_lookup->get_max_depth(); }

		inline std::pair<Vector3, int> project(Vector3 pos, float radius) const {
			return tree_lookup->project(pos, radius, polys, edges);
		}
		inline Vector3 normal(int poly) const { return polys[poly].plane.normal; }

		int motion(int start_poly, Vector3& pos, const Vector3& dir, float distance) const;

		void print() const {
			UtilityFunctions::print("Domain mesh");
			for (auto p : polys) {
				UtilityFunctions::print(" Poly: [",
					"<", p.edges[0].edge, ", ", p.edges[0].link, ">, ",
					"<", p.edges[1].edge, ", ", p.edges[1].link, ">, ",
					"<", p.edges[2].edge, ", ", p.edges[2].link, ">"
					"] plane: ", p.plane);
			}
			for (int i = 0; i < edges.size(); ++i) {
				auto e = edges[i];
				UtilityFunctions::print(" Edge[", i, "]: ", e.a, " * t + ", e.p, " n: ", e.n, " l: ", e.len);
			}
			UtilityFunctions::print("======");
			tree_lookup->print();
		}

	protected:
		static void _bind_methods();

	public:
		void create_from_navmesh(NavigationMesh* p_mesh);
		void init_grid(int size);
		void extend_grid(int size);
	};
}