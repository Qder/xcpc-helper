# 待办题目（todo）设计

> 状态：设计中。完成后请更新 [README.md](../README.md) 索引中的状态。
> 公共约定见 [conventions.md](conventions.md)；错题本见 [review.md](review.md)；
> 需求优先级见 [../../requirements.md](../../requirements.md)。

## 1. 背景与目标

错题本回答「哪里练得不好」：它把全部未通过 / 挣扎题 / 收录题都摆出来，几百条
一起看，反而不知道今晚该补哪几道。真正需要的是一份**自己挑过的短清单**——
「接下来补这五道」，并且能按算法标签快速切到「今天就练二分」。

因此新增 activity 的**第二个子功能**「待办」：

- 错题本里的未通过题、收录题可以一键加进待办（**精选，不自动灌入**）；
- 待办页按平台分页签，页内用标签花片筛选（纯本地、即时）；
- 已 AC 的待办自动隐藏，但磁盘条目保留——同步把提交冲掉后还能回来。

与错题本的关系：

```mermaid
flowchart LR
  review[错题本 unsolved/collected] -->|"一键加入"| todoFile["review-todo.json"]
  todoFile --> merge[与提交 AC 集合合并]
  merge -->|"已 AC 则隐藏"| page[待办页]
  page -->|"标签花片精确匹配"| filter[当前平台下的待办子集]
```

## 2. 总体形态

### 2.1 产品口径

| 概念 | 定义 |
| --- | --- |
| 待办（todo） | 用户从错题本手动挑进 `review-todo.json` 的题，与提交数据无关 |
| 可见待办 | 磁盘上有、且当前用户组全部账号的 AC 集合里**没有**该题 |
| 标签快照 | 加入待办时记录的平台原生标签文本（不跨平台归一） |

- **精选，不自动灌入**：未通过 / 收录只是「可以加入」的来源，不会把 300+ 道
  未通过全部塞进待办；挣扎题已经 AC，行内不出现「加入待办」。
- **独立落盘**：`activity/review-todo.json`，与 `review-manual.json` 同模式
  （临时文件 + `os.replace`）。提交 jsonl 仍只读，待办写入不碰它。
- **已 AC 自动隐藏**：磁盘条目保留，列表不显示。删除只作用于待办，不影响错题本
  与收录清单。
- **标签筛选是待办页的能力，不是错题本的标签过滤**。错题本里做标签筛选仍被否决
  （要为每条错题拉一次画像）；待办是用户挑过的短清单，加入时快照标签，筛选纯
  本地、即时。
- **不跨平台合并标签**：CF 的 `dp`、牛客的「线性dp」、洛谷的「动态规划」各算各的。
  平台页签复用 `PlatformTabs.vue`。
- **筛选不写进网址**：与错题本一致，刷新回默认「全部平台 + 全部标签」。

### 2.2 关键决策

- **不做独立功能域**。待办读写的是 activity 自己目录下的清单，仍由
  `modules/activity/store.py` 的 `UserStore` 独占读写；前端复用 activity 的
  `api` / `types` / `PlatformTabs`，不新建 `features/todo/`。
- **标签在加入时快照**，而不是每次列表现查。理由：花片必须即时——一次列表请求
  要为几十条题各拉一次画像（CF 查题单缓存尚可，洛谷要读 `/_lfe/tags`，牛客要
  查题库），延迟与失败面都不可接受；而待办是用户主动挑的短清单，题目标签在
  「加入 → 做完」的时间尺度内几乎不变，快照足够。
- **查不到画像也允许加入**：与收录一致，平台暂不可达 / 比赛专属题 / 未公开题
  都可能查不到标签，此时 `tags=[]`——题仍能进待办与列表，只是进不了任何花片。
  不允许加入会让「离线时先记一笔」失败，与待办的目的相悖。
- **不加「已加入」去重状态到错题本接口之外的请求**：`GET /review` 每条带
  `inTodo`，service 本地读待办键集即可，零网络。

被否决的方案：

- **把待办塞进 `review-manual.json`**：收录清单的语义是「还没 AC 的题」，且
  `collected` 状态会与待办混在一起；两套清单的删除/清空逻辑也不同（清空待办
  不该删收录）。
- **把标签写进提交 jsonl**：历史数据要全量重刷，且与「平台标签随时会变」冲突；
  提交数据是同步派生缓存，用户手挑的待办不能被下一次同步冲掉。
- **待办页每次现查每题标签**：上面已述，花片必须即时。
- **自动把全部未通过题灌进待办**：300+ 条的清单等于没有清单，用户无从下手。
- **跨平台合并标签花片**：标签体系对不齐（`dp` ↔ 「线性dp」↔「动态规划」），
  合并后点击会得到语义混杂的结果。

## 3. 数据模型与存储

### 3.1 落盘（`review-todo.json`）

```
backend/data/user/<组>/activity/review-todo.json
```

```json
{
  "items": [
    { "platform": "nowcoder", "problemKey": "325082", "problemName": "小月的订单",
      "problemUrl": "https://ac.nowcoder.com/acm/problem/325082", "difficulty": 1600,
      "tags": ["线性dp", "二分"], "addedAt": 1792827869 }
  ]
}
```

- 与 `profile.json` / `review-manual.json` 同级，随用户组目录删除一起清理；
- 由 `UserStore` 独占读写（`load_todo()` / `save_todo()` / `add_todo()` /
  `remove_todo()`），原子写沿用临时文件 + `os.replace`；
- 去重键 `(platform, problemKey)`：重复加入**幂等**，只刷新画像字段
  （题名 / 外链 / 难度 / 标签），`addedAt` 保持首次加入时间；
- 文件损坏按空清单处理，单条坏条目只跳过该条，不阻断页面；
- `addedAt` 为 UTC 秒，与 `Submission.submitted_at` 同口径，排序可直接比较。

**字段为什么存这一份**：平台不可达时列表仍要能显示这一行（题名 / 难度 / 外链），
标签花片也必须离线可用；这些都是「加入时的快照」，展示时以后端返回的快照为准，
不回头查平台。

### 3.2 标签来源（加入时取一次）

复用 review 已有的 `_fetch_problem_quietly()`，按平台能力走：

| 平台 | 取标签方式 | 说明 |
| --- | --- | --- |
| Codeforces | 题单缓存里该题的 `tags` | 缓存未命中/过期刷新失败 → `tags=[]` |
| 牛客竞赛 | 题库 `keyword=<题号>` 的 `tags` | 比赛专属题查不到 → `tags=[]` |
| 洛谷 | 题目页 `tag_ids` → `/_lfe/tags` 映射出名称 | 只取算法标签（`type==2`）；解析失败 → `tags=[]` |

其余平台（AtCoder / LeetCode CN / VJudge / QOJ）没有标签能力，`tags=[]`，
待办里仍可加入与移除，只是不产生花片。

### 3.3 可见性与排序（纯函数思路）

`todo_list()` 的规则都可以用纯本地数据表达：

1. 读 `review-todo.json`；
2. 读提交聚合的 AC 集合（`review.solved_keys`，当前组全部账号）；
3. 过滤掉 `(platform, problem_key)` 命中 AC 集合的条目（**已 AC 隐藏**）；
4. `platform` 非空则只留该平台；
5. `tag` 非空则只留 `tags` 包含该标签（**精确匹配，不做子串**）；
6. 按 `(-added_at, platform, problem_key)` 排序；
7. 标签花片：在第 3、4 步之后的集合上统计 `tag → count`，
   按 `count` 降序、同数按名称升序；**不受第 5 步影响**（否则点了花片后
   其他花片会消失，无法切换）。

### 3.4 不变量

- 待办列表**永远不含**已 AC 的题（按当前用户组全部绑定账号合并判断）；
- 待办写入**只碰 `review-todo.json`**，绝不修改提交数据与收录清单；
- 同一 `(platform, problemKey)` 在文件里至多一条；
- 加入已 AC 的题返回 400（前端不给入口，但接口要挡住直接调用）；
- 删除不存在的条目返回 404（不静默成功，便于前端发现状态漂移）；
- 空待办返回空列表 + 空花片，不报错。

## 4. 页面与交互

新页面 `/activity/todo`，侧边栏「训练统计」组下紧挨错题本。

```
┌ [全部平台][Codeforces][洛谷][牛客]                 3 道 ┐
│ 标签：[全部] [线性dp 2] [二分 1] [dp 1]                  │
├──────────────────────────────────────────────────────────┤
│ 325082  小月的订单   牛客  1600  收录  线性dp · 二分  [移出] │
│ 2245F   Familiar?    CF    2800  未通过  dp          [移出] │
└──────────────────────────────────────────────────────────┘
```

- 平台页签复用 `PlatformTabs.vue`，含「全部平台」；
- 标签花片一行横向排列：`[全部]` + 每个标签带计数；点一个即精确筛选，
  再点一次或点 `[全部]` 取消；
- 行内：题号 + 题名（新标签打开原题）+ 平台 + 难度（原始口径）+ 状态标签
  （未通过 / 收录）+ 标签列表 + 「移出」；
- 「移出」走 `DELETE`，成功后本地移除不整页刷新；
- 空态两分：
  - 还没加过任何题 → 「从错题本把没过的题加进来」（附跳转错题本的按钮）；
  - 当前标签下没有题 → 「当前标签下没有待办」；
- 已 AC 隐藏后若整页为空，也按「还没加过（或都已通过）」的空态处理。

### 4.1 错题本行内入口

- 只有 `status` 为 `unsolved` / `collected` 且 `inTodo=false` 的行显示
  「待办」按钮；`struggled` 已经 AC，不出现；
- 点击 → `POST /api/activity/todo` → 成功后就地把该行标成「已待办」（禁用），
  不整页刷新；失败按后端 `detail` 提示（例如「该题已 AC，不需要加入待办」）；
- `inTodo=true` 的行按钮禁用并显示「已待办」。

## 5. 工程落地

### 后端结构

| 位置 | 内容 |
| --- | --- |
| `modules/activity/models.py` | `TodoItem` / `Todo`（文件结构） |
| `modules/activity/store.py` | `TODO_FILE`、`load_todo()` / `save_todo()` / `add_todo()` / `remove_todo()` |
| `modules/activity/schemas.py` | `TodoIn` / `TodoItemOut` / `TodoTagOut` / `TodoOut`；`ReviewItemOut.inTodo` |
| `services/activity/service.py` | `add_todo()` / `todo_list()` / `remove_todo()`；`review_list()` 标注 `inTodo` |
| `routers/activity/router.py` | `GET/POST/DELETE /api/activity/todo` |

依赖方向不变：router → service → store / adapters，`review.py` 的纯函数复用。

### API

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| `GET` | `/api/activity/todo?platform=&tag=` | 待办列表（已 AC 隐藏）+ 标签花片 `[{name, count}]`；`platform` 缺省为全部；`tag` 精确匹配 |
| `POST` | `/api/activity/todo` | body `{platform, problemKey}`；加入时取一次画像快照；已 AC → 400；幂等（同键不新建，`created=false`） |
| `DELETE` | `/api/activity/todo?platform=&problemKey=` | 只删待办；不存在 404 |
| `GET` | `/api/activity/review` | 每条新增 `inTodo`（本地读待办键集，零网络） |

全部接口作用于当前用户组，不新增用户组参数。

### 前端落地

| 位置 | 内容 |
| --- | --- |
| `frontend/src/features/activity/types.ts` | `TodoItem` / `TodoData` / `TodoTag`；`ReviewItem.inTodo` |
| `frontend/src/features/activity/api.ts` | `fetchTodo()` / `addTodo()` / `removeTodo()` |
| `frontend/src/features/activity/TodoPage.vue` | 页面：平台页签 + 标签花片 + 列表 + 空态 |
| `frontend/src/features/activity/components/ReviewList.vue` | 未通过 / 收录行内「待办 / 已待办」按钮 |
| `frontend/src/app/nav.ts` / `router.ts` | 新增子页与路由 `/activity/todo` |

不引入新依赖；花片用 `NTag`（`checkable`），列表沿用错题本行样式与相对时间格式化。

## 6. 验证方式

- 后端：`UserStore` 待办读写的原子性、幂等、损坏文件容错；service 的 AC 隐藏、
  平台过滤、标签精确匹配与花片统计口径（花片不受 `tag` 参数影响）、加入已 AC
  返回 400、删除 404；`review_list` 的 `inTodo` 标注。`uv run pytest` +
  `uv run ruff check src tests`。
- 前端：`npm run typecheck`、`npm run test`、`npm run build`。
- 手动走查：错题本里把一道未通过题加入待办 → 待办页出现该题 → 标签花片出现该题
  的标签 → 点花片只留匹配项 → 点「全部」复原 → 「移出」后刷新不复现 → 把该题
  在提交里变 AC 后（或 mock AC 集合）刷新，列表不再显示但文件仍在 →
  同一题重复加入不产生两条 → 错题本行内按钮显示「已待办」。
