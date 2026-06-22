class_name PointToGo extends Camera3D

@export
var target_position: Vector3 = Vector3.ZERO

func _process(_delta: float) -> void:
	if (Input.is_action_just_pressed("target")):
		var space_state = get_world_3d().direct_space_state
		var mousepos = get_viewport().get_mouse_position()
		var origin = project_ray_origin(mousepos)
		var end = origin + project_ray_normal(mousepos) * 10000.0
		var query = PhysicsRayQueryParameters3D.create(origin, end, 2)
		query.collide_with_areas = false
		var result = space_state.intersect_ray(query)
		if (result):
			target_position = result["position"]
		print("gtg: ", target_position)
		
