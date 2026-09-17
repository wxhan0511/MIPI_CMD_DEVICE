"""电源板 (STM32) 固件在线升级路由: /power_board/*."""

from __future__ import annotations

from flask import jsonify, request

from core.logger import logger
from core.power_board_update import (
    PB_FLASH_MAX,
    PowerBoardUpdateError,
    get_power_board_updater,
)
from web.auth import login_required, roles_allowed
from web.routes import api_bp


@api_bp.route("/power_board/status", methods=["GET"])
@login_required
def power_board_status():
    """
    升级状态 (前端轮询)
    ---
    tags:
      - Power Board Update
    responses:
      200:
        description: 当前升级状态
    """
    return jsonify(get_power_board_updater().get_state())


@api_bp.route("/power_board/version", methods=["GET"])
@login_required
def power_board_version():
    """
    查询电源板当前运行固件版本 (通过 SPI, 点击查询按钮触发)
    ---
    tags:
      - Power Board Update
    responses:
      200:
        description: 版本号 (如 "0.0.0.1"), 查询失败时 version 为 null
    """
    upd = get_power_board_updater()
    if upd.get_state().get("running"):
        return jsonify({"version": None, "error": "升级进行中, 请稍后再查询"})
    try:
        version = upd.read_running_version()
    except Exception as e:
        logger.warning("power board version query failed: %s", e)
        return jsonify({"version": None, "error": str(e)})
    if version is None:
        return jsonify({"version": None,
                        "error": "电源板未上报版本 (需单片机实现 0x11 GET_SW_VERSION 命令)"})
    return jsonify({"version": version})


@api_bp.route("/power_board/upload", methods=["POST"])
@login_required
@roles_allowed(["admin"])
def power_board_upload():
    """
    上传电源板固件 (.bin)
    ---
    tags:
      - Power Board Update
    consumes:
      - multipart/form-data
    parameters:
      - name: file
        in: formData
        type: file
        required: true
    responses:
      200:
        description: 上传成功, 返回 {path, size}
    """
    try:
        file = request.files.get("file")
        if file is None or file.filename == "":
            return jsonify({"error": "未选择文件"}), 400
        result = get_power_board_updater().save_bin(file.stream, file.filename)
        return jsonify({**result,
                        "message": f"固件已上传 ({result['size']} 字节), 点击开始升级"})
    except PowerBoardUpdateError as e:
        return jsonify({"error": str(e)}), 400
    except Exception as e:
        logger.exception("power board upload failed")
        return jsonify({"error": f"上传失败: {e}"}), 500


@api_bp.route("/power_board/start", methods=["POST"])
@login_required
@roles_allowed(["admin"])
def power_board_start():
    """
    开始升级 (后台线程执行, 前端轮询 /power_board/status)
    ---
    tags:
      - Power Board Update
    parameters:
      - name: body
        in: body
        schema:
          type: object
          properties:
            path:
              type: string
              description: 上传接口返回的固件路径; 缺省用最近一次上传
    responses:
      200:
        description: 升级已开始
    """
    try:
        # 升级期间电源板会停止输出, 与系统更新一样要求 Lua 任务空闲
        from web.routes.update import idle_required

        idle_required()
    except RuntimeError as e:
        return jsonify({"error": str(e)}), 409

    try:
        body = request.get_json(silent=True) or {}
        get_power_board_updater().start_update(body.get("path"))
        return jsonify({"message": "电源板升级已开始"})
    except PowerBoardUpdateError as e:
        return jsonify({"error": str(e)}), 409
    except Exception as e:
        logger.exception("power board update start failed")
        return jsonify({"error": f"启动升级失败: {e}"}), 500


@api_bp.route("/power_board/restore", methods=["POST"])
@login_required
@roles_allowed(["admin"])
def power_board_restore():
    """
    从外部 flash 备份区还原 app 固件 (app 损坏时的回退入口)
    ---
    tags:
      - Power Board Update
    responses:
      200:
        description: 还原已开始
    """
    try:
        # 还原期间电源板会停止输出, 与升级一致要求 Lua 任务空闲
        from web.routes.update import idle_required

        idle_required()
    except RuntimeError as e:
        return jsonify({"error": str(e)}), 409

    try:
        get_power_board_updater().start_restore()
        return jsonify({"message": "电源板还原已开始"})
    except PowerBoardUpdateError as e:
        return jsonify({"error": str(e)}), 409
    except Exception as e:
        logger.exception("power board restore start failed")
        return jsonify({"error": f"启动还原失败: {e}"}), 500
