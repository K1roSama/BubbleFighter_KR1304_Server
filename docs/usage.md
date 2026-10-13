# 运行与客户端接入

先完成[构建与数据库准备](build.md)。以下 PowerShell 命令在仓库根目录执行；`Local/Database.ini`、账号 `player01`、监听端口 `40000` 均为示例，需要按自己的环境配置。

命令与默认值依据 [ServerApplication.cpp](../src/application/server_application.cpp) 和 [LocalAccountCommand.cpp](../src/application/local_account_command.cpp)。

## 查看帮助

```powershell
$server = '.\Artifacts\Bin\x64\Release\BubbleFighter.Server.exe'
& $server --help
```

不带参数也只显示帮助。当前帮助文本仍包含 `full game login pending`；本文记录现有命令和协议入口，客户端接入需要另行实现与验证。

## 创建本地账号

```powershell
& $server --create-account player01 --database-config .\Local\Database.ini
```

程序显示 `Password:` 后输入游戏账号密码，控制台不回显。成功时输出 `Local account created.`。

[LocalAuthentication.cpp](../src/features/authentication/account_authentication.cpp) 将账号名限制为 3–32 个 ASCII 字母、数字或下划线，并将英文字母转为小写。[CredentialCrypto.cpp](../src/features/authentication/credential_crypto.cpp) 对创建账号的密码要求为 12–1024 个 UTF-8 字节。

此命令创建本地游戏账号。INI 中的 `user` 与 `password` 是数据库连接凭据，角色创建由客户端消息流程处理。

## 启动服务端

```powershell
& $server --listen-port 40000 --database-config .\Local\Database.ini
```

按 [TcpServer.cpp](../src/network/tcp_server.cpp) 的输出，成功监听后的提示为：

```text
BubbleFighter.Server
Listening on 127.0.0.1:40000
```

默认监听地址为 `127.0.0.1`。局域网连接时，可指定服务端本机的 IPv4 地址，或使用 `0.0.0.0` 监听所有 IPv4 网卡：

```powershell
& $server --listen-address 0.0.0.0 --listen-port 40000 --database-config .\Local\Database.ini
```

客户端使用服务端机器的实际地址和对应端口连接。游戏监听地址与数据库 `host` 分别由不同配置控制；数据库连接仍限定为本机 `127.0.0.1`。

仅测试连接和握手时，可省略数据库配置。例如让系统选择可用端口，并在处理一个连接后退出：

```powershell
& $server --listen-port 0 --once
```

实际端口以 `Listening on ...` 输出为准。省略数据库配置时，账号登录入口没有配置票据验证与角色存储。

## 签发登录票据并接入客户端

保持服务端运行，在另一个 PowerShell 窗口进入仓库根目录。确认 `Local` 目录已创建后执行：

```powershell
$server = '.\Artifacts\Bin\x64\Release\BubbleFighter.Server.exe'
$ticketFile = Join-Path .\Local ('LoginTicket-' + [guid]::NewGuid().ToString('N') + '.txt')
& $server --local-login player01 --database-config .\Local\Database.ini --ticket-file $ticketFile
```

输入游戏账号密码后，成功提示为：

```text
Local login succeeded; a one-use ticket was written to the requested private file.
```

票据有效期在 [LocalAuthentication.h](../src/features/authentication/account_authentication.h) 中定义为 **300 秒**。数据库通过 [MySqlAccountStore.cpp](../src/persistence/my_sql_account_store.cpp) 检查到期时间和已消费状态；同一票据只能成功消费一次。

`--ticket-file` 的目标文件必须不存在，父目录需要已创建。示例用随机文件名满足新文件要求。重新登录时重新签发票据。

客户端接入依据 [LoginRequest.h](../src/protocol/login/login_request.h) 和 [GameLoginSession.cpp](../src/sessions/player_session.cpp)：登录消息 `0x0003` 的 `NexonPassport` 字段应携带票据文件的完整内容，并按该字段的 UTF-16 字符串格式发送。服务端从此字段读取本地票据，再验证对应账号。

仓库没有提供客户端、启动器或自动接入补丁。上面的命令完成票据签发，客户端连接配置及票据传入方式需要自行完成。

## 自定义对局规则

仓库提供了 [LocalMatchRules.example.ini](../config/examples/match_rules.ini)。复制并编辑后，通过 `--match-rules` 加载。执行启动命令前，先停止此前监听同一端口的服务端：

```powershell
Copy-Item .\config\examples\match_rules.ini .\Local\MatchRules.ini
& $server --listen-port 40000 --database-config .\Local\Database.ini --match-rules .\Local\MatchRules.ini
```

规则由 [LocalMatchRules.cpp](../src/features/rooms/match_reward_policy.cpp) 和 [ResultAwardRules.cpp](../src/features/rooms/result_award_rules.cpp) 解析并校验：

- 使用示例已有的键，按 `Key=无符号十进制整数` 填写；不加空格、符号或行尾注释，不重复键，使用 UTF-8 无 BOM 保存。
- 允许空行和以 `#` 开头的整行注释，读取大小上限为 16 KiB。
- 评级阈值按 SSS 到 E 严格递减；同类勋章阈值按 Beginner、Expert、Master 严格递增。
- 未传入规则文件或省略某个键时，使用代码默认值；修改文件后需要重启服务端。

规则控制本地升级、经验／Lucci 奖励及结果评级、勋章。这些数值在源码和示例中被标注为本地策略。

## 命令行选项

| 选项 | 当前行为 |
| --- | --- |
| `--help` | 单独使用时显示帮助 |
| `--listen-port <0..65535>` | 启动监听时必填；`0` 由系统选择可用端口 |
| `--listen-address <IPv4>` | 默认 `127.0.0.1`；支持本机 IPv4 或 `0.0.0.0` |
| `--database-config <path>` | 指定数据库配置；监听命令可省略，本地账号命令必填 |
| `--idle-timeout-seconds <1..600>` | 连接空闲超时，默认 30 秒 |
| `--match-rules <path>` | 启动时读取本地对局规则 |
| `--once` | 处理一个连接后退出 |
| `--create-account <username>` | 本地创建账号，须作为第一个参数，后接账号名和数据库配置 |
| `--local-login <username>` | 验证本地账号并签发票据，须作为第一个参数 |
| `--ticket-file <new-path>` | `--local-login` 必填，指定尚不存在的票据文件路径 |

账号命令接受的附加参数由 `LocalAccountCommand` 单独解析；不能与监听命令的选项组合使用。

| 退出码 | 当前含义 |
| --- | --- |
| `0` | 帮助、账号命令或监听流程正常完成 |
| `1` | 捕获到运行或校验异常；账号名／密码校验异常也使用此值 |
| `2` | 命令行格式或监听参数数值无效、缺少必填参数 |
| `3` | 账号已存在，或本地登录验证失败；仅账号命令使用 |

本文示例已按源码核对，未执行真实客户端联机验收。功能范围见 [README 使用限制](../README.md#使用限制)。
