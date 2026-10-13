# 编译与发布工作流

## 自动编译检查

[Build and Test](../.github/workflows/build.yml) 在所有分支 push 和 pull request 时自动运行，也支持手动触发。Debug/Release x64 分别执行构建、回归、启动与握手及工程兼容检查；一种配置失败后，另一种配置仍继续检查。同一分支或 PR 的新提交会取消旧的自动检查。

编译出错时，Actions 中对应配置的任务显示失败，并提供：

- **Annotations**：错误码、原始错误说明及可定位的文件、行号和列号。
- **Summary**：各阶段结果与前 12 个不同错误；区分依赖准备、编译及测试的失败。
- **Artifacts**：`build-logs-<Debug或Release>-<run_id>-<run_attempt>`，保留 14 天；内含 MSBuild 全文日志、独立错误日志、编译控制台日志、摘要及各阶段测试日志。

例如 `C2065` 会保留未声明标识符的名称和所在行，`LNK2019` 会保留未解析符号，`MSB8020` 会保留缺失工具集的说明。没有匹配到结构化错误时，摘要会提示查看控制台和全文日志；诊断脚本不猜测原因。错误摘要写入 `GITHUB_STEP_SUMMARY`，实际失败状态来自原构建或测试步骤。

自动检查使用 `contents: read`。发布工作流仍通过下方的手动入口执行。

## 手动发布

[Build and Release](../.github/workflows/release.yml) 参考 PKNPacker 的手动发布流程。提交到默认分支后，在 GitHub 的 **Actions → Build and Release → Run workflow** 选择要构建的分支或标签，并填写：

- `tag`：必填，例如 `v1.0.0`。新标签在所有检查通过后随 Release 创建；已有标签必须指向本次选择的提交。
- `release_name`：可选，默认 `BubbleFighter.Server <tag>`。
- `prerelease`、`draft`：预发布和草稿开关，默认关闭。

工作流仅手动触发。它使用 `windows-2025` 的 Visual Studio/MSVC v143 和 Windows SDK `10.0.26100.0`，保持原有解决方案、C++20、程序名及构建输出位置。GitHub Actions 固定到 checkout v6、setup-python v6、cache v5 和 upload-artifact v7 对应的提交；发布使用 runner 自带的 `gh` 和 `GITHUB_TOKEN`，需要 `contents: write`。

## 发布检查

1. 验证标签格式与对应提交，避免发布与构建内容不一致。
2. 用 Python 3.12 准备固定版本的 MySQL 8.4.11 开发依赖，并验证归档 SHA-256。缓存命中后仍核查提取文件的散列。
3. 执行 `build/verify.py`，检查工程引用、保护宏、文档和兼容清单。
4. 构建 Debug/Release x64，分别执行冻结报文回归，再对两套生产程序执行启动及回环握手检查。
5. 打包 Release，生成 `.sha256`，解压并再次检查包内程序的启动和握手，上传 Actions 产物和诊断日志。
6. 全部成功后创建 GitHub Release 并上传 ZIP 和校验文件。

此次工作流运行结构及兼容清单检查；完整 LLVM 格式、头文件和命名检查仍按 [开发约定](development.md) 使用 22.1.8 执行。数据库与真实游戏客户端不参与发布检查。

## 发布包

产物名称为 `BubbleFighter.Server-<tag>-Windows-x64.zip`，另附同名 `.zip.sha256`。包内包含：

- `BubbleFighter.Server.exe`、MySQL/OpenSSL DLL 和 x64 MSVC CRT DLL。
- `config/examples/` 中的配置示例、`database/migrations/` 中的八份历史迁移。
- 启动说明 `README.txt`、项目 `LICENSE` 和 MySQL 发行包中的许可及构建来源说明。

真实数据库凭据、开发机 `Local/` 配置、测试程序和 PDB 不进入发布包。首次使用按包内说明准备数据库并应用迁移；已有数据库仅应用尚未执行的迁移。完整配置及客户端说明见 [运行指南](usage.md)。

## 本地复现

在仓库根目录执行，要求 Python 3.12+、Visual Studio/MSVC v143 及项目指定 SDK：

```powershell
python build/setup_dependencies.py
python build/verify.py
./build/build.ps1 -Configuration Debug
./Artifacts/Tests/x64/Debug/BubbleFighter.Regression.exe tests/compatibility_packets.txt
./build/build.ps1 -Configuration Release
./Artifacts/Tests/x64/Release/BubbleFighter.Regression.exe tests/compatibility_packets.txt
python tests/runtime_smoke_test.py
./build/package_release.ps1 -Version v1.0.0
```

构建和打包脚本接受 `-MySqlRoot`。使用自备 MySQL 目录打包时，该目录还须包含官方发行包的 `LICENSE`、`README`、`docs/INFO_BIN` 和 `docs/INFO_SRC`。打包自动查找 Visual Studio 的 x64 `Microsoft.VC143.CRT`，也可通过 `-RuntimeDirectory` 指定同类目录。

编译日志在 `Artifacts/Logs/`，启动日志在 `Artifacts/smoke/`，发布文件在 `Artifacts/Distribution/`。即使构建或测试失败，已有诊断日志仍会作为 Actions 产物上传；Release 步骤须等待前置检查全部成功。

自动检查的编译错误日志为 `Artifacts/Logs/build-<配置>.errors.log`，全文日志为 `build-<配置>.log`。可在本地运行诊断样例测试及报告脚本：

```powershell
python -m unittest discover -s tests -p build_report_test.py
$env:COMPILE_OUTCOME = 'success' # 根据真实构建结果填写 failure 或 success
python build/report_build.py --configuration Debug
```

## 本次验证

工作流的 actionlint、PowerShell 语法及本地构建、测试和打包结果见 [验证记录](validation.md)。本地检查不能代替 GitHub runner 执行；工作流需推送后才能在 Actions 中运行，本次没有远程创建标签或 Release。
