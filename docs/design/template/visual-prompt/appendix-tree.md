# 类型附录：树

发给 AI 时整段复制「附录正文」。二叉树 / 线段树 / Trie / 平衡树 / LCA 等必附；布局函数见 [helpers.md](helpers.md)（骨架已内置）。

---

【类型附录：树】
本模板是树结构（二叉树/线段树/Trie/平衡树/LCA 等）。除通用要求外，额外遵守：

1. 节点必须有稳定 id（数字），不能用数组下标当 id。
   旋转、插入后，同一个节点的 id 不变，否则无法对比前后帧。
2. 输入格式：
   - 操作序列型（BST、平衡树、Trie）：每行一个操作，如 "insert 5"、"delete 3"、"find 7"。
   - 静态结构型（LCA、树 DP）：第一行 n，之后 n-1 行 u v。
   MAX_N = 15 个节点；操作序列最多 20 步。
3. 每帧存整棵结构的拷贝，不能存引用。至少包括：
   left[] / right[] / val[] / root，多叉树存 children[]（二维数组要 map(a => a.slice()) 拷贝）。
   额外信息（size、高度、lazy、tag、平衡因子）也要放进快照。
4. 布局每帧重新计算：
   - 二叉树用 layoutBinary(root, left, right)
   - 多叉树用 layoutTree(root, children)
   再用 svgBox 算 viewBox。结构变化后，位置自然更新。
5. 渲染：
   - 边用 edgeSvg(directed:false)，点画圆加 val 文字。
   - 当前访问的节点 var(--accent)；已处理 var(--ok)；路径/将要处理 var(--warn)；其余默认。
   - 线段树：节点里同时显示区间 [l,r] 和聚合值，区间写在节点下方小字。
   - Trie：边上标字符（用 textSvg），终止节点画双圈。
   - LCA：同时用颜色标出 u、v、当前祖先。
6. 旋转类操作（AVL、Splay、Treap）要拆成多帧：
   「发现不平衡」→「旋转前」→「旋转后」，而不是一帧完成。
7. note 要写出节点值和判断，例如「5 < 8，走左子树」。
8. lines 要对应模板中的递归/循环位置；递归进入和返回都算一步。
