"""Repository checks; Python 3.12+, clang-format/clang-tidy 22.1.8."""
import argparse
import concurrent.futures
import hashlib
import json
import pathlib
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as xml

ROOT = pathlib.Path(__file__).resolve().parents[1]
TOKEN = re.compile(r'//[^\n]*|/\*.*?\*/|(?:0[xX][0-9A-Fa-f\']+|[0-9]+(?:\'[0-9]+)*(?:\.[0-9\']*)?(?:[eEpP][+-]?[0-9\']+)?)[uUlLfF]*|(?:u8|u|U|L)?"(?:\\.|[^"\\])*"|(?:u8|u|U|L)?\'(?:\\.|[^\'\\])*\'|\b[A-Za-z_]\w*\b|\s+|.', re.S)


def canonical_digest(text, identifier_aliases=None):
  """Ignore includes, comments, whitespace and approved identifier casing."""
  text = re.sub(r'^\s*#include[^\n]*', '', text, flags=re.M)
  def convert(match):
    value = match[0]
    if value.startswith(('//', '/*')) or value.isspace():
      return ''
    if re.fullmatch(r'[A-Za-z_]\w*', value):
      if re.match(r'k[A-Z]', value):
        value = value[1:]
      value = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', value)
      value = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', value).lower()
      value = (identifier_aliases or {}).get(value, value)
    return value + '\n'
  return hashlib.sha256(TOKEN.sub(convert, text).encode('utf-8')).hexdigest()


def handwritten():
  return sorted(path for folder in ('src', 'tests') for path in (ROOT / folder).rglob('*')
                if path.suffix in ('.cpp', '.h') and 'generated' not in path.parts)


def structure():
  namespace = {'m': 'http://schemas.microsoft.com/developer/msbuild/2003'}
  project = xml.parse(ROOT / 'build/BubbleFighter.Server.vcxproj')
  expected = set(path.resolve() for path in (ROOT / 'src').rglob('*')
                 if path.suffix in ('.cpp', '.h', '.sql'))
  items = project.findall('.//m:ClCompile', namespace) + project.findall('.//m:ClInclude', namespace) + project.findall('.//m:None', namespace)
  actual = [(ROOT / 'build' / item.attrib['Include'].replace('\\', '/')).resolve()
            for item in items if 'Include' in item.attrib]
  if set(actual) != expected or len(actual) != len(set(actual)):
    raise RuntimeError('Project source list differs from src; run update_project_files.py')
  filters = xml.parse(ROOT / 'build/BubbleFighter.Server.vcxproj.filters')
  filtered = {item.attrib['Include'] for kind in ('ClCompile', 'ClInclude', 'None')
              for item in filters.findall('.//m:' + kind, namespace)}
  if filtered != {item.attrib['Include'] for item in items if 'Include' in item.attrib}:
    raise RuntimeError('Project filters differ from project items')
  guards = set()
  for path in handwritten():
    if not re.fullmatch(r'[a-z0-9_]+\.(cpp|h)', path.name):
      raise RuntimeError('Invalid source filename: ' + str(path))
    text = path.read_text(encoding='utf-8')
    for include in re.findall(r'^#include "([^"]+)"', text, re.M):
      target = ROOT / 'src' / include
      if not target.is_file() or target.relative_to(ROOT / 'src').as_posix() != include:
        raise RuntimeError('Unresolved include: ' + str(path) + ' -> ' + include)
    if path.suffix == '.h':
      guard = 'BUBBLEFIGHTER_SERVER_' + re.sub('[^A-Z0-9]', '_', path.relative_to(ROOT / 'src').as_posix().upper()) + '_'
      if guard in guards or '#ifndef ' + guard not in text or '#define ' + guard not in text or '#endif  // ' + guard not in text:
        raise RuntimeError('Invalid or duplicate guard: ' + str(path))
      guards.add(guard)
  for document in [ROOT / 'README.md', *list((ROOT / 'docs').glob('*.md'))]:
    text = document.read_text(encoding='utf-8')
    links = re.findall(r'\]\(([^)]+)\)', text)
    links += re.findall(r'\b(?:src|href)="([^"]+)"', text)
    for link in links:
      if '://' in link or link.startswith(('#', 'mailto:')):
        continue
      link = link.split('#')[0]
      if not (document.parent / link).exists():
        raise RuntimeError('Broken documentation link: ' + str(document) + ' -> ' + link)
  print('Project items, filters, source paths, guards and documentation links passed')


def manifests():
  for name in ('generated_data_manifest', 'migration_manifest', 'protocol_manifest'):
    path = ROOT / 'tests' / (name + '.json')
    data = json.loads(path.read_text(encoding='utf-8'))
    for entry in data['files']:
      text = (ROOT / entry['path']).read_text(encoding='utf-8')
      digest = hashlib.sha256(text.replace('\r\n', '\n').encode()).hexdigest() if name == 'migration_manifest' else canonical_digest(text, entry.get('identifier_aliases'))
      if digest != entry['sha256']:
        raise RuntimeError('Baseline differs: ' + entry['path'])
    print(name + ': ' + str(len(data['files'])) + ' files passed')
  inventory = json.loads((ROOT / 'tests/inventory_transaction_manifest.json').read_text(encoding='utf-8'))
  current = {}
  for source in (ROOT / 'src/persistence').rglob('*.cpp'):
    text = source.read_text(encoding='utf-8')
    safe = TOKEN.sub(lambda match: ' ' * len(match[0]) if match[0].startswith(('//', '/*', '"', "'")) else match[0], text)
    for match in re.finditer(r'\bMySqlInventoryStore::(\w+)\(', text):
      if match[1] == 'MySqlInventoryStore':
        continue
      opening = text.index('{', match.start())
      depth = 0
      for index in range(opening, len(text)):
        if safe[index] == '{':
          depth += 1
        elif safe[index] == '}':
          depth -= 1
          if depth == 0:
            current[match[1]] = canonical_digest(text[opening:index + 1])
            break
  if current != {entry['method']: entry['sha256'] for entry in inventory['methods']}:
    raise RuntimeError('Inventory transaction method body differs from baseline')
  print('Inventory transactions: ' + str(len(current)) + ' unchanged method bodies passed')
  expected = json.loads((ROOT / 'tests/runtime_literal_manifest.json').read_text(encoding='utf-8'))
  literals = set()
  for path in (ROOT / 'src').rglob('*'):
    if path.suffix not in ('.cpp', '.h'):
      continue
    text = re.sub(r'^\s*#include[^\n]*', '', path.read_text(encoding='utf-8'), flags=re.M)
    literals.update(match[0] for match in TOKEN.finditer(text) if re.match(r'(?:u8|u|U|L)?["\']', match[0]))
  digest = hashlib.sha256(json.dumps(sorted(literals), ensure_ascii=False, separators=(',', ':')).encode('utf-8')).hexdigest()
  if len(literals) != expected['count'] or digest != expected['sha256']:
    raise RuntimeError('Production string/character literals differ from baseline')
  print('Production literals: ' + str(len(literals)) + ' unchanged values passed')


def tool(name):
  executable = shutil.which(name)
  if not executable:
    candidate = pathlib.Path('C:/Program Files/LLVM/bin') / (name + '.exe')
    if candidate.is_file():
      executable = str(candidate)
  if not executable:
    raise RuntimeError(name + ' missing; install LLVM 22.1.8 or add it to PATH')
  version = subprocess.check_output([executable, '--version'], text=True)
  if '22.1.8' not in version:
    raise RuntimeError('Expected LLVM 22.1.8: ' + version.splitlines()[0])
  return executable


def parallel_check(paths, command, label):
  log_dir = ROOT / 'Artifacts/checks' / label
  log_dir.mkdir(parents=True, exist_ok=True)
  def run(item):
    index, path = item
    result = subprocess.run(command(path), cwd=ROOT, capture_output=True)
    output = result.stdout + result.stderr
    (log_dir / (str(index) + '.log')).write_bytes(output)
    if result.returncode:
      return str(path.relative_to(ROOT)), output.decode('utf-8', 'replace')[:1200]
  with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    failures = [failure for failure in pool.map(run, enumerate(paths)) if failure]
  if failures:
    for path, output in failures[:8]:
      print(path, output, sep='\n', file=sys.stderr)
    raise RuntimeError(str(len(failures)) + ' ' + label + ' checks failed; full logs: ' + str(log_dir))
  print(label + ': ' + str(len(paths)) + ' files passed')


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--format', action='store_true')
  parser.add_argument('--fix-format', action='store_true')
  parser.add_argument('--headers', action='store_true')
  parser.add_argument('--naming', action='store_true')
  parser.add_argument('--mysql-root', type=pathlib.Path)
  args = parser.parse_args()
  structure()
  manifests()
  files = handwritten()
  if args.format or args.fix_format:
    executable = tool('clang-format')
    flags = ['-i'] if args.fix_format else ['--dry-run', '--Werror']
    parallel_check(files, lambda path: [executable, *flags, str(path)], 'format')
  if args.headers or args.naming:
    flags = ['-std=c++20', '-fno-delayed-template-parsing', '-I' + str(ROOT / 'src')]
    if args.mysql_root:
      flags.append('-I' + str(args.mysql_root / 'include'))
    if args.headers:
      executable = tool('clang++')
      parallel_check([path for path in files if path.suffix == '.h'],
                     lambda path: [executable, *flags, '-fsyntax-only', '-x', 'c++', str(path)], 'headers')
    if args.naming:
      executable = tool('clang-tidy')
      parallel_check([path for path in files if path.suffix == '.cpp'],
                     lambda path: [executable, str(path), '--warnings-as-errors=readability-identifier-naming', '--', *flags], 'naming')


if __name__ == '__main__':
  try:
    main()
  except Exception as error:
    print(error, file=sys.stderr)
    sys.exit(1)
