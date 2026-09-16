"""Run a command on the LubanCat board via SSH (password auth).

Usage:
    python tools/ssh_cat.py "uname -a"
    python tools/ssh_cat.py --put local_file remote_path
    python tools/ssh_cat.py --get remote_file local_path
"""
import sys

import paramiko

HOST, USER, PW = "192.168.10.185", "cat", "temppwd"


def main():
    cli = paramiko.SSHClient()
    cli.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    cli.connect(HOST, username=USER, password=PW, timeout=10)

    args = sys.argv[1:]
    if args and args[0] == "--put":
        sftp = cli.open_sftp()
        sftp.put(args[1], args[2])
        sftp.close()
        print(f"uploaded {args[1]} -> {HOST}:{args[2]}")
    elif args and args[0] == "--get":
        sftp = cli.open_sftp()
        sftp.get(args[1], args[2])
        sftp.close()
        print(f"downloaded {HOST}:{args[1]} -> {args[2]}")
    else:
        cmd = args[0] if args else "uname -a && whoami"
        _, stdout, stderr = cli.exec_command(cmd, timeout=120)
        out = stdout.read().decode("utf-8", "replace")
        err = stderr.read().decode("utf-8", "replace")
        rc = stdout.channel.recv_exit_status()
        if out:
            print(out, end="" if out.endswith("\n") else "\n")
        if err:
            print("[stderr]", err.rstrip(), file=sys.stderr)
        sys.exit(rc)

    cli.close()


if __name__ == "__main__":
    main()
