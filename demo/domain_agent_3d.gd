extends DomainAgent3D

var speed = 1

func _request_motion(delta: float, is_landed: bool) -> Vector3:
	var res := Vector3.ZERO
	if Input.is_action_pressed("back"):
		res.z -= speed * delta
	if Input.is_action_pressed("forward"):
		res.z += speed * delta
	if Input.is_action_pressed("left"):
		res.x += speed * delta
	if Input.is_action_pressed("right"):
		res.x -= speed * delta
	#print(global_position)
	return res
