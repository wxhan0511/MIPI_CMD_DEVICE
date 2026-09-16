from core.power_board_update import get_power_board_updater

u = get_power_board_updater()
try:
    v = u.read_running_version()
    print("VERSION =", v)
except Exception as e:
    print("ERROR:", e)
