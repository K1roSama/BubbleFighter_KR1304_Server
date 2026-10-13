<div align="center">

<img src="docs/assets/banner.png" width="840" alt="泡泡战士与 CatsPub 横幅">

# 🛠 BubbleFighter KR1304 Server 🛠

**面向 BubbleFighter KR1304 客户端的 Windows C++ 服务端实现**

本地账号 · 角色与背包 · 广场与商店 · 多人房间与对局结算

![Windows x64](https://img.shields.io/badge/Windows-x64-0078D4?style=flat-square)
![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=cplusplus&logoColor=white)

[![MIT License](https://img.shields.io/badge/License-MIT-D22128?style=flat-square)](LICENSE)

<img width="100%" src="https://starify.komoridevs.icu/api/starify?owner=K1roSama&repo=BubbleFighter_KR1304_Server" alt="starify" />

[快速上手](#快速上手) · [构建指南](docs/build.md) · [命令行](docs/usage.md#命令行选项) · [文档](#文档与贡献)

<table aria-label="community">
<tr>
<td align="center" width="360">
  <a href="https://qm.qq.com/q/fAcNMJVyuI">
    <img src="docs/assets/qq.svg" height="42" alt="QQ" /><br />
    <strong>加入 泡泡战士 K1ro 服社区</strong>
  </a>
  </td>
<td align="center" width="360">
  <a href="https://afdian.com/a/K1roSama1337">
    <img src="docs/assets/afdian.svg" height="42" alt="afdian" /><br />
    <strong>通过 爱发电 支持我们！</strong>
  </a>
</td>
<td align="center" width="360">
  <a href="https://ko-fi.com/k1rosama">
    <img src="docs/assets/ko-fi.svg" height="42" alt="ko-fi" /><br />
    <strong>请我们喝杯咖啡！</strong>
  </a>
</td>
</tr>
</table>

---

<table aria-label="CatsPub AI">
<tr>
<td align="center" width="840">
  <a href="https://catspub.com">
    <img src="docs/assets/ai_banner.png" alt="AI" /><br />
    <strong>本项目由 Catspub 中转站获得支持</strong>
  </a><br />
  <sub>稳定 · 快速 · 用户至上</sub>
</td>
</tr>
</table>

</div>

## 感谢两位的慷慨付出

向本次促进开源推进的人表达感谢 ^ ^

没有你们的弱智发言，我们也不会加快工作进度。

**希望你们清楚，你们永远要为自己的发言负责，无论处于什么身份和地位。**

<details>
<summary>开源原因 | <b>⚠️多图预警/降智预警⚠️</b> </summary>

### 懒惰、贪婪

<blockquote>
  <p>"作为一名开发，我不应该考虑每个不同地区玩家的延迟最大程度缓解？现在的你，跟你之前的话，越发会发现你就是一个小丑，毕竟我可不需要披着别人的身份在网上装大王，最后再说一句脏话，你们两个傻逼就只会在自己那里舔韩国人当大王了。"</p>
  <p align="right">
    <sub>—— <strong>K1ro</strong> · 2026-10-12</sub>
  </p>
</blockquote>

<img src="docs/assets/sb2.png" alt="sb2" />

### 傲慢、暴怒

<blockquote>
  <p>"你跟那个喜欢伪造他人的小丑一样，总把自己摆在一个很高的位置上，结果你也不知道自己什么能力，没有一个人看得起你，只会没有任何事实依据的情况下想办法去抹黑他人。你的行为跟你说的话倒是符合了你从其他某国学来的特征，我不接受你的道歉，同时还你一句闹麻了。"</p>
  <p align="right">
    <sub>—— <strong>K1ro</strong> · 2026-10-12</sub>
  </p>
</blockquote>

<img src="docs/assets/sb.jpg" alt="sb" />

</details>

## 项目介绍

这是一个面向 KR1304 客户端的 Windows 控制台服务端项目。源码覆盖本地账号验证、角色与库存持久化、广场、商店、练习，以及普通 PVP 房间和对局结算。数据库连接使用 MySQL C API，目前限定为同机 TCP 与 TLS 连接。

**仓库不包含客户端或启动器。** 运行游戏需要另行准备匹配的 KR1304 客户端，并接入本项目的本地登录票据；具体要求见[运行与接入指南](docs/usage.md#签发登录票据并接入客户端)。

| 功能 | 源码覆盖的流程 |
| --- | --- |
| 本地账号 | 创建账号、密码验证、签发与消费一次性登录票据 |
| 角色与菜单 | 角色创建、已有角色加载、等级／经验／Lucci／Cash 数据与菜单初始化 |
| 背包与装备 | 普通库存与待领取 Cash 道具、装备切换、主武器预设与道具使用 |
| 广场与聊天 | 在线成员、移动与装备状态同步、场景聊天 |
| 商店 | 商品目录、购买、余额更新与道具领取 |
| 练习 | 练习进入与会话处理 |
| 多人房间 | 普通 PVP 房间创建、列表、加入／离开、准备与房主变更 |
| 战斗与结果 | 加载与倒计时、移动与攻击、伤害、泡封／爆泡／救援、复活、结果展示与返回房间 |
| 对局持久化 | 结算记录、角色统计与奖励写入、待完成结算重试与恢复 |
| 本地规则 | 可配置升级、经验／金币奖励、结果评级和勋章阈值 |

## 快速上手

完成[构建与数据库准备](docs/build.md)后，在仓库根目录运行。`Local/Database.ini` 是本文约定的配置示例路径，需要自行创建：

```powershell
$server = '.\Artifacts\Bin\x64\Release\BubbleFighter.Server.exe'
& $server --listen-port 40000 --database-config .\Local\Database.ini
```

成功启动时输出：

```text
BubbleFighter.Server
Listening on 127.0.0.1:40000
```

`40000` 是示例端口，客户端连接配置应与之对应。接下来按[运行与接入指南](docs/usage.md)创建本地账号，在另一个终端签发登录票据，再通过已完成接入的客户端连接服务端。

查看命令帮助：

```powershell
& $server --help
```

局域网监听、账号命令、票据有效期与全部参数见[命令行选项](docs/usage.md#命令行选项)。

## 构建与部署

以下要求直接来自当前项目文件：

| 项目 | 当前配置 | 依据 |
| --- | --- | --- |
| 平台与程序类型 | Windows 控制台程序，`Debug / Release`、`x64` | [项目配置](build/BubbleFighter.Server.vcxproj) |
| C++ 与工具链 | C++20、MSVC `v143`、Windows SDK `10.0.26100.0` | [项目配置](build/BubbleFighter.Server.vcxproj) |
| 网络与密码处理 | Windows Winsock 与 CNG，链接 `Ws2_32.lib`、`Bcrypt.lib` | [项目配置](build/BubbleFighter.Server.vcxproj)、[链接配置](build/mysql.props) |
| 数据库开发文件 | MySQL C API：`mysql.h`、`libmysql.lib`、`libmysql.dll`，通过 `MySqlRoot` 指定 | [MySQL 属性表](build/mysql.props) |

准备上述工具链和 MySQL 开发文件后，在 Visual Studio **Developer PowerShell** 中执行。下方 `C:\MySQL` 是示例，需替换为包含所需文件的实际目录：

```powershell
git clone https://github.com/K1roSama/BubbleFighter_KR1304_Server.git
cd .\BubbleFighter_KR1304_Server
$mysqlRoot = 'C:\MySQL'
msbuild .\BubbleFighter.Server.sln /m /p:Configuration=Release /p:Platform=x64 "/p:MySqlRoot=$mysqlRoot"
```

也可在安装了上述工具链的 Visual Studio 中打开 [BubbleFighter.Server.sln](BubbleFighter.Server.sln)，选择 `x64` 后构建。构建要求由工具集和 SDK 配置决定。MySQL 路径、构建时复制的 DLL 和数据库准备步骤见[构建指南](docs/build.md)。

首次运行前需要手动应用八个数据库迁移，并配置运行账号；程序不会自动初始化数据库。数据库配置必须使用 `127.0.0.1`、TCP 和 TLS。完成后返回[快速上手](#快速上手)启动服务端。

## 使用限制

- **版本与模式**：当前面向 KR1304，房间创建入口开放普通 PVP；大型房间及其他创建模式未开放，不承诺兼容其他客户端版本或全部游戏内容。
- **登录接入**：本地账号和票据流程需要客户端配合，官方 Passport 不能直接用于此服务端；仅启动可执行文件不会自动连接游戏。
- **本地策略**：数据库结构、初始角色数值、升级和奖励规则为本项目的本地设计，不代表原版服务端实现。
- **状态保存**：角色、库存和对局结算等数据写入 MySQL；房间、广场和在线会话保存在进程内，重启后需要重新连接。
- **客户端及启动器**：我们暂不提供该类客户端，可以从网上搜索到资源，某垃圾服垃圾启动器源码：https://mab.to/t/4PHOUFUYoLa/as1

具体规则及默认值见[运行与接入指南](docs/usage.md)。

## 文档与贡献

| 文档 | 内容 |
| --- | --- |
| [构建指南](docs/build.md) | 环境要求、编译、数据库迁移与配置、常见问题 |
| [运行与接入指南](docs/usage.md) | 账号、登录票据、监听、客户端接入与命令行参数 |
| [对局规则示例](config/examples/match_rules.ini) | 升级、奖励、评级与勋章配置 |
| [数据库迁移](src/persistence/migrations) | 本地账号、角色、库存、武器预设与结算表定义 |
| [开发约定](docs/development.md) | 代码规范、项目例外及检查入口 |
| [重构与验证](docs/refactoring.md) | 职责调整、兼容依据及未验收范围 |

欢迎通过 [Issues](https://github.com/K1roSama/BubbleFighter_KR1304_Server/issues) 反馈问题，或提交 Pull Request。反馈时请附上客户端版本、服务端提交版本、复现步骤和相关日志；配置与日志中的密码及登录票据请先移除。

## 许可证

本仓库的许可证为 [MIT License](LICENSE)。
