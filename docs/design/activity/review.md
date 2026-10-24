# 错题本与同类推荐（review）设计

> 状态：已实现（第一期：Codeforces 全量题单；第二期：牛客竞赛定点查询；
> 第三期：手动收录与按算法查找，覆盖 Codeforces / 牛客竞赛 / 洛谷）。
> 完成后请更新 [README.md](../README.md) 索引中的状态。
> 公共约定见 [conventions.md](conventions.md)；需求优先级见 [../../requirements.md](../../requirements.md)。

## 1. 背景与目标

现有「数据总览」回答的是「练了多少」（解题数、热力图、近期提交），回答不了
「哪里练得不好」。选手真正需要的是：把散落在各平台的没做出来的题、以及做了
很多次才过的题挑出来，再给几条同类型的练习。

本功能是 activity 的**子功能**，不新开功能域：

- 事实来源仍是 `data/user/<userid>/activity/submissions/*.jsonl`，不新增采集、
  不改提交结构；
- 只读聚合，不做写操作，因此不涉及原子写与并发写约定；
- 「同类推荐」目前支持 Codeforces（全量题单）与牛客竞赛（题库定点查询）；
  其余平台只列错题、不展示推荐入口——是否支持由 adapter 是否声明
  `PROBLEMSET` / `PROBLEM_LOOKUP` 决定，前端不硬编码平台清单。

## 2. 总体形态

### 2.1 产品口径

| 概念 | 定义 |
| --- | --- |
| 未通过（unsolved） | 该 `(platform, problem_key)` 全部提交中从未出现 `AC` |
| 挣扎题（struggled） | 有过 `AC`，但**首次 AC 之前**的失败提交数 ≥ `STRUGGLING_FAILS`（3） |
| 收录（collected） | 用户手动加入、且提交记录里**没有**该题 `AC` 的题（第三期新增） |
| 计入失败 | `WA / TLE / MLE / RE / OLE / UNAC / UKE`（`CE` 属编译错误、`JG` 属评测中，均不计） |
| 题目身份 | `(platform, problem_key)`——不做跨平台同题合并 |

一次都没交过的题不会自动出现（无数据）；`CE` 不计入失败，避免「本地环境没配对」
把题目误判成不会做。

「收录」解决的是**没有提交数据也想记一笔**的场景：题面来自别处（队友发的题、
比赛讲解里看到的题），只知道链接。它与前两类的关系是：

- 三类状态**互斥**，优先级 `unsolved` / `struggled` > `collected`——同一题一旦
  有提交聚合结果，就以提交为准，手动项被隐藏（不删除，保留着，等提交数据被清理
  或换账号时自然复现）；
- 手动项一旦在提交里出现 `AC`，就不再出现在列表中（`collected` 的定义就是
  「还没过」），也不需要用户手动清理；
- 手动项 `fail_count` 恒为 0（没有提交可数），`last_attempt_at` 取 `added_at`，
  因此刚收录的题排在列表最前，符合「我刚刚决定要补这道」的心理顺序。

### 2.2 关键决策

- **不做独立功能域**。提交数据由 `modules/activity/store.py` 独占读取，新域直接
  读 jsonl 会打破「功能间依赖单向、模块零耦合」（[../conventions.md](../conventions.md)）；
  同理不新建 `features/review/`，前端复用 activity 的 `api` / `store` / 平台元数据。
- **不改提交模型**。标签、rating 区间属于「题库」数据，与「我交过什么」正交；
  塞进 `Submission` 会让每行 jsonl 冗余膨胀，且平台标签会随时间变化。推荐所需的
  题目画像走独立缓存。
- **题单缓存为可选能力，不阻断错题本**。没有 `PROBLEMSET` / `PROBLEM_LOOKUP`
  能力的平台、缓存拉取失败、离线启动，错题列表都照常工作，推荐区显示不可用。
- **推荐只在平台内进行，不跨平台互推**。各平台标签体系互不相同（CF 的
  `dp` / `graphs` 对牛客的「线性dp」/「图论」，语义与粒度都对不齐），拿 A 平台
  错题去推 B 平台题目会大幅误推，故 `recommend()` 只在同一 `platform` 内取候选。
- **懒加载推荐**。展开某行时才请求推荐，不随列表一次算全站——列表是毫秒级的
  本地读取，推荐要读题单缓存（数千条）。
- **收录是写操作，单独落盘**。提交 jsonl 是同步结果的**派生缓存**（可整份删除重建），
  用户手写的清单不能被下一次同步冲掉，因此放独立文件 `review-manual.json`，
  与提交流水互不写入；列表读取时才做合并。
- **标签只在查询时现取，不做快照**。收录项不保存 `tags`：平台标签会改，且存下来
  就要处理「本地标签 vs 平台标签」两套事实。展开推荐时按 `problem_key` 重新查一次
  画像（与现有推荐取数完全相同的路径），代价是可接受的定点请求。
- **按算法查找是「搜题 → 收录」的入口，不是错题本的标签筛选**。错题本里那几十条
  题做本地标签过滤价值很低，反而要为每条错题都查一次画像；真正的需求是「我最近
  在练二分，给我几道没做过的二分题」，所以做成独立面板并复用同一个收录接口。

被否决的方案：

- **用大模型按题面推荐**：与「本地、离线可用」的非功能需求冲突，延迟与费用不可控；
- **自建全站题库**：等于维护另一个产品，且与「轻量、依赖越少越好」相反；
- **跨平台互推**（CF 的 `graphs` ↔ 洛谷的「图论」）：标签体系对不齐，误推代价高于收益；
- **把标签写进提交 jsonl**：历史数据需全量重刷，且与「平台标签随时可能变化」冲突；
- **把收录项塞进提交 jsonl**（伪造一条 `submission_id=manual:*` 的提交）：会污染
  解题数、热力图、连续天数等所有统计口径，且同步合并按 `submission_id` 去重、
  语义上根本不是提交；
- **收录时把题名/难度写死**：题名会改、难度会调、比赛题会改名，写死只会在半年后
  让列表显示过期信息。收录只存「定位该题所需的最小字段」，展示字段查不到时退回
  存下来的那一份；
- **在错题本内做标签筛选**：需要为列表里每条题都拉一次画像（几十次请求），
  收益远低于独立搜题面板。

## 3. 数据模型与存储

### 3.1 派生视图（不落盘）

错题列表完全由提交实时聚合，服务层不缓存、不落盘：

```python
ProblemRecord        # 单个题目在全部提交中的汇总（review.py 输出）
{
  platform, problem_key,
  problem_name, problem_url,   # 取最近一次提交的字段
  difficulty,                  # 取最近一次非空值（平台原始口径）
  status,                      # "unsolved" | "struggled" | "collected"
  fail_count,                  # 首次 AC 之前的失败次数（unsolved 为全部失败数；collected 恒 0）
  last_attempt_at,             # 最近一次提交时间（UTC 秒；collected 为 added_at）
  first_ac_at,                 # 首次 AC 时间；unsolved / collected 为 None
  manual,                      # 是否来自手动收录（前端据此显示删除按钮）
}
```

排序：`last_attempt_at` 倒序（最近翻车的排前面），同时间按 `problem_key` 升序稳定。

### 3.2 手动收录清单（落盘）

```
backend/data/user/<组>/activity/review-manual.json
```

```json
{
  "items": [
    { "platform": "codeforces", "problemKey": "2245F", "problemName": "Xor Minimization",
      "problemUrl": "https://codeforces.com/problemset/problem/2245/F", "difficulty": 2100,
      "addedAt": 1761200000 }
  ]
}
```

- 与 `profile.json` / `secrets.json` 同级，随用户组目录删除一起清理，天然隔离；
- 由 `modules/activity/store.py` 的 `UserStore` 独占读写（`load_manual_review()` /
  `add_manual_review()` / `remove_manual_review()`），原子写沿用临时文件 + `os.replace`；
- 去重键 `(platform, problemKey)`：重复提交**幂等**，只更新题名/难度/外链等画像
  字段，`addedAt` 保持首次加入时间（不让重复点击把题顶到列表最前）；
- 文件损坏（手改坏、半截写入）按空清单处理并只跳过坏条目，不阻断错题本；
- `addedAt` 为 UTC 秒，与 `Submission.submitted_at` 同口径，合并排序时可直接比较。

**为什么要存题名/难度/外链**：收录后若平台不可达（离线、洛谷 cookie 过期、牛客
题库查不到），列表仍要能显示这一行；这些字段只是「收录时的快照」，推荐展开时
仍以平台实时画像为准，两者不一致时以实时值为准（见 §4）。

### 3.3 合并规则（纯函数）

```python
merge_review(records=[...], manual=[...], solved={...}) -> list[ProblemRecord]
```

1. `records`（提交聚合，见 3.1）原样保留；
2. 按 `(platform, problem_key)` 建 `records` 的键集；
3. 遍历 `manual`：
   - 键已存在于 `records` → **跳过**（提交数据优先，手动项不重复出现）；
   - 键在 `solved`（当前用户组全部账号的 AC 集合）→ **跳过**（已经过了，不需要补）；
   - 否则转成 `status="collected"`、`fail_count=0`、`last_attempt_at=added_at`、
     `manual=true` 的记录追加；
4. 统一按 `(-last_attempt_at, platform, problem_key)` 排序。

纯函数化是为了完全可单测：合并规则里的三个分支（有提交 / 已 AC / 纯手动）都
不需要文件或网络。

### 3.4 推荐的两种取数方式

平台能力不同，取数方式也不同；打分（`review.recommend_problems`）与前端共用。
手动收录项的推荐也走这里：`recommend()` 拿到 `(platform, problem_key)` 后先
`fetch_problem()` 取实时画像（`collected` 行没有提交可查），再按下面两条分支取候选。

**A. 全量题单缓存（`Capability.PROBLEMSET`，Codeforces）**

一次拉全站题单（含标签与难度），落盘缓存后按源题在缓存里打分。

```
backend/data/cache/activity/problemset/<platform>.json
```

```json
{
  "fetchedAt": 1761200000,
  "items": [
    { "problem_key": "2245A", "name": "X Axis", "rating": 800,
      "tags": ["math", "sortings"], "url": "https://codeforces.com/contest/2245/problem/A" }
  ]
}
```

- 属于**缓存**而非事实来源：`backend/data/` 本就不入 git，可随时删除重建；
  `.gitignore` 已补 `backend/data/cache/`；
- TTL 24 小时（`XCPC_ACTIVITY_PROBLEMSET_TTL_SECONDS`，默认 86400）：过期只在
  **下一次推荐请求**时刷新，不在启动时预拉，避免拖慢启动；
- 刷新失败保留旧缓存并继续服务（带 `stale=true` 标记）；无旧缓存则返回空推荐，
  错题本不受影响；
- 原子写入沿用 `modules/activity/store.py` 的临时文件 + `os.replace` 模式；
- 单条记录损坏只跳过该条，不整体丢弃缓存。

**B. 定点查询（`Capability.PROBLEM_LOOKUP`，牛客竞赛）**

牛客题库约 298 页（pageSize=50），全量抓取按 1s 限流需约 5 分钟且收益低，
因此不做全量缓存，改为每次展开时两次请求（不落盘、无需 TTL）：

1. `fetch_problem(problem_key)`：题库列表页 `?keyword=<题号>` 精确查源题画像；
2. `find_similar_problems(target)`：`?tagId=<标签主键>&minDifficulty=&maxDifficulty=`
   取同标签、难度相近的候选（服务端过滤，候选量与请求量都小）。

关键约束（实测确认）：

- 牛客的标签过滤**只认主键 `data-id`**，标签文本（`tagName` / `tag`）不生效，
  故 `ProblemMeta` 增加 `tag_ids` 字段保存主键；
- 多个 `tagId` 以逗号分隔是 **OR** 语义，候选取并集后由共享打分按标签交集排序；
- 题库页 `pageSize=200` 返回空表（与提交页的重复页 bug 不同），必须用 50；
- 难度列可能为空（旧题未定级）→ `rating=None`，打分退回只按标签匹配。

代价：比赛专属题不进公开题库，此类错题的推荐必然为空——因此接口用
`reason` 区分两种空结果（见 §5 API），前端给出不同提示，而不是笼统说"没找到"。

### 3.5 链接解析（纯函数）

各平台 `normalize.py` 导出 `parse_problem_url(url) -> str | None`：输入任意用户
粘贴的链接（可带 `?query`、`#hash`、尾斜杠、`http://`），输出平台内的
`problem_key`；不认识返回 `None`。service 按 adapter 注册顺序遍历，第一个非
`None` 者赢——由此平台识别逻辑留在各 adapter 内，service 不写 if-else 表。

| 平台 | 识别的链接形态 | `problem_key` |
| --- | --- | --- |
| Codeforces | `/contest/{id}/problem/{idx}`、`/gym/{id}/problem/{idx}`、`/problemset/problem/{id}/{idx}` | `{id}{idx}`（如 `2245F`，与提交归一化同口径） |
| 牛客竞赛 | `/acm/problem/{id}`（含 `/acm/contest/{cid}/problem/{id}` 之外的题库链接） | `{id}` |
| 洛谷 | `/problem/{pid}`（忽略 `?contestId=`） | `{pid}`（如 `P1001`、`CF2245F`） |

约束：

- 只做**字符串解析**，不发请求、不查库，因此可完全单测；
- 解析出的 key 必须与同步归一化的 `problem_key` 完全一致，否则「收录后 AC 了
  却仍显示」——这是合并规则的最大风险点，故每个平台都要有对应单测；
- 域名只认官方域（`codeforces.com`、`www.nowcoder.com`、`www.luogu.com.cn`），
  避免把镜像站链接当成本平台的题号；
- 解析成功后再查一次平台画像（`fetch_problem`）；查不到时**仍允许收录**，
  用 URL 兜底题名（`fetch_problem` 返回 `None` 属正常：比赛专属题、未公开题），
  但 `difficulty=None`、推荐展开会返回 `not_indexed`。

### 3.6 按算法查找

统一标签模型 `TagMeta { id, name }`，与 `ProblemMeta.tag_ids` 对齐：

- **Codeforces** 没有数字主键，**用标签文本当 `id`**（`dp`、`graphs`）；
- **牛客 / 洛谷**用平台自己的主键（HTML 里的 `data-id`）。

两个新增 adapter 方法（基类默认抛 `CapabilityNotSupportedError`，与现有能力方法
同一模式，因此没实现的平台自然返回 400，前端按能力隐藏入口）：

- `list_problem_tags() -> list[TagMeta]`：标签目录；
- `search_problems(tag_id, *, page) -> list[ProblemMeta]`：一页结果（约 50 条）。

取数策略：

| 平台 | 标签目录 | 按标签搜题 | 代价 |
| --- | --- | --- | --- |
| Codeforces | 从全站题单缓存里汇总（按出现次数降序） | 缓存内本地过滤 + 分页 | 零额外请求；缓存未建时先懒刷新一次 |
| 牛客竞赛 | `GET /acm/problem/knowledge/catalog`（裸 JSON，匿名可用，15 组 155 个叶子） | `GET /acm/problem/list/json?tagId={id}&page={n}&pageSize=50` | 每次 1 请求；目录也只需 1 次 |
| 洛谷 | `GET /_lfe/tags`（裸 JSON，匿名可用，`type=2` 为算法标签共 262 个） | `GET /problem/list?tag={id}&page={n}`（lentille `data.problems`） | 每次 1 请求，走 5s 限流 |

实测要点（2026-10）：

- **牛客有裸 JSON 列表接口** `list/json`，比解析 HTML 稳得多：`data.problemSets[]
  {showId, name, difficulty, tagList[{id,name}], problemId}`，`data.problemCount`
  为总数。`showId` 形如 `NC274704`、`problemId` 是纯数字（`274704`），**入库要用
  纯数字**（与提交归一化的 `problem_key` 一致）；HTML 页的 `data-problemId`
  也是纯数字，两者一致。原 HTML 解析路径保留（`fetch_problem` / `find_similar_problems`
  继续用），新增搜索走 JSON；
- 牛客 `knowledge/catalog` 是「按知识点练习」目录，覆盖面比题库 tag 窄（题库里
  存在 `差分/前缀和`(1286) 这类不在 catalog 里的 tag），但 catalog 里的 id
  **可以直接当 `tagId` 用**（校验：`14424` 目录称 20 题，列表返回 20 题）；
- 牛客 `pageSize` 在 JSON 接口可用 50；`minDifficulty`/`maxDifficulty` 同 HTML
  接口语义（相对难度分值）；
- 洛谷 `/_lfe/tags` 返回 `{tags:[{id,name,type,parent}], types:[...]}`，`type=2`
  为算法（其余是区域/来源/时间/特殊题目/其他，搜题时过滤掉）；页面里内嵌的
  `__luoguTagRequest = '/_lfe/tags'` 就是这个端点；
- 洛谷题目列表 lentille `template == "problem.list"`，`data.problems.{perPage:50,
  count, result:[{pid,name,difficulty,tags:[id...]}]}`；`data.filter.tag` 是
  二维数组（`[[1]]`），不用于解析。

不做全量缓存的理由与 §3.4-B 相同：牛客/洛谷题库上万题且页面分页，全量抓一次
要几分钟；用户真正需要的是「一个标签的前几页」。

### 3.7 不变量

- 推荐候选**永远排除**用户已 AC 的题（按当前用户组合并全部绑定账号），
  也排除源题本身；
- 推荐结果不落盘、不记录「跳过/已看」，第一期无状态；
- 没有任何提交时返回空列表，不报错（此时错题本仍可显示手动收录项）；
- 手动收录**只删自己的条目**，不能影响提交聚合出的记录；
- 收录接口对同一 `(platform, problem_key)` 幂等，重复调用不产生第二条；
- 已 AC 的题可以被收录（用户可能想复盘），但列表里**不显示**为 `collected`——
  接口返回收录结果即可，不报错。

## 4. 页面与交互

新页面 `/activity/review`，侧边栏「训练统计」组下第二个子页「错题本」。

```
┌ 收录：[粘贴题目链接.......................] [加入]   [按算法查找] ┐
│ 筛选：[全部平台][Codeforces]...        [全部][未通过][挣扎][收录] │
├──────────────────────────────────────────────────────────────┤
│ 2245F  Xor Minimization    CF 2100  未通过 · 失败 4 次  2 天前  ⌄ │
│   └ 同类练习：1879E(1800) 1970D(2000) 1902E(2100) ...          │
│ 1001A  A+B                 LG 入门  挣扎 · 失败 3 次    5 天前   │
│ P4779  单源最短路          LG 提高  收录 · 刚刚            [删]  │
└──────────────────────────────────────────────────────────────┘
```

- 平台筛选复用 `/platforms` 元数据（不硬编码平台清单），含「全部」；
- 状态筛选：全部 / 未通过 / 挣扎题 / 收录；
- 行内展示：题号 + 题名（新标签打开原题）+ 平台 + 难度（原始口径）+
  状态标签 + 失败次数（`collected` 不显示）+ 最近时间的相对时间；
- 展开箭头在平台具备 `PROBLEMSET` 或 `PROBLEM_LOOKUP` 时出现；
  展开后懒加载，失败只在展开区提示「暂时无法获取同类题目，不影响错题记录」；
- 「删」只出现在 `manual=true` 的行（即 `collected`），走 `DELETE` 接口，
  成功后本地移除而不整页刷新；
- 空态三分：未绑定账号 / 已绑定但没有错题 / 已绑定但尚未同步（引导去总览页同步）。
  纯手动收录的场景下，「已绑定但没有错题」不应把收录项说成空——空态判断以
  **合并后的列表**为空为准。

### 4.1 收录区（粘贴链接）

- 单行输入 + 「加入」按钮，回车提交；输入框不做实时校验（链接形态多，误报更烦），
  提交后由后端判定并回显错误原因；
- 成功：清空输入 + 轻提示「已加入错题本」+ 列表顶部插入该行（不整页刷新，
  保留当前筛选与滚动位置）；若该题已有提交聚合记录则提示「该题已在列表中」；
- 失败按后端 `detail` 直接提示，前端不猜原因：`无法识别该链接，支持 Codeforces /
  牛客 / 洛谷题目页`；
- 提交中禁用按钮并显示 loading，避免重复点击产生并发写。

### 4.2 按算法查找面板

- 入口按钮排在收录区右侧；打开时默认选中**当前筛选的平台**，若当前是「全部」
  则要求先选平台（平台间标签体系不通，不能混）；
- 面板内：平台下拉（只列具备 `search_problems` 能力的平台）→ 标签下拉
  （可输入过滤，`NSelect` 的 `filterable`）→ 结果列表（题号 + 题名 + 难度 +
  标签 + 操作）；
- 结果行三种状态：可收录 / 「已收录」（手动清单里已有，按钮禁用）/「已通过」
  （提交里有 AC，按钮禁用并给 tooltip）——后两者都避免推进已会或已记录的题；
- 分页沿用现有分页组件（每页 50，上一页/下一页 + 页码），不无限滚动；
- 面板关闭后不保留状态（每次打开从当前平台重新取标签目录），实现简单且不会
  出现「标签属于 A 平台但面板显示 B 平台」的错位。

## 5. 工程落地

### 后端结构

| 位置 | 职责 |
| --- | --- |
| `backend/src/modules/activity/review.py` | 纯函数：提交 → `ProblemRecord` 列表；推荐排序打分。无 IO，可单测 |
| `backend/src/modules/activity/problemset.py` | 题单缓存的读写与 TTL（唯一写 `data/cache/` 的模块） |
| `backend/src/modules/activity/schemas.py` | 新增 `ReviewItemOut` / `ReviewOut` / `RecommendItemOut` / `RecommendOut` |
| `backend/src/services/activity/service.py` | `review_list()` / `recommend()`：取提交、调纯函数、按能力分支（题单缓存 / 定点查询） |
| `backend/src/adapters/base.py` | 新增 `Capability.PROBLEMSET` / `Capability.PROBLEM_LOOKUP`、`ProblemMeta`（含 `tag_ids`）、`fetch_problemset()` / `fetch_problem()` / `find_similar_problems()`（基类默认抛 `CapabilityNotSupportedError`） |
| `backend/src/adapters/codeforces/api_models.py` | `CfProblemsetResult`（`result` 是对象不是列表，不能复用 `CfEnvelope[list]`） |
| `backend/src/adapters/codeforces/__init__.py` | 声明 `PROBLEMSET` 并实现 `fetch_problemset()` |
| `backend/src/adapters/nowcoder/__init__.py` | 声明 `PROBLEM_LOOKUP` 并实现 `fetch_problem()` / `find_similar_problems()`（题库列表页 HTML 正则解析） |
| `backend/src/adapters/luogu/__init__.py` | 声明 `PROBLEM_LOOKUP`（第三期新增）并实现 `fetch_problem()` / `find_similar_problems()` / `list_problem_tags()` / `search_problems()`（lentille 解析） |
| `backend/src/adapters/*/normalize.py` | 各平台新增纯函数 `parse_problem_url()` |
| `backend/src/adapters/base.py`（补充） | 新增 `TagMeta`、`list_problem_tags()` / `search_problems()`（基类默认抛 `CapabilityNotSupportedError`） |
| `backend/src/modules/activity/review.py`（补充） | 新增纯函数 `merge_review()`；`ProblemRecord` 增 `manual` 字段 |
| `backend/src/modules/activity/store.py`（补充） | `UserStore` 新增手动清单读写（`MANUAL_REVIEW_FILE`） |

依赖方向不变：`review.py` / `problemset.py` 不 import adapters、不碰 router。

推荐打分（纯函数，便于测试）：

```python
key = (-共同标签数, 难度差绝对值, problem_key)   # 升序取前 5
```

过滤链：至少 1 个共同 tag → rating 落在源题 `[rating-200, rating+100]`
（源题无 rating 时只按 tag；候选无 rating 时保留但排在已知难度之后）→
排除已 AC 与源题 → 取前 5。同分按 `problem_key` 升序，保证结果稳定。

### API

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| `GET` | `/api/activity/review?platform=&status=` | 错题列表；`status` 取 `all`（默认）/ `unsolved` / `struggled` / `collected`；`platform` 缺省为全部 |
| `GET` | `/api/activity/review/recommend?platform=&problemKey=` | 同类推荐 5 条；平台无 `PROBLEMSET` / `PROBLEM_LOOKUP` 任一能力返回 400。空结果时 `reason` 区分 `not_indexed`（源题不在平台题库，如牛客比赛专属题）与 `no_match`（有收录但没有相近题）；`stale=true` 表示平台查询失败或题单缓存已过期 |
| `POST` | `/api/activity/review/manual` | 收录一题。body `{url}` 或 `{platform, problemKey}`（二选一，都缺 400）；返回收录后的 `ReviewItemOut` 与 `created`（false 表示幂等命中已有条目）；无法识别链接 400，平台不存在 400 |
| `DELETE` | `/api/activity/review/manual?platform=&problemKey=` | 删除手动条目；不存在返回 404（不静默成功，便于前端发现状态漂移）；仅删手动项，绝不触碰提交聚合 |
| `GET` | `/api/activity/review/tags?platform=` | 该平台算法标签目录 `[{id, name, count?}]`；平台不具备搜索能力返回 400 |
| `GET` | `/api/activity/review/search?platform=&tag=&page=` | 按标签搜题一页（50 条）；`tag` 必填；返回 `items` + `page` + `hasMore` + 每条 `solved` / `collected` 标记（前端据此禁用按钮，避免前端再拉两份状态） |

全部接口都作用于当前用户组，不新增用户组参数。

`search` 的 `solved` / `collected` 由 service 计算：`solved` 来自提交聚合的 AC 集合，
`collected` 来自手动清单键集——两者都是本地数据，不产生额外平台请求。

### 前端落地

| 位置 | 内容 |
| --- | --- |
| `frontend/src/features/activity/api.ts` | `fetchReview()` / `fetchReviewRecommend()` / `addReviewManual()` / `removeReviewManual()` / `fetchReviewTags()` / `searchReviewProblems()` |
| `frontend/src/features/activity/types.ts` | `ReviewItem` / `RecommendItem` / `RecommendData` / `ProblemTag` / `SearchProblem` |
| `frontend/src/features/activity/ReviewPage.vue` | 页面：收录输入区 + 按算法查找入口 + 平台与状态筛选 + 分页列表 |
| `frontend/src/features/activity/components/ReviewList.vue` | 列表、分页与展开区（推荐加载态、失败态、缓存降级提示）、收录行删除 |
| `frontend/src/features/activity/components/ReviewManualAdd.vue` | 粘贴链接收录（输入 + 校验回显 + loading） |
| `frontend/src/features/activity/components/ReviewTagSearch.vue` | 按算法查找面板（平台 → 标签 → 结果 → 收录） |
| `frontend/src/features/activity/model/review.ts` | 纯函数：失败次数文案、相对时间、状态标签映射、筛选、收录结果合并（配 vitest） |
| `frontend/src/app/nav.ts` / `router.ts` | 新增子页与路由 `/activity/review` |

不引入新依赖；列表沿用现有卡片样式，标签用 `NTag`。

## 6. 验证方式

- 后端：`review.py` / `problemset.py` / 推荐打分 / `merge_review` 的纯函数单测；
  service 层的平台过滤、空态、能力缺失分支、缓存命中与刷新失败降级、收录幂等
  与删除 404；adapter 的题单解析（fixture 覆盖 `result` 为对象、缺 tags、缺 rating）、
  `parse_problem_url` 的每个链接形态（含带 query / 尾斜杠 / 非官方域）、洛谷 lentille
  的标签目录与题目列表解析。
- 前端：`model/review.ts` 单测；`npm run typecheck`、`npm run test`、`npm run build`。
- 手动走查：绑定一个 CF 账号 → 同步 → 错题本出现未通过/挣扎题 → 展开看到推荐 →
  断网后展开只报推荐不可用、列表正常；粘贴 CF / 牛客 / 洛谷三种链接各收录一题 →
  筛选「收录」可见 → 删除一条 → 刷新不复现；按算法查找选标签 → 收录结果里的题 →
  再查同页显示「已收录」。
