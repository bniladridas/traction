# Task 7 tooling: creates the first circuit test map headlessly.
# Run once via: UnrealEditor <uproject> -nullrhi -unattended
#   -ExecCmds="py track1_create_map"
# Produces /Game/Track/Track1_TestCircuit (PlayerStart near the track-owned
# start, light). Road, walls, centerline, and checkpoints are built at
# runtime by ARaceTrack from FRaceTrackConfig; the GameMode snaps the
# vehicle to the exact start pose, so PlayerStart is approximate by design.
import unreal

try:
    lvl = unreal.EditorLevelLibrary
    print("TRACK1MAP: creating /Game/Track/Track1_TestCircuit")
    lvl.new_level("/Game/Track/Track1_TestCircuit")

    ps = lvl.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(-1114, -103, 100), unreal.Rotator(0, -14.9, 0))
    ps.set_actor_label("Track1_PlayerStart")
    print("TRACK1MAP: playerstart done")

    light = lvl.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 0), unreal.Rotator(-50, -30, 0))
    light.set_actor_label("Track1_Sun")
    print("TRACK1MAP: light done")

    lvl.save_current_level()
    print("TRACK1MAP: saved OK")
except Exception as e:
    print("TRACK1MAP: FAILED " + str(e))
