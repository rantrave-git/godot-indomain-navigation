extends DomainAgent3D

@export
var pointer: PointToGo
@export
var speed: float = 10.0

var next_timer: float = 0.0
var ded = false
var random_pressed = false
func _enter_world() ->void:
	pass

func _enter_tree() -> void:
	#super()
	next_timer = randf() * 2.0 + 0.5
	speed = randf() * 12.0 + 3.0
	if randf() < 0.4:
		flags &= ~DomainAgent3D.DOMAINAGENT_FLAG_INDOMAIN
		print($"MeshInstance3D".material_override)
		$"MeshInstance3D".material_override = load('res://materials/in_air.tres')
		print($"MeshInstance3D".material_override)
	

func _physics_process(delta: float):
	if not is_visible_in_tree() and not ded: return
	next_timer -= delta
	if not transform.is_finite():
		print("IAMDED! ", get_runtime_index())
		hide()
		ded = true
	if Input.is_action_just_pressed("random"):
		random_pressed = true
	if next_timer < 0.0:
		next_timer = randf() * 0.2 + 0.1
		if ded:
			show()
			ded = false
		elif random_pressed:
			go_to(Vector3(0.5 - randf(), 0.5 - randf(), 0.5 - randf()) * 200, speed)
			random_pressed = false
			next_timer = 5.0 
		elif pointer:
			go_to(pointer.target_position, speed)
			#print("go to: ", pointer.target_position, " ", global_position)
			
