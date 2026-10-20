# 算法可视化生成提示词

> 给 AI 生成 `visual.html` 用。播放器与骨架见 [../visual-skeleton.html](../visual-skeleton.html)；产品约定见 [../visualizer.md](../visualizer.md)。
> 一期只维护这些文件，界面里的「复制生成提示词」按钮不做。

## 1. 产出落盘（硬约定）

生成结果**必须**保存为对应**版本目录**下的 `visual.html`，与该版本的代码、`README.md` 并排。不要放进 `docs/`，不要按模板（主标签）只放一份。

```plaintext
backend/content/<分类>/<模板>/<版本>/
├── <代码文件>
├── README.md
└── visual.html          ← 本提示词的唯一产出
```

顶层单版本（代码直接在模板目录下）时，放在模板目录：

```plaintext
backend/content/<分类>/<模板>/
├── <代码文件>
├── README.md
└── visual.html
```

示例（与仓库里「01 背包」一致）：

```plaintext
backend/content/背包/01背包/01背包/visual.html
```

文件名固定为 `visual.html`（扫描器大小写不敏感）。保存后刷新或等 watcher 重建，即可出现在「算法可视化」画廊。

## 2. 怎么拼给 AI

按顺序拼成**一条消息**（缺一块就不要开写）：

1. [core.md](core.md)（通用核心，每份模板都要）
2. 按下表选用 **1～2 份**附录
3. [../visual-skeleton.html](../visual-skeleton.html) 全文，标成 `【骨架】`
4. 该版本源代码，标成 `【模板代码】`
5. 该版本 README 全文，标成 `【模板说明】`；没有就写 `无`

图、树类附录依赖骨架里 ALGO START **之前**的布局函数（`layoutCircle`、`layoutBinary`、`layoutTree`、`edgeSvg`、`svgBox`、`textSvg`），只调用、不要改写。函数说明见 [helpers.md](helpers.md)。

## 3. 按分类选附录

以 `backend/content/` 下的**一级分类目录名**为准；一个模板跨两类就附两份。

| 分类（示例） | 附录 |
| --- | --- |
| 图论 | [appendix-graph.md](appendix-graph.md) |
| 树论 | [appendix-tree.md](appendix-tree.md) |
| 动态规划、背包 | [appendix-array.md](appendix-array.md) |
| 字符串 | [appendix-string.md](appendix-string.md) |
| 数学、数论 | [appendix-math.md](appendix-math.md) |
| 数据结构 | [appendix-ops.md](appendix-ops.md)（堆/并查集/树状数组等）；平衡树、Trie、线段树改用树附录 |
| 树上 DP | 树 + 数组/DP |
| 树链剖分 | 树 + 操作序列 |
| 最短路树上 DP | 图 + 树 |

## 4. 文件一览

| 文件 | 作用 |
| --- | --- |
| [core.md](core.md) | 角色、输出契约、帧与渲染、忠实原则、代码面板 |
| [helpers.md](helpers.md) | 图/树布局函数（骨架已内置） |
| [appendix-graph.md](appendix-graph.md) | 图：最短路、MST、拓扑、网络流 |
| [appendix-tree.md](appendix-tree.md) | 树：二叉树、线段树、Trie、平衡树、LCA |
| [appendix-array.md](appendix-array.md) | 数组 / DP / 二分 / 双指针 |
| [appendix-string.md](appendix-string.md) | 字符串：KMP、哈希、Z、AC 自动机 |
| [appendix-math.md](appendix-math.md) | 数论 / 数学：筛、快速幂、FFT |
| [appendix-ops.md](appendix-ops.md) | 操作序列：并查集、堆、树状数组、单调栈/队列 |
