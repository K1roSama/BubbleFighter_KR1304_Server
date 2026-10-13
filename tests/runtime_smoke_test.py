"""Run production startup and loopback handshake checks without a database."""

import argparse
import pathlib
import re
import socket
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
NO_WINDOW = getattr(subprocess, 'CREATE_NO_WINDOW', 0)
GREETING = b'\x04\x00' + bytes(256)

# Fixed wire vectors, independent of the current C++ implementation.
# Request: message 1, initial value 1256, 22 empty UTF-16 fields, zero
# intermediate value. Its header/XOR bytes were assembled manually.
# Encryption uses the tables from original commit 6fcf2a64f046f5d2e16d1d1289697f09fe342147.
# Response: the pre-refactor frozen initial_handshake batch, encrypted with
# that same historical table and the server-to-client starting offsets.
INITIAL_REQUEST = bytes.fromhex(
    'bf623297c28b327945a429c268ac86a0b5b25668382648655f30cda5e855fe5542d1f'
    '7cd7e92a5c13d9be78841c7677c7f833e57723418ab87cf2e9d4e9b79ba6cda8dcdf'
    '19dc16b42830cf2805be263f0b45f6b59b15fd98d62765e11a6628a59a35f6332e5c17d')
INITIAL_RESPONSE = bytes.fromhex(
    '206b87d0308e4ee479fa6ce88d8af15dc5bf42c41df08021e223f0b5')


def require(condition, message):
  if not condition:
    raise RuntimeError(message)


def run_cli(executable, arguments, label, log_dir):
  result = subprocess.run([str(executable), *arguments], cwd=ROOT,
                          capture_output=True, timeout=10,
                          creationflags=NO_WINDOW)
  (log_dir / (label + '.stdout.log')).write_bytes(result.stdout)
  (log_dir / (label + '.stderr.log')).write_bytes(result.stderr)
  return result


def receive_exactly(connection, length):
  received = bytearray()
  while len(received) < length:
    block = connection.recv(length - len(received))
    require(block, 'Connection closed before the fixed response was complete')
    received.extend(block)
  return bytes(received)


def check_executable(executable, label):
  require(executable.is_file(), 'Production executable is missing: ' + str(executable))
  log_dir = ROOT / 'Artifacts/smoke' / label
  log_dir.mkdir(parents=True, exist_ok=True)

  help_result = run_cli(executable, ['--help'], 'help', log_dir)
  require(help_result.returncode == 0 and not help_result.stderr and
          b'Usage: BubbleFighter.Server --listen-port' in help_result.stdout,
          '--help did not load and exit successfully')
  default_result = run_cli(executable, [], 'default', log_dir)
  require(default_result.returncode == 0 and not default_result.stderr and
          default_result.stdout == help_result.stdout,
          'Default invocation did not match help output')
  invalid_result = run_cli(executable, ['--listen-port', '65536'],
                           'invalid_port', log_dir)
  require(invalid_result.returncode == 2 and
          invalid_result.stderr.strip() == b'Invalid listen port.',
          'Invalid port did not produce the expected error and exit code')

  stdout_path = log_dir / 'server.stdout.log'
  stderr_path = log_dir / 'server.stderr.log'
  with stdout_path.open('wb') as stdout, stderr_path.open('wb') as stderr:
    process = subprocess.Popen(
        [str(executable), '--listen-port', '0', '--once',
         '--idle-timeout-seconds', '5'],
        cwd=ROOT, stdout=stdout, stderr=stderr, creationflags=NO_WINDOW)
    try:
      deadline = time.monotonic() + 15
      port = None
      while time.monotonic() < deadline:
        output = stdout_path.read_bytes()[:10000]
        match = re.search(rb'Listening on 127\.0\.0\.1:(\d+)', output)
        if match:
          port = int(match[1])
          break
        require(process.poll() is None, 'Server exited before listening; inspect ' + str(log_dir))
        time.sleep(0.025)
      require(port is not None and 0 < port <= 65535, 'Server did not start listening within 15 seconds')
      with socket.create_connection(('127.0.0.1', port), timeout=10) as connection:
        require(receive_exactly(connection, len(GREETING)) == GREETING,
                'The 258-byte plaintext greeting differs')
        connection.sendall(INITIAL_REQUEST)
        require(receive_exactly(connection, len(INITIAL_RESPONSE)) == INITIAL_RESPONSE,
                'The complete encrypted handshake response differs from the historical vector')
        connection.shutdown(socket.SHUT_WR)
      require(process.wait(timeout=10) == 0,
              '--once did not exit successfully after the peer disconnected')
      require(not stderr_path.read_bytes(), 'Server reported a runtime error; inspect ' + str(log_dir))
    finally:
      if process.poll() is None:
        process.terminate()
        try:
          process.wait(timeout=5)
        except subprocess.TimeoutExpired:
          process.kill()
          process.wait(timeout=5)
  print(label + ': 7 runtime smoke checks passed (CLI, listen, greeting, handshake, exit)')


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  target = parser.add_mutually_exclusive_group()
  target.add_argument('--configuration', nargs='+', choices=['Debug', 'Release'],
                      default=['Debug', 'Release'])
  target.add_argument('--executable', type=pathlib.Path,
                      help='Check an extracted release executable instead of build outputs')
  arguments = parser.parse_args()
  if arguments.executable:
    check_executable(arguments.executable.resolve(), 'Package')
  else:
    for configuration in arguments.configuration:
      executable = ROOT / 'Artifacts/Bin/x64' / configuration / 'BubbleFighter.Server.exe'
      check_executable(executable, configuration)


if __name__ == '__main__':
  try:
    main()
  except Exception as error:
    print(error, file=sys.stderr)
    sys.exit(1)
