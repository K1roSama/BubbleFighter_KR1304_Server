# 构建与数据库准备

以下命令在仓库根目录的 Visual Studio Developer PowerShell 中执行。构建要求来自 [BubbleFighter.Server.vcxproj](../build/BubbleFighter.Server.vcxproj) 和 [MySql.props](../build/mysql.props)。

## 构建要求

| 项目 | 当前配置 |
| --- | --- |
| 平台 | Windows 控制台程序，仅配置 `x64` |
| 构建配置 | `Debug`、`Release` |
| C++ 标准 | C++20，项目值为 `stdcpp20` |
| 编译工具集 | MSVC `v143` |
| Windows SDK | `10.0.26100.0` |
| 系统库 | `Ws2_32.lib`、`Bcrypt.lib` |
| 外部开发文件 | MySQL C API 的头文件、导入库与运行时文件，通过 `MySqlRoot` 指定 |

安装包含上述工具集和 SDK 的 Visual Studio / MSBuild 环境。[setup_dependencies.py](../build/setup_dependencies.py) 可下载固定版本的 MySQL 开发文件，核查 SHA-256 后提取头文件、库、运行时及发行说明。

## 编译

使用固定依赖和自动定位 MSBuild 的入口：

```powershell
python .\build\setup_dependencies.py
.\build\build.ps1 -Configuration Release
```

Python 要求 3.12 或更高版本。GitHub Actions 的编译、测试和发布流程见 [CI 说明](ci.md)。

将下面的 `C:\MySQL` 替换为实际提供 MySQL 开发文件的目录：

```powershell
$mysqlRoot = 'C:\MySQL'
msbuild .\BubbleFighter.Server.sln /m /p:Configuration=Release /p:Platform=x64 "/p:MySqlRoot=$mysqlRoot"
```

调试构建将 `Configuration=Release` 改为 `Configuration=Debug`。也可以在 Visual Studio 中打开 [解决方案](../BubbleFighter.Server.sln)，选择 `x64` 和所需配置；使用自定义依赖路径时，在启动 VS 前设置环境变量 `MySqlRoot`。

`MySqlRoot` 默认指向 `Artifacts/Dependencies/mysql-8.4.11-winx64`，准备脚本写入此目录。也可自行提供开发文件并覆盖路径。属性表的错误提示要求 MySQL 8.4 x64 ZIP 安装，实际检查的是以下文件是否存在：

| 相对于 `MySqlRoot` 的路径 | 构建用途 |
| --- | --- |
| `include/mysql.h` | C API 头文件，构建前检查 |
| `lib/libmysql.lib` | 链接输入，构建前检查 |
| `lib/libmysql.dll` | 构建前检查，构建后复制到输出目录 |
| `bin/libssl-3-x64.dll` | 构建后复制到输出目录 |
| `bin/libcrypto-3-x64.dll` | 构建后复制到输出目录 |

上述两个 `bin` 下的 DLL 是现有复制步骤的文件要求；项目没有为它们配置独立的源码构建或安装流程。

可执行文件为 `Artifacts/Bin/x64/<配置>/BubbleFighter.Server.exe`，中间文件位于 `Artifacts/Obj/x64/<配置>/BubbleFighter.Server`。运行时保留输出目录中复制的 DLL。

## 数据库迁移

运行账号、角色、库存和结算功能前，需要自行准备同机 MySQL 服务、数据库及运行账号。数据库账号应能读写这些功能使用的表。程序不会自动创建数据库或应用迁移。

使用自己的数据库管理工具，在选定数据库中依次执行以下仓库文件。顺序依据迁移注释和表之间的依赖：

| 顺序 | 文件 | 作用 |
| --- | --- | --- |
| 1 | [CreateLocalAccounts.sql](../src/persistence/migrations/create_local_accounts.sql) | `Accounts`、`LoginTickets` |
| 2 | [CreateCharacters.sql](../src/persistence/migrations/create_characters.sql) | `Characters` |
| 3 | [AddCharacterStatistics.sql](../src/persistence/migrations/add_character_statistics.sql) | 角色等级、经验、Lucci、Cash 字段 |
| 4 | [CreateInventorySlots.sql](../src/persistence/migrations/create_inventory_slots.sql) | `InventorySlots` |
| 5 | [CreateCashInventorySlots.sql](../src/persistence/migrations/create_cash_inventory_slots.sql) | `CashInventorySequence`、`CashInventorySlots` |
| 6 | [CreateWeaponPresets.sql](../src/persistence/migrations/create_weapon_presets.sql) | `WeaponPresetState`、`WeaponPresetSlots` |
| 7 | [ExpandCashInventoryUsage.sql](../src/persistence/migrations/expand_cash_inventory_usage.sql) | 扩展 Cash 库存的使用类型约束 |
| 8 | [CreateMatchSettlements.sql](../src/persistence/migrations/create_match_settlements.sql) | `MatchSettlements` |

这些迁移按一次性执行编写。已有数据库只应用尚未执行的迁移，重复执行会遇到已有表、字段或约束错误。

启用数据库配置后，服务端在监听前通过 [MySqlMatchStore.cpp](../src/persistence/my_sql_match_store.cpp) 查询结算表结构。缺少 `MatchSettlements` 或其所需字段时，启动失败。

## 数据库配置

配置由 [my_sql_connection.cpp](../src/persistence/my_sql_connection.cpp) 读取，通过 `--database-config` 指定路径。仓库提供了无真实凭据的 [database.ini 示例](../config/examples/database.ini)；复制到 `Local/Database.ini` 后填写自己的数据库账号。

本文统一使用 `Local/Database.ini` 作为示例路径，先创建目录：

```powershell
New-Item -ItemType Directory -Path .\Local -Force | Out-Null
```

自行创建 `Local/Database.ini`，使用 UTF-8 无 BOM 保存。下面的端口、账号、密码和库名按实际环境填写；`3306` 是示例端口，`BubbleFighterServer` 是源码的默认库名：

```ini
[client]
host=127.0.0.1
port=3306
protocol=TCP
user=YOUR_DATABASE_USER
password=YOUR_DATABASE_PASSWORD
ssl-mode=REQUIRED
database=BubbleFighterServer
```

| 配置项 | 当前解析要求 |
| --- | --- |
| 节名 | `[client]` 或 `[mysql]` |
| `host` | 必须为 `127.0.0.1` |
| `port` | 必填，范围 `1..65535` |
| `protocol` | 必须为 `TCP` |
| `user`、`password` | 必填且非空，填写实际数据库凭据 |
| `ssl-mode` | 必须为 `REQUIRED`，数据库服务需支持 TLS |
| `database` | 可省略，默认 `BubbleFighterServer`；填写时不能为空 |

配置文件最多 16 KiB，键名区分大小写，重复或未知键会被拒绝。允许空行、以 `#` 或 `;` 开头的整行注释；当前解析器不支持远程数据库地址或其他 MySQL 配置选项。

完成后按[运行与接入指南](usage.md)创建本地游戏账号并启动服务端。
