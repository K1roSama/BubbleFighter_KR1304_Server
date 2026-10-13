"""Check fixed diagnostic examples and reporting of failed CI stages."""

import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'build'))
import report_build


class BuildReportTests(unittest.TestCase):
  def test_compiler_location_and_duplicate(self):
    text = (str(ROOT / 'src/main.cpp') + '(12,3): error C2065: '
            "'missing_name': undeclared identifier [" +
            str(ROOT / 'build/BubbleFighter.Server.vcxproj') + ']')
    errors = report_build.parse_diagnostics([' 2>' + text, text])
    self.assertEqual(len(errors), 1)
    self.assertEqual((errors[0].file, errors[0].line, errors[0].column),
                     ('src/main.cpp', 12, 3))
    self.assertEqual(errors[0].code, 'C2065')
    self.assertEqual(errors[0].message, "'missing_name': undeclared identifier")

  def test_relative_location_uses_project_directory(self):
    text = ('..\\src\\main.cpp(7): error C2143: missing semicolon [' +
            str(ROOT / 'build/BubbleFighter.Server.vcxproj') + ']')
    error = report_build.parse_diagnostics([text])[0]
    self.assertEqual((error.file, error.line, error.column), ('src/main.cpp', 7, None))

  def test_linker_diagnostic_without_source_location(self):
    errors = report_build.parse_diagnostics([
        'packet.obj : error LNK2019: unresolved external symbol MissingSymbol referenced in function main',
        'LINK : fatal error LNK1120: 1 unresolved externals'])
    self.assertEqual([error.code for error in errors], ['LNK2019', 'LNK1120'])
    self.assertIsNone(errors[0].file)
    self.assertIn('MissingSymbol', report_build.annotation(errors[0], 'Release'))
    self.assertIn('1 unresolved externals', report_build.annotation(errors[1], 'Release'))

  def test_msbuild_dependency_error_and_localized_prefix(self):
    errors = report_build.parse_diagnostics([
        'MSBuild : error MSB1009: Project file does not exist.',
        'MSBuild : 错误 MSB8036: 找不到 Windows SDK。'])
    self.assertEqual([error.code for error in errors], ['MSB1009', 'MSB8036'])

  def test_external_toolchain_location_does_not_annotate_repository(self):
    root = pathlib.Path('C:/project')
    error = report_build.parse_diagnostics([
        'C:/Program Files/VS/Microsoft.Cpp.targets(42,5): error MSB8020: '
        'The build tools for v143 cannot be found.'], root)[0]
    self.assertIsNone(error.file)
    self.assertNotIn('file=', report_build.annotation(error, 'Debug'))

  def test_annotation_escapes_control_characters_and_properties(self):
    error = report_build.Diagnostic('source', 'C2065', 'bad%\n::notice::injected',
                                    'src/name,part.cpp', 2, 3)
    command = report_build.annotation(error, 'Debug')
    self.assertIn('file=src/name%2Cpart.cpp,line=2,col=3', command)
    self.assertIn('bad%25%0A::notice::injected', command)
    self.assertEqual(command.count('\n'), 0)

  def test_summary_escapes_markup_and_limits_output(self):
    errors = [report_build.Diagnostic('file', 'C2065', '<script>' + str(index))
              for index in range(report_build.MAX_DISPLAY + 2)]
    text = report_build.summary('Debug', errors, {'COMPILE_OUTCOME': 'failure'})
    self.assertEqual(text.count('<pre>'), report_build.MAX_DISPLAY)
    self.assertIn('&lt;script&gt;', text)
    self.assertIn('14 个', text)

  def test_failure_without_structured_diagnostic_and_skipped_compile(self):
    errors = report_build.parse_diagnostics(['file.cpp(3): warning C4100: unused argument'])
    self.assertEqual(errors, [])
    failed = report_build.summary('Debug', errors, {'COMPILE_OUTCOME': 'failure'})
    self.assertIn('编译步骤失败', failed)
    skipped = report_build.summary('Debug', errors, {
        'DEPENDENCIES_OUTCOME': 'failure', 'COMPILE_OUTCOME': 'skipped'})
    self.assertIn('| 依赖准备 | failure |', skipped)
    self.assertIn('编译没有完成', skipped)
    self.assertNotIn('编译成功', skipped)


if __name__ == '__main__':
  unittest.main()
