# 可维护性重构记录

基线提交：`6fcf2a64f046f5d2e16d1d1289697f09fe342147`。实施前工作区干净，421 个受版本控制文件，没有测试工程。采用 Google C++ Style Guide，按用户后续要求使用 `.cpp`。[开发约定](development.md) 记录工具及项目例外。

## 目录与职责

```text
src/
  application/                 启动、命令行和依赖装配
  network/                     TCP 生命周期与心跳
  sessions/                    玩家跨业务状态、菜单及业务分派
  protocol/
    codec/ login/ characters/ inventory/ shop/
    scenes/ rooms/ battle/ configuration/ unresolved/
  features/                    账号、角色、库存、商店、房间、战斗等业务
    <所属业务>/generated/      保留内嵌编译的数据表
  persistence/
    inventory/                 库存 SQL 行操作与交易方法
    migrations/                保留内容和文件名的历史迁移
  crypto/                      Windows SHA-256 底层机制
build/                         工程、属性表、工程项更新和检查入口
docs/                          构建、运行、开发及重构说明
tests/                         同体系回归工程、固定字节及基线清单
config/examples/               数据库和自定义奖励规则示例
```

玩家会话成为 `sessions::PlayerSession`。`ProcessCore` 保留分派顺序和外层拒绝/异常边界，商店/库存、练习、房间/广场及练习状态分别有实现入口；资源与状态仍由同一个会话持有。`RoomGameSession` 的生命周期、弹道/伤害、角色动作及泡泡交互分文件，注册表锁内的状态所有权保持不变。

MySQL 库存共享行加载和持久化位于 `inventory_rows.*`，装备/预设/道具写入及购买/领取交易分别有实现入口。9 个原有存储方法的函数体由历史清单验证；余额扣减、锁顺序、容量检查、写入、提交点和失败回滚没有借此次整理改写。

奖励配置解析、经验/奖励计算及资料包区间绑定分文件；评级和勋章继续由已有 `ResultAwardRules` 负责。`MatchRewardPolicy` 保留冻结结算格式所需的聚合和字段，配置键及 V1/V2 语义不变。

## 关键名称

| 原名 | 当前名及职责 |
| --- | --- |
| `LocalAuthentication` | `AccountAuthentication`，账号验证及票据 |
| `GameLoginSession` / `GameLoginState` | `PlayerSession` / `PlayerSessionState`，玩家会话 |
| `LocalMenuData::Create` | `CreateMenuSnapshot`，菜单快照组装函数 |
| `LocalShopCatalog::Create` | `CreateShopCatalogResponse`，目录协议响应；资源目录仍是 `ShopCatalog` |
| `LocalDynamicConfiguration::Respond` | `RespondContentConfiguration`，内容配置响应函数 |
| `LocalMatchRules` | `MatchRewardPolicy`，自定义奖励及升级；评级/勋章在 `ResultAwardRules` |

命名空间、字段和参数按规范调整大小写。`--local-login`、客户端本机/本地玩家语义、配置键、SQL 名称、错误文本及未知偏移含义保留。自定义规则与原版已确认行为的区分注释保留。

## 合并与兼容依据

- 菜单及结算重复的 FighterInfo 基线统一由 characters 的 `CreateCharacterProfileBaseline` 提供，保持 `{1000, 1}` 与 6 个能力默认值。
- 普通/Cash 交易的库存归属、角色状态及目录校验使用 inventory 的独立谓词，调用方保留检查顺序及异常文本。待领取校验保持自己的职责。
- 商店使用角色基线及装备视图直接构造响应，消除完整菜单、场景和预设的多余构造及库存副本。Cash 购买保留原装备验证，即使它不发送装备包。返回快照继续拥有自己的数据。
- SHA-256 只共享 BCrypt 底层调用及句柄关闭；账号保留 1024 字节限制，目录保留协议负载限制，各调用方保留错误文本。PBKDF2 和随机票据机制不改。
- 拆分产生的无用 include 根据 Clang 符号引用清理。没有仅凭无直接调用删除协议实现。

普通购买：资料 `105` → 库存 `802` → 装备 `806` → 购买结果 `816`。Cash 购买：资料 `105` → 待领取 `810` → 购买结果 `816`。Cash 领取：库存 `802` → 装备 `806` → 领取结果 `835` → 待领取刷新 `810`；失败路径有各自固定预期。

## 验证依据

`compatibility_packets.txt` 在逻辑重构前冻结 16 组完整响应，测试没有重新捕获入口。人工字节样例额外验证 TCP 校验、普通购买请求及历史 V1 冻结结算，避免仅依靠往返测试。

协议清单验证 129 份实现的源码 token 顺序；允许 include 路径、空白和标识符大小写适配，以及清单中明确列出的 4 个集合循环变量改名（消除成员遮蔽），保留数值、字符串、类型及表达式。生成清单验证 11 份数据头的同类 token 等价。8 个迁移验证统一行尾后的 UTF-8 SHA-256。库存事务清单验证 9 个原有方法的函数体，而非只查 SQL 字符串。

原有 6740 种生产代码字符串/字符字面量全部保留，包括 SQL、协议文本、配置键、命令行与错误文本。额外用重构前 Debug 目标文件及该提交头文件核查目录修订值和完整登录→菜单→购买路径，固定预期已写入回归测试。

回归覆盖资料/菜单默认值；交易成功、余额不足、无效商品、已拥有、容量不足、非法归属和槽分配；动态配置缺失/命中/未命中；密码、票据有效期传参及一次消费存储契约；握手/拒绝；房间加载、倒计时、结束、报告等待、结算确认及返回时限；奖励阈值、历史版本和 SHA-256 已知答案。

## 生成链与未验收范围

重新核查后仍未找到 `CombatSceneData.h`、`ShopStockCatalogData.h` 或其他生成头所列的 `Tools/Generate*.py`。现有仓库无法恢复这些脚本及其原输入。保留来源/指纹注释，不声称恢复了生成工具；后续应找回脚本及锁定输入，先复现原数据再生成。

本机有 MySQL 8.0 开发文件及运行服务，但没有本项目测试配置、凭据及已迁移的专用测试库；也没有用于真实联机验收的客户端。本轮不对现有数据库进行未经配置的业务写入。实际提交、故障回滚、并发锁及真实票据失效检查未运行；票据隔离测试验证应用与存储接口契约。

默认 `MySqlRoot` 指向未安装的 MySQL 8.4.11，首次基线因此失败。显式指定本机 8.0 后建立编译基线；不能宣称已通过 MySQL 8.4 实机验收。初始 Debug 出现 PDB C1090；基线以临时 `/Z7` 完成，后续项目加入 `/FS` 及明确的 PDB 路径。

尚未确认需要作为独立功能修复的缺陷。动态配置缺少 checksum 时不返回类别、Cash 购买不立即发物品及历史自定义奖励规则等现状均保留。没有性能测量，不宣称性能收益。最终执行命令与结果见 [验证记录](validation.md)。
