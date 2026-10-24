# 错题本与同类推荐（review）设计

> 状态：已实现（第一期：Codeforces 推荐）。完成后请更新 [README.md](../README.md) 索引中的状态。
> 公共约定见 [conventions.md](conventions.md)；需求优先级见 [../../requirements.md](../../requirements.md)。

## 1. 背景与目标

现有「数据总览」回答的是「练了多少」（解题数、热力图、近期提交），回答不了
「哪里练得不好」。选手真正需要的是：把散落在各平台的没做出来的题、以及做了
很多次才过的题挑出来，再给几条同类型的练习。

本功能是 activity 的**子功能**，不新开功能域：

- 事实来源仍是 `data/user/<userid>/activity/submissions/*.jsonl`，不新增采集、
  不改提交结构；
- 只读聚合，不做写操作，因此不涉及原子写与并发写约定；
- 「同类推荐」第一期只做 Codeforces（唯一能一次拿到全站题目标签与分值的平台），
  其他平台只列错题、不展示推荐入口。

## 2. 总体形态

### 2.1 产品口径

| 概念 | 定义 |
| --- | --- |
| 未通过（unsolved） | 该 `(platform, problem_key)` 全部提交中从未出现 `AC` |
| 挣扎题（struggled） | 有过 `AC`，但**首次 AC 之前**的失败提交数 ≥ `STRUGGLING_FAILS`（3） |
| 计入失败 | `WA / TLE / MLE / RE / OLE / UNAC / UKE`（`CE` 属编译错误、`JG` 属评测中，均不计） |
| 题目身份 | `(platform, problem_key)`——不做跨平台同题合并 |

一次都没交过的题不会出现（无数据）；`CE` 不计入失败，避免「本地环境没配对」
把题目误判成不会做。

### 2.2 关键决策

- **不做独立功能域**。提交数据由 `modules/activity/store.py` 独占读取，新域直接
  读 jsonl 会打破「功能间依赖单向、模块零耦合」（[../conventions.md](../conventions.md)）；
  同理不新建 `features/review/`，前端复用 activity 的 `api` / `store` / 平台元数据。
- **不改提交模型**。标签、rating 区间属于「题库」数据，与「我交过什么」正交；
  塞进 `Submission` 会让每行 jsonl 冗余膨胀，且平台标签会随时间变化。推荐所需的
  题目画像走独立缓存。
- **题单缓存为可选能力，不阻断错题本**。没有 `PROBLEMSET` 能力的平台、
  缓存拉取失败、离线启动，错题列表都照常工作，推荐区显示不可用。
- **懒加载推荐**。展开某行时才请求推荐，不随列表一次算全站——列表是毫秒级的
  本地读取，推荐要读题单缓存（数千条）。

被否决的方案：

- **用大模型按题面推荐**：与「本地、离线可用」的非功能需求冲突，延迟与费用不可控；
- **自建全站题库**：等于维护另一个产品，且与「轻量、依赖越少越好」相反；
- **跨平台互推**（CF 的 `graphs` ↔ 洛谷的「图论」）：标签体系对不齐，误推代价高于收益；
- **把标签写进提交 jsonl**：历史数据需全量重刷，且与「平台标签随时可能变化」冲突。

## 3. 数据模型与存储

### 3.1 派生视图（不落盘）

错题列表完全由提交实时聚合，服务层不缓存、不落盘：

```python
ProblemRecord        # 单个题目在全部提交中的汇总（review.py 输出）
{
  platform, problem_key,
  problem_name, problem_url,   # 取最近一次提交的字段
  difficulty,                  # 取最近一次非空值（平台原始口径）
  status,                      # "unsolved" | "struggled"
  fail_count,                  # 首次 AC 之前的失败次数（unsolved 为全部失败数）
  last_attempt_at,             # 最近一次提交时间（UTC 秒）
  first_ac_at,                 # 首次 AC 时间；unsolved 为 None
}
```

排序：`last_attempt_at` 倒序（最近翻车的排前面），同时间按 `problem_key` 升序稳定。

### 3.2 题单缓存（推荐用，可删除重建）

```
backend/data/cache/activity/problemset/<platform>.json
```

```json
{
  "fetchedAt": 1761200000,
  "items": [
    { "problem_key": "2245A", "name": "X Axis", "rating": 800,
      "tags": ["math", "sortings"],
      "url": "https://codeforces.com/contest/2245/problem/A" }
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

### 3.3 不变量

- 推荐候选**永远排除**用户已 AC 的题（按当前用户组合并全部绑定的 CF 账号），
  也排除源题本身；
- 推荐结果不落盘、不记录「跳过/已看」，第一期无状态；
- 没有任何提交时返回空列表，不报错。

## 4. 页面与交互

新页面 `/activity/review`，侧边栏「训练统计」组下第二个子页「错题本」。

```
┌ 筛选：[全部平台][Codeforces][AtCoder]...   [全部][未通过][挣扎题] ┐
├──────────────────────────────────────────────────────────────┤
│ 2245F  Xor Minimization    CF 2100  未通过 · 失败 4 次  2 天前  ⌄ │
│   └ 同类练习：1879E(1800) 1970D(2000) 1902E(2100) ...          │
│ 1001A  A+B                 LG 入门  挣扎 · 失败 3 次    5 天前   │
└──────────────────────────────────────────────────────────────┘
```

- 平台筛选复用 `/platforms` 元数据（不硬编码平台清单），含「全部」；
- 状态筛选：全部 / 未通过 / 挣扎题；
- 行内展示：题号 + 题名（新标签打开原题）+ 平台 + 难度（原始口径）+
  状态标签 + 失败次数 + 最近尝试的相对时间；
- 展开箭头只在 `canRecommend`（平台具备 `PROBLEMSET` 能力）时出现；
  展开后懒加载，失败只在展开区提示「暂时无法获取同类题目，不影响错题记录」；
- 空态三分：未绑定账号 / 已绑定但没有错题 / 已绑定但尚未同步（引导去总览页同步）。

## 5. 工程落地

### 后端结构

| 位置 | 职责 |
| --- | --- |
| `backend/src/modules/activity/review.py` | 纯函数：提交 → `ProblemRecord` 列表；推荐排序打分。无 IO，可单测 |
| `backend/src/modules/activity/problemset.py` | 题单缓存的读写与 TTL（唯一写 `data/cache/` 的模块） |
| `backend/src/modules/activity/schemas.py` | 新增 `ReviewItemOut` / `ReviewOut` / `RecommendItemOut` / `RecommendOut` |
| `backend/src/services/activity/service.py` | `review_list()` / `recommend()`：取提交、调纯函数、按能力分支 |
| `backend/src/adapters/base.py` | 新增 `Capability.PROBLEMSET`、`ProblemMeta`、`fetch_problemset()`（基类默认抛 `CapabilityNotSupportedError`） |
| `backend/src/adapters/codeforces/api_models.py` | `CfProblemsetResult`（`result` 是对象不是列表，不能复用 `CfEnvelope[list]`） |
| `backend/src/adapters/codeforces/__init__.py` | 声明 `PROBLEMSET` 并实现 `fetch_problemset()` |

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
| `GET` | `/api/activity/review?platform=&status=` | 错题列表；`status` 取 `all`（默认）/ `unsolved` / `struggled`；`platform` 缺省为全部 |
| `GET` | `/api/activity/review/recommend?platform=&problemKey=` | 同类推荐 5 条；平台无 `PROBLEMSET` 能力返回 400，题单未命中源题返回空列表 |

两个接口都作用于当前用户组，不新增用户组参数。

### 前端落地

| 位置 | 内容 |
| --- | --- |
| `frontend/src/features/activity/api.ts` | `fetchReview()` / `fetchReviewRecommend()` |
| `frontend/src/features/activity/types.ts` | `ReviewItem` / `RecommendItem` / `RecommendData` |
| `frontend/src/features/activity/ReviewPage.vue` | 页面：平台与状态筛选 + 分页列表 |
| `frontend/src/features/activity/components/ReviewList.vue` | 列表、分页与展开区（推荐加载态、失败态、缓存降级提示） |
| `frontend/src/features/activity/model/review.ts` | 纯函数：失败次数文案、相对时间、状态标签映射、筛选（配 vitest） |
| `frontend/src/app/nav.ts` / `router.ts` | 新增子页与路由 `/activity/review` |

不引入新依赖；列表沿用现有卡片样式，标签用 `NTag`。

## 6. 验证方式

- 后端：`review.py` / `problemset.py` / 推荐打分的纯函数单测；service 层的
  平台过滤、空态、能力缺失分支、缓存命中与刷新失败降级；adapter 的题单解析
  （fixture 覆盖 `result` 为对象、缺 tags、缺 rating）。
- 前端：`model/review.ts` 单测；`npm run typecheck`、`npm run test`、`npm run build`。
- 手动走查：绑定一个 CF 账号 → 同步 → 错题本出现未通过/挣扎题 → 展开看到推荐 →
  断网后展开只报推荐不可用、列表正常。
