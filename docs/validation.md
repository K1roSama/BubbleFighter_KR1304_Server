# 重构验证记录

验证日期：2026-10-13。重构前提交为 `6fcf2a64f046f5d2e16d1d1289697f09fe342147`，当时工作区干净且没有测试工程。全部实现保留用户指定的 `.cpp` 扩展名。

## 环境与构建基线

- Windows x64；Visual Studio Community 2026 18.7.1；MSBuild 来自安装目录 `D:/Program Files/Microsoft Visual Studio/18/community`。
- 保留 MSVC `v143` 工具集、C++20 和 SDK `10.0.26100.0`，本机编译器为 `14.44.35207`。
- LLVM `22.1.8` 用于格式、命名及头文件检查；检查入口仅使用 Python 标准库，要求 Python 3.12+。
- 默认依赖路径中的 MySQL 8.4.11 未安装，首次基线构建因此失败。本次显式使用本机 `D:/Program Files/MySQL/MySQL Server 8.0` 的开发文件，不代表 MySQL 8.4 验收。
- 原 Debug 基线遇到 PDB C1090，以临时 `/FS /Z7` 完成构建。最终工程使用正常 PDB 设置，并加入 `/FS` 和独立的编译器 PDB 路径；最终构建不使用该临时环境覆盖。

基线先冻结报文，再依次整理路径/格式、改名、拆分职责和提取共同实现。行为敏感调整前先运行相应回归；最后补充了使用原提交头文件及原 Debug 目标文件的独立登录、菜单和购买探针。测试中的期望值没有由重构后的实现重新生成。

## 最终执行命令与结果

以下命令在仓库根目录执行。为复核本机环境，列出实际工具位置；其他机器可使用相同版本的 PATH 工具。

```powershell
$msbuildExe = 'D:/Program Files/Microsoft Visual Studio/18/community/MSBuild/Current/Bin/MSBuild.exe'
$pythonExe = 'C:/Users/27231/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'

& $msbuildExe BubbleFighter.Server.sln /m /p:Configuration=Debug /p:Platform=x64 /p:CL_MPCount=4 `
  '/p:MySqlRoot=D:\Program Files\MySQL\MySQL Server 8.0' /v:quiet /nologo `
  '/flp:logfile=Artifacts/final_debug.log;verbosity=minimal'
& ./Artifacts/Tests/x64/Debug/BubbleFighter.Regression.exe tests/compatibility_packets.txt

& $msbuildExe BubbleFighter.Server.sln /m /p:Configuration=Release /p:Platform=x64 /p:CL_MPCount=4 `
  '/p:MySqlRoot=D:\Program Files\MySQL\MySQL Server 8.0' /v:quiet /nologo `
  '/flp:logfile=Artifacts/final_release.log;verbosity=minimal'
& ./Artifacts/Tests/x64/Release/BubbleFighter.Regression.exe tests/compatibility_packets.txt

& $pythonExe tests/runtime_smoke_test.py

& $pythonExe build/verify.py --format --headers --naming `
  --mysql-root 'D:/Program Files/MySQL/MySQL Server 8.0'
& $pythonExe build/verify.py
git diff --check
git diff --cached --check
```

| 检查 | 结果 |
| --- | --- |
| Debug x64 解决方案构建 | 成功，退出码 0 |
| Release x64 解决方案构建 | 成功，退出码 0 |
| Debug 回归 | `79 checks passed; 16 frozen packet batches`，退出码 0 |
| Release 回归 | 同上，退出码 0 |
| Debug/Release 生产程序运行 | 各通过 7 项启动及回环握手检查，退出码 0 |
| 手写代码格式 | 408 个 `.cpp`/`.h` 通过；生成数据排除 |
| 头文件自包含 | 200 个手写头文件通过独立解析 |
| 标识符命名 | 208 个手写实现通过配置中的命名规则 |
| 工程及引用 | 工程项、filters、文件名、内部 include、保护宏及文档链接通过 |
| 原协议实现 | 129 份实现通过历史 token 清单 |
| 生成数据 | 11 份头文件通过历史 token 清单，保留数值、顺序、精度及类型语义 |
| 历史迁移 | 8 份迁移内容通过统一行尾后的散列检查 |
| 库存持久化方法 | 9 个原有方法的函数体通过历史清单 |
| 生产字符串与字符字面量 | 原有 6740 种值保持一致 |
| Git 空白检查 | 工作区及暂存区通过 |

协议与生成数据清单允许路径、空白及已批准标识符适配；协议清单另列明 4 个循环局部变量的改名。它们提供源码兼容证据，不代替完整协议实机验收。清单依据和去重说明见 [重构记录](refactoring.md)。

构建输出仍为 `Artifacts/Bin/x64/Debug/BubbleFighter.Server.exe` 和 `Artifacts/Bin/x64/Release/BubbleFighter.Server.exe`。没有新增生产可执行程序。检查详细日志位于被忽略的 `Artifacts/final_*.log` 和 `Artifacts/checks/`。

生产程序运行检查通过 `tests/runtime_smoke_test.py` 执行：实际启动两个配置的 EXE，验证 `--help`、无参数帮助及非法端口退出码；使用 `--listen-port 0 --once` 在回环地址监听；连接并检查 258 字节固定问候、完整加密握手响应及断开后的正常退出。请求由人工协议布局构造，响应取重构前冻结批次，线上加密向量使用原提交中的表预先固定，不调用当前 C++ 编解码器生成预期。运行日志保存在 `Artifacts/smoke/{Debug,Release}/`。该检查不使用数据库或真实游戏客户端。

## 回归范围与限制

固定字节及响应批次覆盖受影响协议、资料初始化和菜单默认值；三种交易的成功/失败路径及通知顺序；动态配置 checksum 命中/未命中/缺失；密码限制、票据有效期与一次消费接口契约；玩家登录、菜单和购买分派；房间及对局生命周期边界；奖励阈值、历史 V1 结算和 SHA-256 已知答案。

本机没有该项目的专用测试数据库配置、凭据、迁移库及真实客户端。账号和角色存储在回归中使用隔离替身；真实数据库提交、故障回滚、并发锁、数据库票据失效和客户端联机未验证。没有进行实际性能测量。

生成头注明的 `Tools/Generate*.py` 和原始输入仍缺失。本轮保留内嵌数据及有效来源注释；后续需找回生成链并先复现原数据。没有确认需要单独修复的功能缺陷，也没有借重构改变现有行为。格式和命名检查通过不意味着满足 Google 规范的所有规则。

## CI 与发布包补充验证（2026-10-13）

新增 [手动发布工作流](../.github/workflows/release.yml) 和 [CI 使用说明](ci.md)。此前重构使用本机 MySQL 8.0；本节改用准备脚本指定的 MySQL 8.4.11 开发文件。官方 ZIP 的 MD5 与官方提供值一致，下载得到的 SHA-256 固定在脚本中。仅准备客户端开发文件，没有安装或修改数据库服务。

```powershell
& $pythonExe build/setup_dependencies.py
& $pythonExe build/setup_dependencies.py # 再次运行，验证缓存文件
./build/build.ps1 -Configuration Debug
./Artifacts/Tests/x64/Debug/BubbleFighter.Regression.exe tests/compatibility_packets.txt
./build/build.ps1 -Configuration Release
./Artifacts/Tests/x64/Release/BubbleFighter.Regression.exe tests/compatibility_packets.txt
& $pythonExe tests/runtime_smoke_test.py
./build/package_release.ps1 -Version vci-smoke-20261013
Expand-Archive -LiteralPath Artifacts/Distribution/BubbleFighter.Server-vci-smoke-20261013-Windows-x64.zip -DestinationPath Artifacts/PackageSmoke-local -Force
& $pythonExe tests/runtime_smoke_test.py --executable Artifacts/PackageSmoke-local/BubbleFighter.Server.exe
./Artifacts/Tools/actionlint/actionlint.exe .github/workflows/release.yml
& $pythonExe build/verify.py
git diff --check
```

| 检查 | 结果 |
| --- | --- |
| MySQL 8.4.11 依赖准备 | 提取并验证 25 个文件；隔离目录的冷缓存首次下载及再次运行的缓存检查均通过 |
| Debug/Release x64 构建 | 两套成功，均 0 警告、0 错误 |
| Debug/Release 回归 | 各通过 79 项检查、16 组冻结报文 |
| Debug/Release 启动与握手 | 各通过 7 项运行检查 |
| 发布 ZIP | 30 个文件，含 MSVC/MySQL/OpenSSL 运行时；8 份迁移和配置示例内容保持一致；归档 SHA-256 匹配 |
| 解压后的发布程序 | 通过 7 项运行检查，退出码 0 |
| 工作流语法 | actionlint 1.7.12 通过 |
| PowerShell 语法 | 两个构建脚本及七个工作流脚本块通过解析 |
| 标签校验 | 新标签接受，非法标签拒绝；隔离 Git 目录中已有标签的同提交接受、不同提交拒绝；项目标签未改动 |
| 工程、文档和兼容清单 | 通过；生产 C++、协议和数据库迁移没有修改 |

打包脚本及工作流在本机完成了构建、测试和打包路径验证；GitHub runner 尚未实际执行，远程标签与 Release 未创建。MySQL 8.4.11 客户端库的本地构建和基础网络运行通过，不代表真实数据库业务或真实游戏客户端验收。

## 自动编译与失败诊断验证（2026-10-13）

新增 [Build and Test](../.github/workflows/build.yml)，在分支 push 和 pull request 时检查 Debug/Release，失败后始终生成摘要并上传日志。构建脚本新增 UTF-8 独立错误日志，诊断报告保留编译器原始说明及可定位的文件、行号和列号；详见 [CI 说明](ci.md)。

```powershell
./build/build.ps1 -Configuration Debug
./Artifacts/Tests/x64/Debug/BubbleFighter.Regression.exe tests/compatibility_packets.txt
./build/build.ps1 -Configuration Release
./Artifacts/Tests/x64/Release/BubbleFighter.Regression.exe tests/compatibility_packets.txt
& $pythonExe tests/runtime_smoke_test.py
& $pythonExe -m unittest discover -s tests -p build_report_test.py
./Artifacts/Tools/actionlint/actionlint.exe .github/workflows/build.yml .github/workflows/release.yml
& $pythonExe build/verify.py
git diff --check
```

| 检查 | 结果 |
| --- | --- |
| Debug/Release 构建与回归 | 两套构建成功；各通过 79 项检查及 16 组冻结报文 |
| Debug/Release 启动与握手 | 各通过 7 项检查 |
| 诊断报告样例 | 8 项测试通过：MSVC 位置、相对路径、重复诊断、链接错误、MSBuild 错误、注释转义、摘要范围及失败阶段 |
| 真实编译失败 | 隔离故障工程第 2 行产生 `C2065`；执行实际工作流编译脚本块，进程返回非零，错误日志、错误注释及任务摘要均保留未声明标识符和正确行号 |
| 成功构建摘要 | 两套摘要报告编译成功和零错误，向模拟 `GITHUB_STEP_SUMMARY` 追加内容正确 |
| 工作流和脚本 | 两份工作流通过 actionlint 1.7.12；六个新增脚本块及改动后的构建脚本通过 PowerShell 解析 |
| 工程与兼容清单 | 通过；生产代码及迁移未修改 |

故障探针仅在被忽略的 `Artifacts/compile-failure-*` 中创建小型测试工程，原项目保持可编译。诊断步骤成功不会覆盖原编译步骤的失败状态。GitHub runner 上的 push/PR 自动触发和最终页面展示仍需推送后验证，本次未创建远程任务或发布。
