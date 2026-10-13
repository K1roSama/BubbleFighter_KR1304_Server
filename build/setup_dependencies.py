"""Prepare the pinned MySQL C API files without installing a database server."""

import hashlib
import json
import pathlib
import shutil
import sys
import urllib.request
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
MYSQL_DISTRIBUTION = 'mysql-8.4.11-winx64'
MYSQL_URL = 'https://cdn.mysql.com/Downloads/MySQL-8.4/' + MYSQL_DISTRIBUTION + '.zip'
MYSQL_SHA256 = 'a492371d687d2bab088b0062581144a0044b8964baefdf4faa579292b423d25c'
DEPENDENCY_ROOT = ROOT / 'Artifacts/Dependencies' / MYSQL_DISTRIBUTION
ARCHIVE = ROOT / 'Artifacts/Downloads' / (MYSQL_DISTRIBUTION + '.zip')
MANIFEST = DEPENDENCY_ROOT / '.download_manifest.json'
REQUIRED_FILES = {
    'include/mysql.h', 'lib/libmysql.lib', 'lib/libmysql.dll',
    'bin/libssl-3-x64.dll', 'bin/libcrypto-3-x64.dll',
    'LICENSE', 'README', 'docs/INFO_BIN', 'docs/INFO_SRC'}


def sha256(path):
  with path.open('rb') as stream:
    return hashlib.file_digest(stream, 'sha256').hexdigest()


def dependency_path(relative):
  path = pathlib.PurePosixPath(relative)
  if path.is_absolute() or '..' in path.parts or '\\' in relative or ':' in relative:
    raise RuntimeError('Invalid dependency path: ' + relative)
  return DEPENDENCY_ROOT.joinpath(*path.parts)


def cache_is_valid():
  if not MANIFEST.is_file():
    return False
  try:
    recorded = json.loads(MANIFEST.read_text(encoding='utf-8'))
    if recorded['archive_sha256'] != MYSQL_SHA256 or recorded['distribution'] != MYSQL_DISTRIBUTION:
      return False
    files = recorded['files']
    if not REQUIRED_FILES.issubset(files):
      return False
    for relative, digest in files.items():
      path = dependency_path(relative)
      if not path.is_file() or sha256(path) != digest:
        return False
    return True
  except (KeyError, OSError, TypeError, ValueError, RuntimeError):
    return False


def main():
  if cache_is_valid():
    print('Verified cached MySQL development files: ' + str(DEPENDENCY_ROOT))
    return
  if not ARCHIVE.is_file():
    ARCHIVE.parent.mkdir(parents=True, exist_ok=True)
    temporary = ARCHIVE.with_suffix('.zip.download')
    print('Downloading ' + MYSQL_URL, flush=True)
    with urllib.request.urlopen(MYSQL_URL, timeout=60) as response, temporary.open('wb') as destination:
      shutil.copyfileobj(response, destination, length=1024 * 1024)
    if sha256(temporary) != MYSQL_SHA256:
      raise RuntimeError('Downloaded MySQL archive SHA-256 differs from the pinned value')
    temporary.replace(ARCHIVE)
  if sha256(ARCHIVE) != MYSQL_SHA256:
    raise RuntimeError('MySQL archive SHA-256 differs from the pinned value: ' + str(ARCHIVE))

  files = {}
  prefix = MYSQL_DISTRIBUTION + '/'
  with zipfile.ZipFile(ARCHIVE) as archive:
    for entry in archive.infolist():
      if entry.is_dir() or not entry.filename.startswith(prefix):
        continue
      relative = entry.filename[len(prefix):]
      if relative not in REQUIRED_FILES and not relative.startswith('include/'):
        continue
      destination = dependency_path(relative)
      destination.parent.mkdir(parents=True, exist_ok=True)
      with archive.open(entry) as source, destination.open('wb') as target:
        shutil.copyfileobj(source, target)
      files[relative] = sha256(destination)
  if not REQUIRED_FILES.issubset(files):
    raise RuntimeError('MySQL archive is missing required files')
  MANIFEST.write_text(json.dumps({
      'distribution': MYSQL_DISTRIBUTION,
      'archive_sha256': MYSQL_SHA256,
      'files': files}, indent=2, sort_keys=True) + '\n', encoding='utf-8')
  print('Prepared ' + str(len(files)) + ' verified MySQL files: ' + str(DEPENDENCY_ROOT))


if __name__ == '__main__':
  try:
    main()
  except Exception as error:
    print(error, file=sys.stderr)
    sys.exit(1)
