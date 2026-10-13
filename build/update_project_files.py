"""Update Visual Studio source items and filters without changing build settings."""
import pathlib
import uuid
import xml.etree.ElementTree as xml

root = pathlib.Path(__file__).resolve().parents[1]
namespace = 'http://schemas.microsoft.com/developer/msbuild/2003'
xml.register_namespace('', namespace)
tag = lambda name: '{' + namespace + '}' + name
project_path = root / 'build/BubbleFighter.Server.vcxproj'
project = xml.parse(project_path)
for group in list(project.getroot().findall(tag('ItemGroup'))):
  if not group.attrib.get('Label'):
    project.getroot().remove(group)
items = xml.Element(tag('ItemGroup'))
filters = xml.Element(tag('Project'), ToolsVersion='4.0')
directories = set()
entries = []
for path in sorted((root / 'src').rglob('*')):
  kind = {'.cpp': 'ClCompile', '.h': 'ClInclude', '.sql': 'None'}.get(path.suffix)
  if not kind:
    continue
  include = '..\\' + str(path.relative_to(root)).replace('/', '\\')
  xml.SubElement(items, tag(kind), Include=include)
  folder = str(path.parent.relative_to(root / 'src')).replace('/', '\\')
  if folder != '.':
    parts = folder.split('\\')
    directories.update('\\'.join(parts[:i]) for i in range(1, len(parts) + 1))
  entries.append((kind, include, folder))
definitions = xml.SubElement(filters, tag('ItemGroup'))
for folder in sorted(directories):
  element = xml.SubElement(definitions, tag('Filter'), Include=folder)
  xml.SubElement(element, tag('UniqueIdentifier')).text = '{' + str(uuid.uuid5(uuid.NAMESPACE_URL, 'bubble-fighter/' + folder)).upper() + '}'
for kind in ('ClCompile', 'ClInclude', 'None'):
  group = xml.SubElement(filters, tag('ItemGroup'))
  for item_kind, include, folder in entries:
    if item_kind != kind:
      continue
    element = xml.SubElement(group, tag(kind), Include=include)
    if folder != '.':
      xml.SubElement(element, tag('Filter')).text = folder
project.getroot().insert(len(project.getroot()) - 3, items)
xml.indent(project, space='  ')
project.write(project_path, encoding='utf-8', xml_declaration=True)
xml.indent(filters, space='  ')
xml.ElementTree(filters).write(project_path.with_suffix('.vcxproj.filters'), encoding='utf-8', xml_declaration=True)
print('Updated', len(entries), 'project items and', len(directories), 'filters')
