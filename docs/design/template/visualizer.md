# 算法可视化（template / visualizer）设计

> 状态：已实现。一期实现。需求优先级归属见 [../../requirements.md](../../requirements.md)；
> 跨功能公共约定见 [../conventions.md](../conventions.md)；模板库本体见 [template-library.md](template-library.md)。
> 演示页骨架见 [visual-skeleton.html](visual-skeleton.html)；
> 给 AI 生成 visual.html 的提示词见 [visual-prompt/README.md](visual-prompt/README.md)。

## 1. 背景与目标

选手需要「看着动画理解算法」。模板库里每个版本已经是一份代码 + `README.md`；本功能允许在同一版本目录里再放一份自包含的 `visual.html`，作为该版本的算法演示。

模板整理侧新增「算法可视化」子页：画廊只列出带演示的模板，点击进入沙箱 iframe 播放对应 `visual.html`。文件由选手手写，或由 AI 按 [visual-prompt/](visual-prompt/README.md) 的通用核心 + 类型附录生成，保存到该版本目录。

本功能仍在 **template** 域内（不新开功能域）。画廊页可粘贴/覆盖 `visual.html`；覆盖前把旧文件归档到 `.visual-history/`。不做界面删除当前演示、压缩包导入导出仍不携带该文件。

## 2. 总体形态

- **文件约定**：每个版本目录下固定文件名 `visual.html`，与代码、`README.md` 并排；按版本放，不按模板（主标签）放；不在 README front matter 里新增字段。
- **单文件离线**：`visual.html` 必须内联全部 CSS/JS，不引用任何外部资源。骨架已内置统一播放器（播放 / 单步 / 进度 / 随机输入 / 主题跟随）。
- **不可信内容**：`visual.html` 可能由 AI 生成或来自他人压缩包，必须按不可信内容处理（沙箱隔离，见 §5）。

被否决方案：

| 方案 | 否决理由 |
| --- | --- |
| 内置通用播放器引擎、每个算法只写步骤脚本 | 前期要写多种数据结构渲染器，工作量大；骨架已内置统一播放器，足以保证观感一致，日后可迁移 |
| 在 README front matter 增加 visual 字段指向任意文件 | 多一套校验，收益小；固定文件名即可被扫描器发现 |
| 按模板（主标签）放一份 | 多版本模板（如并查集的路径压缩 / 带权）演示内容本就不同 |

## 3. 数据模型与存储

事实来源仍是 `backend/content/`。可视化文件挂在版本目录下：

```plaintext
content/图论/最短路/dijkstra/
├── dijkstra.cpp
├── README.md
├── visual.html
└── .visual-history/
    └── 20261020-040112Z.html
```

顶层单版本（代码直接在模板目录下）时，`visual.html` 与代码、README 同级，放在模板目录。历史目录必须点开头（`.visual-history/`），否则顶层单版本会被扫描器当成副标签。快照文件名为 UTC 时间戳 `YYYYMMDD-HHMMSSZ.html`（撞名则 `-2` 递增）；id 只允许 `^\d{8}-\d{6}Z(?:-\d+)?$`。

扫描器只认代码扩展名与 `README.md`。`.html` 不会被当成代码，也不产生诊断。文件名匹配**大小写不敏感**（与 README 查找方式一致）。

领域模型：

- `scanner.py` 增加常量 `VISUAL_FILENAME = "visual.html"`；
- `VersionNode` 增加 `has_visual: bool`；扫描该版本目录时检测文件是否存在；
- SQLite 索引（可随时重建）在 `templates` / `versions` 表带上 `has_visual`：模板级为「至少一个版本有可视化」。不得绕开 `rebuild_index`。

写入层：`delete_version_dir` 对顶层单版本删代码、README、`visual.html` 与 `.visual-history/`；子目录版本整体 `rmtree`。`write_visual_html` 把粘贴的 HTML 原子写入规范名 `visual.html`（已有大小写不同的同名文件先删再写）；若已有当前文件且内容不同，先拷到 `.visual-history/` 再覆盖。首次写入不归档；内容相同不重复归档。经 writer 的覆盖才会自动归档；磁盘直写（含 AI 用编辑器/Write 工具）需按 visual-prompt 自行先拷一份。

### 后续（本期不做）

- 导出压缩包与导入暂不携带 `visual.html`（见 [transfer.md](transfer.md)）；
- 界面里删除 `visual.html`；
- 「复制生成提示词」按钮（二期：按分类拼接 `visual-prompt/core.md` + 对应附录 + 骨架 + 模板代码到剪贴板）；
- 演示与代码面板的行高亮联动；
- 通用播放器引擎。

## 4. 页面与交互

1. 侧边栏「模板整理」下新增子页「算法可视化」（`nav.ts` / `router.ts`，meta：group「模板整理」，sub「算法可视化」）：
   - 路径 `/template/visual`：画廊页，只列出有可视化的模板；支持分类筛选与关键词搜索；卡片显示模板名、分类、标签、版本数；点击进入演示页。
   - 工具栏「导入可视化」：选已有模板与版本，粘贴 `visual.html` 源码（或选本地 `.html` 填进文本框）；该版本已有演示时二次确认后覆盖（覆盖前会留一份历史）；成功后刷新列表并进入演示页。
   - 工具栏「使用说明」：弹窗说明三种来源——版本目录放入文件、本页粘贴导入、对新对话说「把某某算法可视化」；并提示可在演示页管理历史。
   - 空状态：说明「在版本目录放入 visual.html 即可出现在这里」，并提示也可点右上角导入；给出骨架文件路径 `docs/design/template/visual-skeleton.html`。
2. 演示页 `/template/visual/:category/:name`，query `?version=<版本名，缺省为主版本>`：
   - 左侧大块 iframe 展示 `visual.html`；右侧可折叠面板，显示该版本代码（`CodeView`）与说明（`MarkdownView`）；
   - 可同时收起应用左侧导航与右侧代码，给演示区更多宽度：骨架在足够宽时会把动画内代码排到右侧，而不是堆在动画下方；
   - 多版本时提供版本切换（只列有可视化的版本）；当前 query 指向无可视化的版本时显示提示，不报错；
   - 「历史」：列出该版本 `.visual-history/` 快照；点一项用 iframe 播放（query `?history=<id>`，与 `version` 并存）；看历史时可「恢复为当前」（现稿会再归档）或单条删除；「清空历史」一键删除全部快照，不删当前 `visual.html`；
   - 「新窗口打开」按钮；加载失败、404、文件过大各有明确提示；
   - 网址状态同步：刷新、前进后退、复制链接均可恢复同一视图（参考训练统计页的 query 同步）。
3. 模板详情页：当前版本 `has_visual` 为真时，显示「可视化」按钮（lucide `Play`），跳到演示页并带上当前版本。
4. 主题联动：宿主在 iframe 加载完成后，以及主题变化时，向 iframe 发送 postMessage：`{ type: 'xcpc-theme', dark: boolean, hue: number }`（`dark` 来自 `documentElement` 的 `data-theme`，`hue` 来自 `--hue`）。沙箱 iframe 的源是 opaque，`targetOrigin` 只能用 `'*'`；消息只含主题信息，不得携带任何其他数据。
5. 全部使用 Naive UI 与 lucide 图标，样式随主题，不写死颜色。窗口缩窄时左右面板可收起，布局不崩。

## 5. 工程落地

### 后端结构

依赖方向沿用 `routers → services → modules`；不新增功能域，仍在 template 域内。

- `modules/template`：scanner / models / schemas / repository 增加 `has_visual`；新增读取与写入可视化文件的函数。路径必须由 `category` / `name` / `version` 经现有 `validate_name` 解析后拼接，并校验最终路径在 `content_dir` 内（防穿越）；版本用 `ROOT_VERSION_TOKEN` `"~"` 表示顶层单版本。
- `core/config.py` 新增 `visual_max_bytes`，默认 2 MiB（`XCPC_VISUAL_MAX_BYTES` 可覆盖）。
- `watcher.py` 当前对任意非目录、非 `.tmp-*` 文件事件去抖重建，**无扩展名白名单**，`visual.html` 的新增 / 修改 / 删除已能触发索引重建；本期只确认该行为，不收紧过滤。

安全约定：`visual.html` 视为不可信内容。

- 接口放在 `/api` 下，避免被 SPA 回退吞掉；
- 响应带 `X-Content-Type-Options: nosniff` 与严格 CSP（见下表）；
- 前端 iframe 必须 `sandbox="allow-scripts"`，**不得**加 `allow-same-origin`，并设 `referrerpolicy="no-referrer"`；
- 宿主只向 iframe 发送主题 postMessage，不发送代码、路径或其他数据。

超限抛业务异常 413（`payload_too_large`），非法名称 / 穿越 400，文件或版本不存在 404。

### API

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/api/templates` | 现有列表增加 `has_visual`；新增可选查询参数 `has_visual=true`，只返回至少一个版本有可视化的模板 |
| GET | `/api/templates/{id}` | 详情的每个 variant 增加 `has_visual` |
| GET | `/api/templates/{category}/{name}/versions/{version}/visual` | 返回该版本的 `visual.html`；`~` 表示顶层单版本；无文件或版本不存在 404；超过 `visual_max_bytes` 413 |
| PUT | `/api/templates/{category}/{name}/versions/{version}/visual` | 写入该版本 `visual.html`（请求体 `{ html }`）；已有且内容不同时先归档；版本不存在 404；空或非 HTML 400；超过 `visual_max_bytes` 413 |
| GET | `/api/templates/{category}/{name}/versions/{version}/visual/history` | 该版本历史快照列表 `{ items: [{ id, bytes, saved_at }] }`，新到旧 |
| GET | `/api/templates/{category}/{name}/versions/{version}/visual/history/{id}` | 托管该快照 HTML；CSP 与 GET visual 相同；不存在 404；超限 413 |
| POST | `/api/templates/{category}/{name}/versions/{version}/visual/history/{id}/restore` | 把快照写回当前（走 PUT 同一套写入，现稿会再归档）；204 |
| DELETE | `/api/templates/{category}/{name}/versions/{version}/visual/history/{id}` | 删除一份快照；不存在 404；不删当前 `visual.html` |
| DELETE | `/api/templates/{category}/{name}/versions/{version}/visual/history` | 清空该版本全部历史；当前 `visual.html` 不动 |

该托管接口响应必须带：

```
Content-Type: text/html; charset=utf-8
X-Content-Type-Options: nosniff
Content-Security-Policy: sandbox allow-scripts; default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; img-src data: blob:; font-src data:; connect-src 'none'
```

字段命名沿用 template 域现有 schemas 风格（如 `variant_count`）。`GET /templates/{template_id:path}` 会吞掉更深路径，visual 路由必须注册在它之前。

### 前端落地

`features/template/` 下新增：

- `VisualGalleryPage.vue`、`VisualViewerPage.vue`；
- `components/VisualFrame.vue`（iframe 与主题联动）、`VisualImportModal.vue`、`VisualHelpModal.vue`；
- `model/visual.ts`（地址构建、版本名与 `~` 的转换、列表筛选等纯函数）+ `model/visual.test.ts`。

主题监听封装为 composable，参考 `features/activity/components/use-chart-palette.ts` 的 `MutationObserver` 写法。`api.ts` / `store.ts` / `types.ts` 沿用现有模式；不新增第三方依赖。地址里的中文分类、模板名、版本名必须 `encodeURIComponent`。

## 6. 验证方式

后端（`backend/` 下逐行）：`uv run pytest`、`uv run ruff check src tests`。新增覆盖：

1. scanner：有 / 无 `visual.html` 的 `has_visual`；大小写不敏感；不产生诊断；顶层单版本与子目录版本；中文路径；
2. writer：删除顶层单版本会清理 `visual.html` 与 `.visual-history/`；PUT 写入规范名、覆盖大小写不同的旧文件、空/非 HTML 拒绝、超限 413；覆盖归档、内容相同不重复归档、非法快照 id、穿越、清空后当前文件仍在；
3. 托管接口：200 且响应头逐项断言（CSP 含 `sandbox` 与 `connect-src 'none'`）；404；413；非法名称与 `../` 穿越被拒绝；`~` 寻址；中文路径；PUT 写入后 GET 可读；历史 list/get/restore/单删/清空；
4. 列表与详情的 `has_visual`、`has_visual=true` 过滤。

前端（`frontend/` 下逐行）：`npm run typecheck`、`npm run test`、`npm run build`。纯函数单测覆盖地址构建与 `~` 转换。

涉及 API 契约：起服务后 `curl http://127.0.0.1:8000/api/diagnostics` 正常返回；再 `curl -i` 访问 visual 接口核对响应头。

手动走查：把骨架文件临时复制到任一模板版本目录并改名为 `visual.html`（仅验证，不入库）→ 画廊出现该模板 → 点击进入演示页 → 播放、单步、随机、非法输入报错均正常 → 切换明暗主题，iframe 跟随 → 新窗口打开 → 删除文件后列表自动消失（watcher 重建）→ 窗口缩窄时布局不崩。
