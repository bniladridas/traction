# Task 2 tooling: creates the flat prototype test map headlessly.
# Run once via: UnrealEditor <uproject> -nullrhi -unattended
#   -ExecCmds="py task2_create_map"
# Produces /Game/Task2/Task2_TestTrack (200m flat floor, PlayerStart, light).
import unreal

try:
    lvl = unreal.EditorLevelLibrary
    print("TASK2MAP: creating /Game/Task2/Task2_TestTrack")
    lvl.new_level("/Game/Task2/Task2_TestTrack")

    cube = unreal.load_asset("/Engine/BasicShapes/Cube")
    print("TASK2MAP: cube asset=" + str(cube is not None))

    floor = lvl.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(0, 0, -50), unreal.Rotator(0, 0, 0))
    floor.static_mesh_component.set_static_mesh(cube)
    floor.set_actor_scale3d(unreal.Vector(200.0, 200.0, 1.0))
    floor.set_actor_label("Task2_Floor")
    print("TASK2MAP: floor done")

    ps = lvl.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0, 0, 100), unreal.Rotator(0, 0, 0))
    ps.set_actor_label("Task2_PlayerStart")
    print("TASK2MAP: playerstart done")

    light = lvl.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 0), unreal.Rotator(-50, -30, 0))
    light.set_actor_label("Task2_Sun")
    print("TASK2MAP: light done")

    lvl.save_current_level()
    print("TASK2MAP: saved OK")
except Exception as e:
    print("TASK2MAP: FAILED " + str(e))
