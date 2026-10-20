---
description: 用户要把某算法做成可视化 / 生成 visual.html 时，按分类调用提示词并写入版本目录
alwaysApply: true
---

# 算法可视化生成（调度）

用户只要提到「把某某算法可视化」「生成 visual.html」「做算法演示页」，**立刻**按本规则做，不要再问用哪份提示词、要不要手贴骨架。

## 流程

1. 在 `backend/content/` 里定位该模板的**版本目录**（代码 + `README.md` 所在目录）。找不到就先问分类/模板名，不要空写。
2. **按顺序读完**再生成，禁止凭印象：
   1. [../design/template/visual-prompt/core.md](../design/template/visual-prompt/core.md)
   2. 按下表选用附录（可多份），读全文
   3. 图 / 树再读 [../design/template/visual-prompt/helpers.md](../design/template/visual-prompt/helpers.md)
   4. [../design/template/visual-skeleton.html](../design/template/visual-skeleton.html)
   5. 该版本的源代码与 `README.md`（没有说明当「无」）
3. 把生成的完整 HTML **写入该版本目录的 `visual.html`**，与代码、README 并排。不要放进 `docs/`，不要只按主标签放一份。若 `visual.html` 已存在，先复制到同目录 `.visual-history/YYYYMMDD-HHMMSSZ.html`（UTC；撞名加 `-2`）再覆盖。
4. 只改骨架里 `ALGO START`～`ALGO END` 以及供 `render` 用的样式；播放器、控制条、快捷键、主题监听、布局函数一律不动。
5. 不要提交 `visual.html`，除非用户明确要求提交。

拼装说明与分类细则见 [../design/template/visual-prompt/README.md](../design/template/visual-prompt/README.md)。

## 分类 → 附录

以 `content/` 一级目录名为准：

| 分类 | 读这个附录 |
| --- | --- |
| 图论 | `appendix-graph.md` |
| 树论 | `appendix-tree.md` |
| 动态规划、背包 | `appendix-array.md` |
| 字符串 | `appendix-string.md` |
| 数学、数论 | `appendix-math.md` |
| 数据结构 | `appendix-ops.md`（平衡树 / Trie / 线段树改用 `appendix-tree.md`） |
| 树上 DP | `appendix-tree.md` + `appendix-array.md` |
| 树链剖分 | `appendix-tree.md` + `appendix-ops.md` |
