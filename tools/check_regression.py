"""Local regression check for traction (runs on the Mac, not in CI).
Reads the seven headless E2E artifacts produced by the four local runs
(flat map, circuit map, race-state map, AI map) and fails unless every
flag is true: 30 frozen regression flags plus 10 Task 8 gates plus
6 Task 9 gates.
Usage from the repo root: python3 tools/check_regression.py
"""
import json
import sys

T2_KEYS = ['pass_forward', 'pass_brake', 'pass_reverse', 'pass_steer',
           'pass_camera', 'pass_reset']
T3_KEYS = ['pass_task3_gravity', 'pass_task3_mass', 'pass_task3_brake_force',
           'pass_task3_reverse_bound', 'pass_task3_steer_rule',
           'pass_task3_wheels']
T5_KEYS = ['contact_ok', 'susp_ok', 'load_ok', 'long_ok', 'lat_ok',
           'circle_ok']
T6_KEYS = ['engine_response', 'rpm_bounds', 'gear_progression',
           'reverse_drive', 'torque_transfer', 'engine_braking']
T7_KEYS = ['track_load', 'road_contact', 'start_alignment',
           'centerline_valid', 'checkpoint_order', 'lap_traversal']
T8_KEYS = ['starts_ready', 'countdown_ok', 'racing_begins',
           'checkpoint_order', 'wrong_order_ignored', 'single_increment',
           'no_double_count', 'reset_behavior', 'finish_transition',
           'finished_locked']
T9_KEYS = ['ai_spawned', 'ai_progress', 'ai_lap', 'ai_valid',
           'ai_recovery', 'ai_timing']


def load(name):
    with open('game/RacingGame/Saved/%s/results.json' % name) as f:
        return json.load(f)


def main():
    t2 = load('Task2E2E')
    t5 = load('Task5E2E')
    t6 = load('Task6E2E')
    t7 = load('Task7E2E')
    t8 = load('Task8E2E')
    t9 = load('Task9E2E')
    flags = ([t2[k] for k in T2_KEYS] + [t2[k] for k in T3_KEYS]
             + [t5[k] for k in T5_KEYS] + [t6[k] for k in T6_KEYS]
             + [t7[k] for k in T7_KEYS] + [t8[k] for k in T8_KEYS]
             + [t9[k] for k in T9_KEYS])
    print('%d flags: %s' % (len(flags), flags))
    ok = len(flags) == 46 and all(flags) and t2['reached_end']
    print('PASS' if ok else 'FAIL')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
