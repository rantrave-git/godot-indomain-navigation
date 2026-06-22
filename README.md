# In-domain navigation

An extension for the Godot engine to handle in-domain navigation (without pathfinding) for thousands of individually controlled agents with collisions handling.


# Build

Clone the repository with submodules.

Build the project:
```bash
$ scons release=yes
```

## Custom compile options

Add following keys to customize build:
- `release=no` - disable release compile flags (`-O2 -mmmx -msse4.1 -ffast-math -fassociative-math` and disable `NDEBUG`)
- `no_tools=yes` - don't include editor tools to the build
- `debug=<module-names>`/`trace=<module-names>` - enable debug/trace logs for selected modules (comma separated):
  + `vmath` - plane math
  + `bsp` - BSP tree
  + `bsp_split` - BSP tree building
  + `move` - in-domain movement methods
  + `grid` - grid lookup
  + `navdomain` - navigation domain building
  + `domainagent` - logs for `DomainAgent3D`
  + `domainregion` - logs for `DomainRegion3D`
  + `collisions` - collision detection
  + `stats` - calls statistics per frame
- `no_sequential_iteration=yes` - use associative list for agents' iteration (may be more effective in some cases)


# Demo

There's configured demo project at `demo/` directory. `NavigationRegion3D` is used to make the domain. Part of the agents are intentionally selected to be not bound to domain to illustrate collision detection.


# Usage

To use agents it's required to make `DomainRegion3D` and attach some `DomainAgent3D`s to it. Then the agents' update process would be handled by the region. Agents may either pick destination or be controlled manually. Each agent moves in the direction to the destination at the desired speed.

## Building domain

Select `NavigationRegion3D` and press `Bake domain` button in 3D editing mode. Pick destination path to save the domain. Configure `Grid Lookup`'s `Cell Size` for optimal performance (not less than greatest agent's radius).

## Configuring scene

Create `DomainRegion3D` and select corresponding `NavigationMesh` and `NavigationDomain`. Add agents. If `Autoattach` flag is set for the agent it's automatically added to the parent region. Agent can be attached to only one region.


# How it works
Associative structure is created for the navigation mesh, allowing to move within polygons from one to another. Two additional lookup containers are created, first is BSP tree for polygons, second is uniform grid collection with associative lists for agents. Agents are only updated at `_physics_update` of the attached `DomainRegion3D`. Collisions are handled with solving nearest neighbour distance optimisation with Gauss-Seidel iterations, number of iterations and relaxation coefficient may be configured.

## Performance
Depending on of the agents' radius and speed and total agents number the performance may be tuned through varrying the lookup grid size and collision parameters.

On my hardware (Ryzen 9 7900X) I managed to achieve 4k+ agents at 144fps (physics frame rate is 144) in one thread.


# TODO
- gizmos for domain region and agents
- jumping agents
- multigrid lookup
- build as godot module
- physical collisions with world at automatic movement phase (requires godot module build for optimal performance)
- add tests
- write docs
