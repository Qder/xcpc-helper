# 类型附录：图

发给 AI 时整段复制「附录正文」。图算法必附；布局函数见 [helpers.md](helpers.md)（骨架已内置）。

---

【类型附录：图】
本模板是图算法。除通用要求外，额外遵守：

1. 输入格式：第一行 n m，之后 m 行 u v w（无权图省略 w）。点编号 1..n。
   MAX_N = 12，边数不超过 30。parseInput 要校验点编号范围，并拒绝自环和重边（模板不支持时）。
2. 布局：在 parseInput 之后调用一次 layoutCircle，得到 pos 数组。
   坐标不随帧变化，保证每帧点的位置一致。
3. 每帧快照必须自包含，至少存这些，且都是拷贝：
   - dist / vis / par 等数组：用 slice() 拷贝
   - cur：当前点，没有时填 -1
   - curEdge：正在松弛、检查的边 [u, v]，没有时填 null
   - queue / stack / heap：拷贝成数组
   - treeEdges：已确定进入结果的边（MST、最短路树）
4. 渲染：
   - 先画所有边（用 edgeSvg），再画点，最后画边权（用 textSvg），保证标签不被线盖住。
   - 无向图 directed:false，有向图 directed:true。
   - 边权标签放在边的中点，垂直方向偏移 -6。
   - 点的状态着色：当前点 var(--accent)，已确定 var(--ok)，在队列/堆里 var(--warn)，其余 var(--faint)。
   - 当前检查的边用 var(--warn) 加粗；已入树的边用 var(--ok)。
5. 在舞台下方额外画一行 dist 表（dist[1..n]），当前被更新的位置高亮。
6. note 要写出具体数值，例如「松弛 3→5：dist[3]+w=7 < dist[5]=9，更新为 7」，不要写「进行松弛」。
7. 每一步都要有 lines：取点、遍历邻边、松弛成功、入队/入堆分别对应模板代码里的相应行。
8. 必须手动考虑的边界：不连通（dist 为 INF 时显示「∞」）、负权（若模板不支持，在 parseInput 里报错）。
