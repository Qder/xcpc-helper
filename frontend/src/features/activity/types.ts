/** 训练统计（activity）域类型。与后端 schemas 对齐；mock 阶段仅覆盖第一期范围。 */

export type PlatformId = 'codeforces' | 'atcoder' | 'luogu' | 'leetcode-cn' | 'nowcoder' | 'vjudge' | 'qoj'

/** 平台凭据（cookie 授权平台；与后端 adapters.base.Credentials 对齐） */
export interface AccountCredentials {
  cookies?: Record<string, string>
  headers?: Record<string, string>
}

export interface PlatformMeta {
  id: PlatformId
  name: string
  /** 凭据需求：none 匿名可取 / cookie 需登录授权（洛谷等） */
  auth: 'none' | 'cookie'
  /** 一键登录可用（cookie 平台且服务端具备浏览器登录能力） */
  browserLogin: boolean
  /** 平台主页 URL（前端跳转用） */
  homepageUrl: string
  /** 数据区块能力（refine_verdict = 支持 UNAC 精细化同步） */
  capabilities: string[]
}

/** 已绑定账号及其同步状态 */
export interface BoundAccount {
  platform: PlatformId
  handle: string
  /** 展示名（洛谷用户名等）；空则界面回退显示 handle */
  displayName?: string | null
  /** ISO 时间；null 表示从未同步成功 */
  lastSyncAt: string | null
  syncState: 'idle' | 'running' | 'error'
  /** syncState 为 error 时的诊断信息 */
  syncError?: string
  /** 结构化错误码；auth_expired = 凭据过期，引导重新授权 */
  syncErrorCode?: string | null
  /** 同步进度 0~1；null/缺省 = 总量未知（不定态） */
  syncProgress?: number | null
}

/** 单日聚合（后端按本地时区切天后返回） */
export interface DayActivity {
  /** YYYY-MM-DD（本地时区） */
  date: string
  submissions: number
  /** 当天 AC 的不同题目数 */
  solved: number
}

export interface OverviewTotals {
  totalSolved: number
  totalSubmissions: number
  todaySolved: number
  weekSolved: number
  streakDays: number
}

export interface OverviewData {
  totals: OverviewTotals
  /** 近约 370 天的日序列，按日期升序，末尾为今天 */
  daily: DayActivity[]
}

/** 评测结果；JG = 评测中（如 Codeforces 的 SUBMITTED / TESTING）；
 * UNAC = 未通过但细分未知（洛谷记录列表口径只有 AC/CE/Unaccepted） */
export type Verdict = 'AC' | 'WA' | 'CE' | 'RE' | 'TLE' | 'MLE' | 'OLE' | 'UKE' | 'JG' | 'UNAC'

export interface SubmissionEntry {
  id: string
  platform: PlatformId
  problemKey: string
  problemName: string
  problemUrl: string
  verdict: Verdict
  language: string
  /** HH:mm（本地时区） */
  time: string
}

/** 近期提交列表条目：带所属日期，跨天合并后按时间倒序展示 */
export interface RecentSubmission extends SubmissionEntry {
  /** YYYY-MM-DD（本地时区） */
  date: string
}

/** 精细化同步状态（与后端 RefineStatusOut 对齐） */
export type RefineState = 'idle' | 'running' | 'stopped' | 'done'

export interface RefineStatus {
  state: RefineState
  done: number
  /** idle 时为存量 UNAC 数（供耗时预估） */
  total: number
  /** 普通同步完成后自动精化 */
  auto: boolean
}

/* ---------- 错题本（review，见 docs/design/activity/review.md） ---------- */

/** 错题状态：未通过（从未 AC）/ 挣扎题（首次 AC 前失败达阈值） */
export type ReviewStatus = 'unsolved' | 'struggled'

/** 错题状态筛选（all 为不筛选） */
export type ReviewFilter = 'all' | ReviewStatus

export interface ReviewItem {
  platform: PlatformId
  problemKey: string
  problemName: string
  problemUrl: string
  /** 平台原始难度值（不跨平台归一） */
  difficulty: number | string | null
  status: ReviewStatus
  /** 首次 AC 之前的失败次数（未通过为全部失败数） */
  failCount: number
  /** 最近一次尝试（本地时区） */
  lastAttemptAt: string
  /** 该平台是否具备题单能力（决定展开箭头是否出现） */
  canRecommend: boolean
}

export interface ReviewData {
  items: ReviewItem[]
  /** 当前组是否绑定了任何账号（区分空态） */
  bound: boolean
  /** 是否至少有一个账号同步成功过 */
  synced: boolean
}

/** 同类推荐题（题单缓存的题目画像；本期仅 Codeforces 提供） */
export interface RecommendItem {
  problemKey: string
  name: string
  /** 平台原始难度值（CF 为分值） */
  rating: number | null
  tags: string[]
  url: string
}

export interface RecommendData {
  items: RecommendItem[]
  /** true = 本次用的是过期缓存（刷新失败后的降级） */
  stale: boolean
  /** 空结果原因：not_indexed = 源题不在平台题库；no_match = 没有相近题 */
  reason: 'not_indexed' | 'no_match' | null
}
