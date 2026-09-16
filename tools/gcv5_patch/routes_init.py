"""HTTP 路由聚合包 (原 web/routes.py 1904 行拆分而来).

所有子模块共享同一个 Blueprint `api_bp`. 本 `__init__` 在定义 Blueprint 之后
立即 import 每个子模块, 让子模块里的 `@api_bp.route(...)` 装饰器触发注册副作用.

拆分原则:
- system.py       /version /status /system/* /performance/stats
- update.py       /update/* (含备份/回滚/重启 helper)
- auth.py         /login /logout
- users.py        /users*
- nodes.py        /nodes*
- pages.py        / /login_page /users_page /solo /multi /dashboard /update_page /logs_page
- power_meter.py  /power/* /meter/*
- power_board.py  /power_board/* (电源板 STM32 固件在线升级)
- images.py       /images*
- files.py        /files*
- execute.py      /execute*
- jobs.py         /jobs/*
- logs.py         /logs/files /logs/content/<filename>
- lua_api.py      /lua/commands (Lua 指令目录数据)
"""

from flask import Blueprint, jsonify, request
from werkzeug.exceptions import HTTPException

from core.dispatcher import CommandError
from core.logger import logger

api_bp = Blueprint("api", __name__)


@api_bp.errorhandler(CommandError)
def _handle_command_error(e):
    """业务错误统一出口, 与原各路由内联 `except CommandError` 的响应结构一致."""
    return jsonify({"error": str(e)}), e.status


@api_bp.errorhandler(Exception)
def _handle_unexpected_error(e):
    """未预期错误统一出口; HTTPException (404/405 等) 保持 Flask 原生行为."""
    if isinstance(e, HTTPException):
        return e
    logger.exception("Unhandled error in %s %s", request.endpoint, request.path)
    return jsonify({"error": str(e)}), 500

# 导入顺序无强依赖, 但按功能聚合便于排查.
# 任何新增的子模块都需要在此处 import 一次才会被 Flask 注册.
from web.routes import (  # noqa: E402,F401
    system,
    update,
    kernel,
    auth,
    users,
    nodes,
    pages,
    power_meter,
    power_board,
    images,
    files,
    execute,
    jobs,
    logs,
    lua_api,
)

__all__ = ["api_bp"]
