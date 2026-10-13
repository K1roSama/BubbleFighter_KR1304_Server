# 开发约定

采用 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)。类型和函数使用 `CamelCase`；变量、参数、结构体公开字段及命名空间使用 `snake_case`；类的私有成员加尾随下划线；常量与枚举值使用 `kCamelCase`。不确定语义的 `value_at_*`、`unknown_value` 和消息编号保留其证据范围。

源码从 `src/` 开始查找，内部 include 使用从该目录开始的完整路径。手写头文件使用路径唯一的 `BUBBLEFIGHTER_SERVER_..._` 保护宏，并能独立编译。源码使用 UTF-8、两空格缩进和 LF；Visual Studio 工程仍由原有 MSBuild 构建。

本项目例外：

- 用户选择普通 C++ 扩展名 `.cpp`，头文件为 `.h`，文件名使用小写下划线。
- 保留现有异常处理及错误传播模型；本轮不迁移为错误返回对象。
- Windows/MySQL API 名称、协议字符串、SQL、配置键、命令行选项、程序名和构建输出目录保持兼容。
- `src/**/generated/` 中的数据表不按手写代码格式化。缺失的生成链见 [重构记录](refactoring.md)。
- 格式器禁止拆分字符串字面量，以便逐项核查现有 SQL、错误文本和协议文本。

格式与命名工具固定为 **LLVM 22.1.8**；检查脚本要求 **Python 3.12 或更高版本**，只使用标准库。安装工具后，在仓库根目录执行：

```powershell
python build/verify.py --format
python build/verify.py --headers --naming --mysql-root 'C:\MySQL'
```

默认检查生产工程项、filters、内部 include、头文件保护宏、文档链接及冻结清单。`--format` 检查 `src/` 和 `tests/` 内全部手写 `.cpp`/`.h`，排除所有 `generated/`；`--fix-format` 在同一范围应用格式。`--headers` 单独解析每个手写头文件；`--naming` 对每个手写实现运行命名检查。完整输出写入被忽略的 `Artifacts/checks/`，控制台只显示有限错误。

生成目录还有独立 `.clang-format` 禁用格式与 include 重排，防止手工执行格式器时误处理大型数据表。Windows 的基础头文件需先于依赖其类型的 SDK 头文件，格式器显式维护这个顺序。

新增生产源码后执行 `python build/update_project_files.py` 更新项目与 filters。测试工程使用同一解决方案，覆盖全部 feature、protocol 和 session 实现，不依赖数据库。

```powershell
msbuild BubbleFighter.Server.sln /m /p:Configuration=Debug /p:Platform=x64 /p:CL_MPCount=4 "/p:MySqlRoot=C:\MySQL"
Artifacts/Tests/x64/Debug/BubbleFighter.Regression.exe tests/compatibility_packets.txt
python tests/runtime_smoke_test.py --configuration Debug
```

Release 使用相应配置与测试输出目录。生产程序运行检查默认验证 Debug 和 Release 的 CLI、回环监听、固定握手及正常退出，不需要数据库，日志位于 `Artifacts/smoke/`。格式和命名检查只是规范的一部分，不能据此声称完全符合 Google 的全部规则；编译及隔离回归也不能代替真实客户端、数据库或联网验收。

修改协议或交易前先补人工固定预期或独立历史样本。冻结清单绑定重构前的 Git 提交，不得用待验证的新实现重写期望值。迁移内容的更改属于兼容边界；为真正的后续迁移另建文件并明确说明。
