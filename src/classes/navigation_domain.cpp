#include "navigation_domain.h"
#include <stack>
#include <map>
#include <alloca.h>
#include <numeric>
#include <ranges>
#include <set>
#include <utility>
#include <cassert>

#include "math_constants.h"

namespace godot {
    int Polygon::classify(Plane p, const std::vector<Edge>& e) const {
        int result = 0;
        TRACE(TRACE_VMATH, "plane ", p);
        for (size_t i{}; i < 3; ++i) {
            float d = p.distance_to(e[edges[i].edge].p);
            TRACE(TRACE_VMATH, "classify ", e[edges[i].edge].p, " -> ", d, " [", (d < Math_NegEps),  ((d > Math_Eps) << 1), "]");
            result |= (d < Math_NegEps) | ((d > Math_Eps) << 1);
        }
        return result;
    }

    Vector3 Polygon::project(Vector3 pos, const std::vector<Edge>& e) const {
        bool inside = true;
        size_t closest{};
        float dist{ 1e10f };
        TRACE(TRACE_VMATH, "projecting on poly: ", pos);
        for (size_t i{}; i < 3; ++i) {
            auto edge = e[edges[i].edge];
            TRACE(TRACE_VMATH, "edge: ", edge.a, " * t + ", edge.p);
            TRACE(TRACE_VMATH, "n: ", edge.n);
            auto p = pos - edge.p;
            TRACE(TRACE_VMATH, "p: ", p);
            if (edge.n.dot(p) > 0.0f) {
                TRACE(TRACE_VMATH, "out: ", edge.n.dot(p));
                continue;
            }
            // point lays outside a triangle prism along the polygon's normal
            // so closest point is on a boundary
            TRACE(TRACE_VMATH, "in: ", edge.n.dot(p));
            inside = false;
            auto psq = p.length_squared();
            if (psq < dist) {
                // update closest point (edge origin)
                dist = psq;
                closest = i;
            }
            auto d = p.dot(edge.a);
            if (d < 0.0f || d > edge.len) // the edge is further than one of the ends
                continue;
            // return point on the edge
            TRACE(TRACE_VMATH, "on edge");
            return edge.p + d * edge.a;
        }
        if (inside) {
            // closest point is inside the triangle
            TRACE(TRACE_VMATH, "inside");
            auto p = plane.project(pos);
            TRACE(TRACE_VMATH, "point: ", p);
            return p;
        }
        TRACE(TRACE_VMATH, "origin of ", closest);
        return e[edges[closest].edge].p;
    }

    int Polygon::move_along(int index, Vector3& pos, Vector3& d, float& distance, const std::vector<Edge>& e, const std::vector<Polygon>& p) const {
        float total_distance = 0.0f;
        for (size_t _clip{}; _clip < 2; ++_clip) { // cycle once more if motion is clipped by boundary edges
            d = d - d.project(plane.normal);
            auto l = d.length();
            if (l < Math_Eps) {
                // bumped into orthogonal face
                // check if there's an opposite poly to move into
                for (size_t i{}; i < 3; ++i) {
                    auto ee = e[edges[i].edge];
                    auto pn = (pos - ee.p).dot(ee.n);
                    if (pn < Math_Eps) { // placed on the edge
                        auto l = edges[i].link;
                        if (l != -1 && fabs(p[l].plane.project(d).length_squared()) > 1e6f) {
                            // has opposite poly whrere dir projection is not zero
                            return l;
                        }
                    }
                }
                // [NOTE] in a special case when pos is at the corner and both linked polygons are orthogonal to dir
                // we ignore movement assuming there's no way to squeeze between the polygons
                distance = 0.0f;
                return index;
            }
            d /= l;
            for (size_t i{}; i < 3; ++i) {
                TRACE(TRACE_MOVE, "test edge: ", edges[i].edge, " d: ", d, " iter: ", _clip);
                auto ee = e[edges[i].edge];
                auto dn = d.dot(ee.n);
                if (dn > -1.e-3f) // moving from the edge
                    continue;
                // moving against the edge or along
                TRACE(TRACE_MOVE, "motion to the edge: ", edges[i].edge, " dn: ", dn);
                auto da = d.dot(ee.a);
                auto pp = pos - ee.p;
                // [NOTE] assuming `a` is normalized
                auto t = pp.dot(ee.a) - pp.dot(ee.n) * da / dn;
                TRACE(TRACE_MOVE, "t on edge: ", t);
                if (t < Math_NegEps || (t - ee.len) > Math_Eps)
                    continue;
                pp = t * ee.a + ee.p;
                auto ppl = d.dot(pp - pos);
                TRACE(TRACE_MOVE, "distance to edge: ", ppl, " remaining motion: ", distance - total_distance);
                if (ppl > distance - total_distance) { // whole movement is within the face
                    TRACE(TRACE_MOVE, "moved ", (distance - total_distance), " d: ", d);
                    pos += (distance - total_distance) * d;
                    distance = 0.0f;
                    return index;
                }
                total_distance += ppl;
                // update position
                TRACE(TRACE_MOVE, "ppl ", ppl, " on edge ", pp);
                pos = pp;
                if (edges[i].link < 0) { // motion is crossing a boundary edge
                    // clip and repeat
                    d = da * ee.a;
                    TRACE(TRACE_MOVE, "direction changed ", d);
                    // repeat with clipped direction and resting distance
                    break;
                }
                // move out of polygon
                distance -= total_distance;
                return edges[i].link;
            }
        }
        // motion is clipped twice => we're at the corner (moving through corner is not allowed)
        distance = 0.0f;
        return index;
    }

    void BspTree::recompute_max_depth() {
        TRACE(TRACE_BSP, "Eval max depth...");
        if (nodes.size() == 0) {
            TRACE(TRACE_BSP, "The tree is empty!");
            return;
        }
        std::stack<std::pair<int, int>> s;
        s.push({ 0, 0 });
        int depth = 0;
        // int iter = 0;
        while (!s.empty()) {
            // if (iter++ > 100) break;
            auto [ind, d] = s.top();
            TRACE(TRACE_BSP, "ind: ", ind, " depth: ", d);
            s.pop();
            depth = std::max(d, depth);
            for (auto i : nodes[ind].next) {
                if (i < 0) continue;
                s.push({ i, d + 1 });
            }
        }
        TRACE(TRACE_BSP, "max_depth: ", depth);
        max_depth = depth + 1;
    }

    void BspTree::set_data(const PackedByteArray& ndata) {
        lock.write_lock();
        data = ndata;
        leafs.resize(data.decode_u32(0));
        nodes.resize(data.decode_u32(sizeof(uint32_t)));
        memcpy(leafs.data(), data.ptrw() + 2 * sizeof(uint32_t), sizeof(int) * leafs.size());
        memcpy(nodes.data(), data.ptrw() + 2 * sizeof(uint32_t) + sizeof(int) * leafs.size(), sizeof(BspNode) * nodes.size());
        recompute_max_depth();
        notify_property_list_changed();
    }
    PackedByteArray BspTree::get_data() const {
        {
            lock.read_lock();
            if (!is_dirty)
                return data;
        }
        {
            lock.write_lock();
            if (is_dirty) {
                int req_size = sizeof(uint32_t) * 2 + sizeof(BspNode) * nodes.size() + sizeof(int) * leafs.size();
                data.resize(req_size);
                // spin
                data.encode_u32(0, leafs.size());
                data.encode_u32(sizeof(uint32_t), nodes.size());
                mempcpy(data.ptrw() + sizeof(uint32_t) * 2, (uint8_t*)leafs.data(), sizeof(int) * leafs.size());
                mempcpy(data.ptrw() + sizeof(uint32_t) * 2 + sizeof(int) * leafs.size(), (uint8_t*)nodes.data(), sizeof(BspNode) * nodes.size());
                is_dirty = false;
            }
            return data;
        }
    }

    Vector2i BspTree::find_leaf(const Vector3& pos) const {
        if (nodes.size() == 0) {
            return Vector2i(0, leafs[0]);
        }
        auto cur = 0;
        while (cur >= 0) {
            cur = nodes[cur].next[nodes[cur].plane.is_point_over(pos)];
        }
        return Vector2i(-cur - 1, leafs[-cur - 1]);
    }

    void BspTree::print() const {
        std::stack<int> s;
        s.push(0);
        UtilityFunctions::print("BSP Tree");
        for (int i = 0; !s.empty(); ++i) {
            auto n = s.top();
            auto& node = nodes[n];
            UtilityFunctions::print("BspNode[", i, "]: [", node.next[0], ", ", node.next[1], "] | ", node.plane);

            s.pop();
            for (auto i : node.next) {
                if (i < 0) {
                    godot::String s = "Leaf[";
                    Variant q = ~i;
                    s += q.stringify() + "]: [";
                    int end = leafs[~i];
                    for (int j = ~i + 1; j < end; ++j) {
                        q = j;
                        s += q.stringify() + ", ";
                    }
                    s += "]";
                    UtilityFunctions::print(s);
                    continue; // leaf
                }
                s.push(i);
            }
        }
    }

    inline int BspTree::get_content(int index) const {
        return leafs[index];
    }
    void BspTree::build(const BspSplitterFinder& s) {
        int index = 0;
        int max_d = 0;
        typedef struct {
            int leaf;
            int depth;
            int parent;
            int side;
            BspSplitterFinder::build_data_t data;
        } stack_record_t;
        std::vector<stack_record_t> stack;
        std::vector<std::set<int>> leafs;
        std::vector<BspNode> nodes;
        std::vector<std::pair<int, int>> node_leafs;
        std::set<int> front;
        DEBUG(DEBUG_BSP, "BspTree: build start...");

        stack.push_back({ 0, 0, -1, 0, {} });
        leafs.emplace_back(s.initial_leaf(stack[0].data));
        node_leafs.push_back({ 0, 0 });
        stack_record_t cur;
        while (stack.size()) {
            cur = stack.back();
            DEBUG(DEBUG_BSP, "build leaf: ", cur.leaf);
            stack.pop_back();
            max_d = std::max(cur.depth, max_d);
            // if (max_d > 100)
            //     break;
            Plane p;
            front = {};
            BspSplitterFinder::build_data_t d;
            DEBUG(DEBUG_BSP, " estimating split for ", leafs[cur.leaf].size(), " polygons");
            if (s.estimate(cur.depth, leafs[cur.leaf], cur.data, p, front, d)) {
                DEBUG(DEBUG_BSP, " predicted split with", p);
                DEBUG(DEBUG_BSP, "  split:", leafs[cur.leaf].size(), " to ", front.size());
                leafs.emplace_back(std::move(front));
                index++;
                stack.push_back({ cur.leaf, cur.depth + 1, (int)nodes.size(), 0, cur.data });
                stack.push_back({ index, cur.depth + 1, (int)nodes.size(), 1, d });
                if (cur.parent >= 0) {
                    nodes[cur.parent].next[cur.side] = nodes.size();
                }
                node_leafs[cur.leaf] = { nodes.size(), 0 };
                node_leafs.push_back({ nodes.size(), 1 });
                nodes.push_back({ p, {~cur.leaf, ~index} });
            } else {
                DEBUG(DEBUG_BSP, " leaf[", cur.leaf, "] of size ", leafs[cur.leaf].size(), " not splitted");
            }
        }
        DEBUG(DEBUG_BSP, " ns: ", node_leafs.size(), " sz: ", leafs.size(), " nd: ", nodes.size());
        for (size_t i{}; i < leafs.size(); ++i) {
            DEBUG(DEBUG_BSP, " leaf[", i, "].size = ", leafs[i].size());
        }
        index = 0;
        for (size_t i{}; i < leafs.size(); ++i) {
            auto& leaf = leafs[i];
            auto [pnode, side] = node_leafs[i];
            nodes[pnode].next[side] = ~this->leafs.size();
            this->leafs.push_back(this->leafs.size() + leaf.size() + 1);
            std::copy(leaf.cbegin(), leaf.cend(), std::back_inserter(this->leafs));
        }
        this->nodes = std::move(nodes);
        this->max_depth = max_d + 1;
        DEBUG(DEBUG_BSP, " depth ", this->max_depth);
        this->is_dirty = true;
    }
    void BspTree::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_data", "data"), &BspTree::set_data);
        ClassDB::bind_method(D_METHOD("get_data"), &BspTree::get_data);

        ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL), "set_data", "get_data");
    }

    std::set<int> BspSplitterFinder::initial_leaf(build_data_t& data) const {
        if (polys.size() == 0)
            return {};
        data.leaf_size = polys[0].box;
        for (size_t i{ 1 }; i < polys.size(); ++i)
            data.leaf_size.merge_with(polys[i].box);
        std::set<int> result;
        for (size_t i{}; i < polys.size(); ++i)
            result.insert(result.end(), (int)i);
        return result;
    }
    bool BspSplitterFinder::estimate(int depth, std::set<int>& leaf, build_data_t& data, Plane& splitter, std::set<int>& front, build_data_t& dfront) const {
        const float good_relation = 0.75f;
        Plane plane{ Vector3{0.0f, 1.0f, 0.0f}, 0.0f };
        if (data.leaf_size.size.y > 4.0f && leaf.size() > 10) {
            TRACE(TRACE_BSP, " floors situation");
            // potentially splittable by floors
            float low = data.leaf_size.position.y;
            float hi = low + data.leaf_size.size.y;
            Vector2i s{};
            for (size_t i{}; i < 10; ++i) {
                plane.d = (low + hi) * 0.5f;
                s = count_sides(plane, leaf);
                if (s.x < s.y)
                    hi = plane.d;
                else if (s.x > s.y)
                    low = plane.d;
                else {
                    TRACE(TRACE_BSP, "  found split at ", plane.d);
                    break;
                }
            }
            if (s.x < good_relation * leaf.size() && s.y < good_relation * leaf.size()) {
                // able to split by floors
                TRACE(TRACE_BSP, "  splitting vertical");
                do_split(plane, leaf, data, front, dfront);
                splitter = plane;
                return true;
            }
            TRACE(TRACE_BSP, "  floor split failed");
        }
        // find the best splitter along an edge
        int best_metric = leaf.size() * leaf.size() * 2;
        auto size = Vector2i{ (int)leaf.size(), (int)leaf.size() };
        auto best_split{ size };
        // f - b -> 0
        // f + b -> leaf.size
        // f -> leaf.size() / 2
        // b -> leaf.size() / 2
        // metric: (2f-s)^2 + (2b-s)^2 -> min
        for (auto i : leaf) {
            TRACE(TRACE_BSP, " poly: ", i);
            for (auto e : polys[i].edges) {
                auto edge = edges[e.edge];
                if (edge.a.abs().dot(Vector3{ 1.0f, 0.0f, 1.0f }) < Math_Eps)
                    continue;
                Plane p = Plane{ Vector3{edge.a.z, 0.0f, -edge.a.x}.normalized(), edge.p };
                auto s = count_sides(p, leaf);
                TRACE(TRACE_BSP, "  splitter: <", edge.a, " t + ", edge.p, "> sides: ", s.x, " | ", s.y);
                auto mets = s * 2 - size;
                mets = mets * mets;
                int m = mets.x + mets.y;
                if (m < best_metric) {
                    plane = p;
                    best_metric = m;
                    best_split = s;
                }
            }
        }
        TRACE(TRACE_BSP, " best splitter is: ", plane, " with ", best_metric, " and ", best_split.x, " | ", best_split.y);
        if (best_split.x < good_relation * leaf.size() && best_split.y < good_relation * leaf.size()) {
            // able to split by floors
            TRACE(TRACE_BSP, " splitting...");
            do_split(plane, leaf, data, front, dfront);
            splitter = plane;
            return true;
        }

        return false;
    }
    AABB BspSplitterFinder::box(const std::set<int>& data) const {
        if (data.size() == 0)
            return AABB{};
        auto p = data.cbegin();
        AABB result{ polys[*p].box };
        ++p;
        for (; p != data.cend(); ++p) {
            result.merge_with(polys[*p].box);
        }
        return result;
    }

    void BspSplitterFinder::do_split(Plane splitter, std::set<int>& leaf, build_data_t& data, std::set<int>& front, build_data_t& dfront) const {
        size_t size = leaf.size();
        if (size == 0) return;
        TRACE(TRACE_BSP_SPLIT, " do_split: ", splitter, " cursize: ", leaf.size());
        std::vector<int> inds;
        int* indices;
        if (leaf.size() < 4048) {
            // allocate on stack
            indices = (int*)alloca(leaf.size() * sizeof(int));
            std::copy(leaf.cbegin(), leaf.cend(), indices);
        }
        else {
            // for big sets use heap
            inds.resize(size);
            std::copy(leaf.cbegin(), leaf.cend(), inds.begin());
            indices = inds.data();
        }
        TRACE(TRACE_BSP_SPLIT, " do_split stack setup");
        for (size_t i{}; i < size; ++i) {
            auto index = indices[i];
            TRACE(TRACE_BSP_SPLIT, "  classify ", index);
            auto c = polys[index].classify(splitter, edges);
            TRACE(TRACE_BSP_SPLIT, "  res: ", c);
            if ((c & ClassifyFlags::BACK) == 0)
                leaf.erase(index);
            if (c & ClassifyFlags::FRONT)
                front.insert(index);
        }
        data.leaf_size = box(leaf);
        TRACE(TRACE_BSP_SPLIT, " back box: ", data.leaf_size);
        dfront.leaf_size = box(front);
        TRACE(TRACE_BSP_SPLIT, " front box: ", dfront.leaf_size);
    }
    Vector2i BspSplitterFinder::count_sides(Plane plane, std::set<int>& inds) const {
        int f{}, b{};
        for (auto i : inds) {
            auto r = polys[i].classify(plane, edges);
            f += r & 0x2;
            b += r & 0x1;
        }
        return Vector2i{ f >> 1, b };
    }
    void GridLookup::init(int size) {
        lock.write_lock();
        int szx = std::max(1, (int)(box.size.x / approximate_cell_size.x - 1));
        int szy = std::max(1, (int)(box.size.y / approximate_cell_size.y - 1));
        int szz = std::max(1, (int)(box.size.z / approximate_cell_size.z - 1));
        grid_size = coord_t{ (int16_t)szx, (int16_t)szy, (int16_t)szz };
        vec_grid_size = Vector3(grid_size.x - 0.999f, grid_size.y - 0.999f, grid_size.z - 0.999f);
        cell_size = box.size / Vector3(szx, szy, szz);
        if (cell_size.x < Math_Eps) cell_size.x = approximate_cell_size.x;
        if (cell_size.y < Math_Eps) cell_size.y = approximate_cell_size.y;
        if (cell_size.z < Math_Eps) cell_size.z = approximate_cell_size.z;
        grid_heads.resize(szx * szy * szz);
        std::fill(grid_heads.begin(), grid_heads.end(), -1);
        grid_list.resize(grid_heads.size());
        std::fill(grid_list.begin(), grid_list.end(), GridRef{ -1, -1 });
        grid_head = GridRef{ -1, -1 };

        DEBUG(DEBUG_GRID, "Initialized grid_lookup [", grid_size.x, ", ", grid_size.y, ", ", grid_size.z, "] covering ", box);
        DEBUG(DEBUG_GRID, " cell size ", cell_size);
        members.reserve(size);
        members.clear();
        for (int i{}; i < size; ++i) {
            members.emplace_back(Member{ -1, -1, -1 });
        }
    }
    void GridLookup::extend_members(int size) {
        DEBUG(DEBUG_GRID, "Extending grid_lookup..");
        size_t old_size = members.size();
        if (old_size >= size)
            return;
        members.reserve(size);
        for (int i = old_size; i < size; ++i) {
            members.emplace_back(Member{ -1, -1, -1 });
        }
    }
    void GridLookup::init(const AABB& containing_box, const Vector3& approximate_cell_dimensions, int size) {
        approximate_cell_size = approximate_cell_dimensions;
        box = containing_box;
        init(size);
    }
    void GridLookup::init(const AABB& containing_box, float max_radius, int size) {
        init(containing_box, Vector3{ max_radius, max_radius, max_radius }, size);
    }
    void GridLookup::set_cell_size(const Vector3& size) {
        approximate_cell_size = size;
        init();
    }
    Vector3 GridLookup::get_cell_size() const { return approximate_cell_size; }

    void GridLookup::set_box(const AABB& b) {
        box = b;
        init();
    }
    AABB GridLookup::get_box() const { return box; }
    void GridLookup::add_or_update(int index, const Vector3& position) {
        ensure_size(index);
        int cell = locate(position);
        auto& m = members[index];
        if (m.cell == cell) return; // up to date
        auto& head = grid_heads[cell];
        DEBUG(DEBUG_GRID, "member[", cell, "] head: ", head, " cell: ", m.cell);
        assert(m.prev != m.next || m.prev == -1);
        if (m.cell != -1) {
            // changed location, remove from old location
            if (m.prev == m.next) { // both equal -1 (members list contains only one element)
                auto& l = grid_list[m.cell];
                TRACE(TRACE_GRID, " remove head: ", m.cell, " p: ", l.prev, " n: ", l.next);
                if (l.prev != -1) {
                    grid_list[l.prev].next = l.next;
                }
                else { // element is the head one
                    grid_head.next = l.next;
                }
                if (l.next != -1) {
                    grid_list[l.next].prev = l.prev;
                }
                else { // element is the tail one
                    grid_head.prev = l.prev;
                }
                l.next = -1;
                l.prev = -1;
                grid_heads[m.cell] = -1; // reset head to empty
            } else {
                if (m.prev != -1) { // if not first
                    members[m.prev].next = m.next;
                }
                else { // if first so there's a head pointing at the element
                    grid_heads[m.cell] = m.next;
                }
                if (m.next != -1) {
                    members[m.next].prev = m.prev;
                }
            }
        }
        m.cell = cell;
        m.next = head;
        m.prev = -1;
        if (head != -1) {
            // push into existing list
            TRACE(TRACE_GRID, " add to existing list: ", index);
            members[head].prev = index;
            TRACE(TRACE_GRID, " new head: ", grid_heads[cell]);
        }
        else {
            // new list added, update heads list
            if (grid_head.next == -1) {
                // had no elements
                TRACE(TRACE_GRID, " add first head: ", cell);
                grid_head.next = cell;
                grid_head.prev = cell;
                auto& l = grid_list[cell];
                l.prev = -1;
                l.next = -1;
            }
            else {
                // [TODO] compare to pushing head
                // add to tail
                auto& l = grid_list[cell];
                TRACE(TRACE_GRID, " add new head: ", cell, " p: ", l.prev, " n: ", l.next);
                l.prev = grid_head.prev;
                l.next = -1;
                TRACE(TRACE_GRID, " to new head: ", cell, " p: ", l.prev, " n: ", l.next);
                grid_list[l.prev].next = cell;
                grid_head.prev = cell;
                TRACE(TRACE_GRID, " prev p: ", grid_list[l.prev].prev, " n: ", grid_list[l.prev].next);
            }
        }
        head = index;
        assert(m.prev != m.next || m.prev == -1);
    }
    bool GridLookup::remove(int index) {
        if (index >= members.size())
            return false;
        auto& m = members[index];
        if (m.cell == -1)
            return false;
        if (m.prev == m.next) { // both equal -1 (list contains only one element)
            assert(m.prev == -1);
            auto& l = grid_list[m.cell];
            if (l.prev != -1) {
                grid_list[l.prev].next = l.next;
            }
            else { // element is the head one
                grid_head.next = l.next;
            }
            if (l.next != -1) {
                grid_list[l.next].prev = l.prev;
            }
            else { // element is the tail one
                grid_head.prev = l.prev;
            }
            l.next = -1;
            l.prev = -1;
            m.cell = -1;
            grid_heads[m.cell] = -1;
            return true;
        }
        if (m.prev != -1) {
            members[m.prev].next = m.next;
        }
        else {
            grid_heads[m.cell] = m.next;
        }
        if (m.next != -1) {
            members[m.next].prev = m.prev;
        }
        m.prev = -1;
        m.next = -1;
        m.cell = -1;
        return true;
    }
    void GridLookup::ensure_size(int new_index) {
        if (new_index >= members.size()) {
            members.resize((size_t)(new_index * 1.5f) + 1LU);
        }
    }
    void GridLookup::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_cell_size", "size"), &GridLookup::set_cell_size);
        ClassDB::bind_method(D_METHOD("get_cell_size"), &GridLookup::get_cell_size);
        ClassDB::bind_method(D_METHOD("set_box", "box"), &GridLookup::set_box);
        ClassDB::bind_method(D_METHOD("get_box"), &GridLookup::get_box);
        // ClassDB::bind_method(D_METHOD("find_leaf", "pos"), &BspTree::find_leaf);
        // ClassDB::bind_method(D_METHOD("get_content"), &BspTree::get_content);

        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "cell_size"), "set_cell_size", "get_cell_size");
        ADD_PROPERTY(PropertyInfo(Variant::AABB, "box", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL), "set_box", "get_box");
    }
    void BspTree::ProjectionLeafVisitor::operator()(int data) {
        auto pp = polys[data].project(point, edges);
        auto dd = (pp - point).length_squared();
        TRACE(TRACE_BSP, "Visit poly[", data, "] distance: ", dd, " lowest: ", distance_cache);
        if (dd < distance_cache) {
            distance_cache = dd;
            projection = pp;
            poly = data;
        }
    }
    bool NavigationDomain::is_initialized() const {
        return tree_lookup.is_valid() && grid_lookup.is_valid() && polys.size() > 0;
    }
    void NavigationDomain::set_data(const PackedByteArray& ndata) {
        lock.write_lock();
        data = ndata;
        polys.resize(data.decode_u32(0));
        edges.resize(data.decode_u32(sizeof(uint32_t)));
        memcpy(polys.data(), data.ptrw() + 2 * sizeof(uint32_t), sizeof(Polygon) * polys.size());
        memcpy(edges.data(), data.ptrw() + 2 * sizeof(uint32_t) + sizeof(Polygon) * polys.size(), sizeof(Edge) * edges.size());
        notify_property_list_changed();
    }
    PackedByteArray NavigationDomain::get_data() const {
        {
            lock.read_lock();
            if (!is_dirty)
                return data;
        }
        {
            lock.write_lock();
            if (is_dirty) {
                int req_size = sizeof(uint32_t) * 2 + sizeof(Polygon) * polys.size() + sizeof(Edge) * edges.size();
                data.resize(req_size);
                // spin
                data.encode_u32(0, polys.size());
                data.encode_u32(sizeof(uint32_t), edges.size());
                mempcpy(data.ptrw() + 2 * sizeof(uint32_t),
                    (uint8_t*)polys.data(), sizeof(Polygon) * polys.size());
                mempcpy(data.ptrw() + 2 * sizeof(uint32_t) + sizeof(Polygon) * polys.size(),
                    (uint8_t*)edges.data(), sizeof(Edge) * edges.size());
                is_dirty = false;
            }
            return data;
        }
    }
    void NavigationDomain::set_tree_lookup(Ref<BspTree> tree_lookup) { this->tree_lookup = tree_lookup; }
    Ref<BspTree> NavigationDomain::get_tree_lookup() const { return tree_lookup; }
    void NavigationDomain::set_grid_lookup(Ref<GridLookup> grid_lookup) { this->grid_lookup = grid_lookup; }
    Ref<GridLookup> NavigationDomain::get_grid_lookup() const { return grid_lookup; }

    int NavigationDomain::motion(int start_poly, Vector3& pos, const Vector3& dir, float distance) const {
        int prev, cur = start_poly;
        auto d = distance;
        auto direction = dir;
        TRACE(TRACE_NAVDOMAIN, "Navdomain motion at: ", pos, " on: ", start_poly, " to: ", dir, " distance: ", distance);
        while (cur != -1) {
            if (d < Math_Eps)
                return cur;
            auto& p = polys.at(cur);
            prev = cur;
            TRACE(TRACE_NAVDOMAIN, " premove: ", cur, " pos: ", pos, " to: ", d);
            cur = p.move_along(cur, pos, direction, d, edges, polys);
            TRACE(TRACE_NAVDOMAIN, " move: ", cur, " pos: ", pos, " to: ", d);
        }
        return prev;
    }
    void NavigationDomain::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_data", "data"), &NavigationDomain::set_data);
        ClassDB::bind_method(D_METHOD("get_data"), &NavigationDomain::get_data);

        ClassDB::bind_method(D_METHOD("set_tree_lookup", "tree"), &NavigationDomain::set_tree_lookup);
        ClassDB::bind_method(D_METHOD("get_tree_lookup"), &NavigationDomain::get_tree_lookup);

        ClassDB::bind_method(D_METHOD("set_grid_lookup", "grid_lookup"), &NavigationDomain::set_grid_lookup);
        ClassDB::bind_method(D_METHOD("get_grid_lookup"), &NavigationDomain::get_grid_lookup);

        ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL), "set_data", "get_data");
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tree_lookup", PROPERTY_HINT_RESOURCE_TYPE, "BspTree"), "set_tree_lookup", "get_tree_lookup");
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "grid_lookup", PROPERTY_HINT_RESOURCE_TYPE, "GridLookup"), "set_grid_lookup", "get_grid_lookup");
    }
    void NavigationDomain::create_from_navmesh(NavigationMesh* p_mesh) {
        polys.clear();
        edges.clear();
        is_dirty = true;

        if (p_mesh == nullptr) return;
        auto verts = p_mesh->get_vertices();
        auto poly_count = p_mesh->get_polygon_count();
        if (!verts.size() || !poly_count)
            return;

        if (tree_lookup.is_null()) {
            tree_lookup.instantiate();
        }
        if (grid_lookup.is_null()) {
            grid_lookup.instantiate();
        }

        AABB box(verts[0], Vector3{});
        for (int64_t i = 1; i < verts.size(); ++i) {
            box.expand_to(verts[i]);
        }
        std::map<std::pair<int, int>, int> vx_to_edge;
        std::vector<std::pair<int, int>> edge_to_poly;
        polys.resize(poly_count);
        edges.reserve(poly_count * 3 * 2);
        edge_to_poly.resize(poly_count * 3 * 2);

        // build links
        DEBUG(DEBUG_NAVMESH, "Building links...");
        for (int i = 0; i < poly_count; ++i) {
            auto& poly = polys[i];
            auto p = p_mesh->get_polygon(i);

            poly.plane = Plane{ verts[p[0]], verts[p[1]], verts[p[2]], ClockDirection::COUNTERCLOCKWISE };
            for (int j = 0; j < p.size(); ++j) {
                auto p0 = p[j];
                auto p1 = p[(j + 1) % p.size()];
                auto ef = std::make_pair(p0, p1);
                auto er = std::make_pair(p1, p0);
                if (vx_to_edge.count(ef)) {
                    auto edge = vx_to_edge[ef];
                    edges[edge].n = poly.plane.normal.cross(edges[edge].a).normalized();
                    auto op = edge_to_poly[edge];
                    poly.edges[j].edge = edge;
                    poly.edges[j].link = op.first;
                    DEBUG(DEBUG_NAVMESH, " poly[", op.first, "][", op.second, "].link = ", i);
                    polys[op.first].edges[op.second].link = i;
                }
                else {
                    vx_to_edge[ef] = edges.size();
                    DEBUG(DEBUG_NAVMESH, " poly[", i, "][", j, "].edge = ", edges.size());
                    poly.edges[j].edge = edges.size();
                    auto e = Edge::create(verts[p0], verts[p1], poly.plane.normal);
                    edges.push_back(e);
                    edge_to_poly[edges.size()] = std::make_pair(i, j);
                    vx_to_edge[er] = edges.size();
                    edges.push_back(e.flipped());
                }
                poly.box.expand_to(verts[p[1]]);
            }
        }

        edges.shrink_to_fit();

        BspSplitterFinder splitter{ polys, edges };
        DEBUG(DEBUG_NAVMESH, "building tree...");
        tree_lookup->build(splitter);

        DEBUG(DEBUG_NAVMESH, "building grid_lookup...");
        grid_lookup->init(box, p_mesh->get_agent_radius());
#ifdef INDOMAIN_DEBUG_NAVMESH
        print();
#endif
    }
    void NavigationDomain::init_grid(int size) {
        grid_lookup->init(size);
    }
    void NavigationDomain::extend_grid(int size) {
        grid_lookup->extend_members(size);
    }
}
