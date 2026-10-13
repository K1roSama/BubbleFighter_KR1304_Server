"""Report MSVC/MSBuild diagnostics without changing the build's failure status."""

import argparse
import dataclasses
import html
import ntpath
import os
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
MAX_DISPLAY = 12
MAX_DETAIL = 700
ERROR = re.compile(
    r'^\s*(?:\d+>)?(?P<origin>.*?)\s*:\s*'
    r'(?:(?:fatal|致命)\s+)?(?:error|错误)\s+'
    r'(?P<code>[A-Z]+\d+)\s*:\s*(?P<message>.*)$', re.I)
LOCATION = re.compile(r'^(?P<path>.+)\((?P<line>\d+)(?:,(?P<column>\d+))?\)$')
PROJECT = re.compile(r'\s+\[(?P<path>[^\]]+\.(?:vcxproj|sln|proj))\]\s*$', re.I)
STAGES = (
    ('依赖准备', 'DEPENDENCIES_OUTCOME'), ('编译', 'COMPILE_OUTCOME'),
    ('回归测试', 'REGRESSION_OUTCOME'), ('启动与握手', 'SMOKE_OUTCOME'),
    ('工程及兼容检查', 'VERIFY_OUTCOME'))


@dataclasses.dataclass(frozen=True)
class Diagnostic:
  origin: str
  code: str
  message: str
  file: str | None = None
  line: int | None = None
  column: int | None = None


def repository_path(path, root, project=None):
  path = path.strip('"')
  if not ntpath.isabs(path):
    parent = ntpath.dirname(project) if project else str(root)
    path = ntpath.join(parent, path)
  try:
    common = ntpath.commonpath([str(root), path])
    if ntpath.normcase(common) != ntpath.normcase(str(root)):
      return None
    return ntpath.relpath(path, str(root)).replace('\\', '/')
  except ValueError:
    return None


def parse_diagnostics(lines, root=ROOT):
  diagnostics = []
  seen = set()
  for line in lines:
    match = ERROR.match(line.rstrip('\r\n'))
    if not match:
      continue
    origin = match['origin'].strip()
    message = match['message']
    project = PROJECT.search(message)
    if project:
      message = message[:project.start()]
    file = row = column = None
    location = LOCATION.match(origin)
    if location:
      file = repository_path(location['path'], root, project['path'] if project else None)
      row = int(location['line'])
      column = int(location['column']) if location['column'] else None
    item = Diagnostic(origin, match['code'].upper(), message, file, row, column)
    if item not in seen:
      seen.add(item)
      diagnostics.append(item)
  return diagnostics


def command_escape(value, property=False):
  text = str(value).replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
  if property:
    text = text.replace(':', '%3A').replace(',', '%2C')
  return text


def annotation(diagnostic, configuration):
  properties = ['title=' + command_escape(configuration + ' ' + diagnostic.code, property=True)]
  if diagnostic.file and diagnostic.line:
    properties.extend(['file=' + command_escape(diagnostic.file, property=True),
                       'line=' + str(diagnostic.line)])
    if diagnostic.column:
      properties.append('col=' + str(diagnostic.column))
  message = diagnostic.origin + ': ' + diagnostic.message
  return '::error ' + ','.join(properties) + '::' + command_escape(message[:MAX_DETAIL])


def summary(configuration, diagnostics, outcomes):
  lines = ['## Windows x64 / ' + configuration, '', '| 阶段 | 结果 |', '| --- | --- |']
  for name, key in STAGES:
    lines.append('| ' + name + ' | ' + outcomes.get(key, 'unknown') + ' |')
  lines.extend(['', '### 编译诊断', ''])
  if diagnostics:
    lines.append('发现 ' + str(len(diagnostics)) + ' 个不同的编译/链接/构建错误；以下保留编译器原始说明：')
    for item in diagnostics[:MAX_DISPLAY]:
      text = item.origin + ': ' + item.code + ': ' + item.message
      lines.extend(['', '<pre>' + html.escape(text[:MAX_DETAIL]) + '</pre>'])
    if len(diagnostics) > MAX_DISPLAY:
      lines.extend(['', '摘要仅显示前 ' + str(MAX_DISPLAY) + ' 个错误，完整诊断请下载日志。'])
  elif outcomes.get('COMPILE_OUTCOME') == 'success':
    lines.append('编译成功，没有 MSBuild 错误诊断。')
  elif outcomes.get('COMPILE_OUTCOME') == 'failure':
    lines.append('编译步骤失败，未匹配到结构化诊断；请查看编译控制台日志及全文日志末尾。')
  else:
    lines.append('编译没有完成；请查看上表中的失败、跳过或取消阶段及其日志。')
  lines.extend(['', '在本次运行的 Artifacts 下载 `build-logs-' + configuration +
                '-<run_id>-<run_attempt>`，查看 `build-' + configuration +
                '.errors.log`、`build-' + configuration + '.log` 和各阶段日志。', ''])
  return '\n'.join(lines)


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--configuration', required=True, choices=['Debug', 'Release'])
  arguments = parser.parse_args()
  logs = ROOT / 'Artifacts/Logs'
  error_log = logs / ('build-' + arguments.configuration + '.errors.log')
  full_log = logs / ('build-' + arguments.configuration + '.log')
  path = error_log if error_log.is_file() else full_log
  diagnostics = []
  if path.is_file():
    with path.open(encoding='utf-8-sig', errors='replace') as stream:
      diagnostics = parse_diagnostics(stream)
  outcomes = {key: os.environ.get(key) or 'unknown' for _, key in STAGES}
  report = summary(arguments.configuration, diagnostics, outcomes)
  logs.mkdir(parents=True, exist_ok=True)
  (logs / ('summary-' + arguments.configuration + '.md')).write_text(report, encoding='utf-8')
  destination = os.environ.get('GITHUB_STEP_SUMMARY')
  if destination:
    with pathlib.Path(destination).open('a', encoding='utf-8') as stream:
      stream.write(report)
  for item in diagnostics[:MAX_DISPLAY]:
    print(annotation(item, arguments.configuration))
  print(arguments.configuration + ': ' + str(len(diagnostics)) + ' distinct build diagnostics; summary saved')


if __name__ == '__main__':
  main()
