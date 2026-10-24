/** activity 后端 API 客户端（与后端 modules/activity/schemas.py 对齐）。 */

import { request } from '@/shared/api/client'
import type {
  AccountCredentials,
  BoundAccount,
  DayActivity,
  ManualReviewResult,
  OverviewTotals,
  PlatformId,
  PlatformMeta,
  ProblemTag,
  RecommendData,
  RefineStatus,
  ReviewData,
  ReviewFilter,
  SearchProblemsData,
  SubmissionEntry,
} from '@/features/activity/types'
import type { PlatformScope } from '@/features/activity/store'

export interface ApiPlatformMeta extends PlatformMeta {
  capabilities: string[]
  /** 该平台当前绑定账号；未绑定为 null */
  account: BoundAccount | null
}

export interface ApiPlatformsResponse {
  platforms: ApiPlatformMeta[]
}

export interface ApiVerifyResponse {
  platform: string
  handle: string
  displayName: string | null
  avatar: string | null
}

/** 浏览器一键登录会话状态（与后端 BrowserLoginStatusOut 对齐） */
export interface ApiBrowserLoginStatus {
  state: 'waiting' | 'success' | 'canceled' | 'timeout' | 'error'
  handle: string | null
  displayName: string | null
  avatar: string | null
  error: string | null
}

export interface ApiOverviewResponse {
  totals: OverviewTotals
  /** 近约 370 天日序列，升序，末尾为今天 */
  daily: DayActivity[]
}

/** 提交条目：date 为本地时区日期（近期提交与当日明细共用） */
export interface ApiSubmission extends SubmissionEntry {
  date: string
}

export interface ApiSubmissionsResponse {
  items: ApiSubmission[]
}

function scopeParam(scope: PlatformScope): string {
  return scope === 'all' ? '' : scope
}

export function fetchPlatforms(): Promise<ApiPlatformsResponse> {
  return request<ApiPlatformsResponse>('/activity/platforms')
}

export function verifyAccount(
  platform: string,
  handle: string,
  credentials?: AccountCredentials,
): Promise<ApiVerifyResponse> {
  return request<ApiVerifyResponse>('/activity/accounts/verify', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ platform, handle, ...(credentials ? { credentials } : {}) }),
  })
}

export function bindAccount(
  platform: string,
  handle: string,
  opts: { displayName?: string | null; credentials?: AccountCredentials } = {},
): Promise<BoundAccount> {
  return request<BoundAccount>('/activity/accounts', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      platform,
      handle,
      ...(opts.displayName ? { displayName: opts.displayName } : {}),
      ...(opts.credentials ? { credentials: opts.credentials } : {}),
    }),
  })
}

/** 启动浏览器一键登录会话（202 立即返回；轮询 fetchBrowserLoginStatus） */
export function startBrowserLogin(platform: PlatformId): Promise<void> {
  return request<void>(`/activity/platforms/${platform}/browser-login`, { method: 'POST' })
}

export function fetchBrowserLoginStatus(platform: PlatformId): Promise<ApiBrowserLoginStatus> {
  return request<ApiBrowserLoginStatus>(`/activity/platforms/${platform}/browser-login/status`)
}

export function updateCredentials(
  platform: string,
  handle: string,
  credentials?: AccountCredentials,
): Promise<BoundAccount> {
  const path = `/activity/accounts/${encodeURIComponent(platform)}/${encodeURIComponent(handle)}/credentials`
  return request<BoundAccount>(path, {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(credentials ? { credentials } : {}),
  })
}

export function unbindAccount(platform: string, handle: string): Promise<void> {
  const path = `/activity/accounts/${encodeURIComponent(platform)}/${encodeURIComponent(handle)}`
  return request<void>(path, { method: 'DELETE' })
}

export function fetchOverview(scope: PlatformScope = 'all'): Promise<ApiOverviewResponse> {
  const platform = scopeParam(scope)
  return request<ApiOverviewResponse>(`/activity/overview${platform ? `?platform=${platform}` : ''}`)
}

export function fetchSubmissions(opts: {
  date?: string | null
  platform?: PlatformScope
} = {}): Promise<ApiSubmissionsResponse> {
  const params = new URLSearchParams()
  if (opts.date) params.set('date', opts.date)
  const platform = opts.platform ? scopeParam(opts.platform) : ''
  if (platform) params.set('platform', platform)
  const qs = params.toString()
  return request<ApiSubmissionsResponse>(`/activity/submissions${qs ? `?${qs}` : ''}`)
}

export function triggerSync(platform?: PlatformId): Promise<void> {
  return request<void>('/activity/sync', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(platform ? { platform } : {}),
  })
}

export function fetchSyncStatus(): Promise<BoundAccount[]> {
  return request<BoundAccount[]>('/activity/sync/status')
}

/* ---------- 错题本（review） ---------- */

export function fetchReview(opts: {
  platform?: PlatformScope
  status?: ReviewFilter
} = {}): Promise<ReviewData> {
  const params = new URLSearchParams()
  const platform = opts.platform ? scopeParam(opts.platform) : ''
  if (platform) params.set('platform', platform)
  if (opts.status && opts.status !== 'all') params.set('status', opts.status)
  const qs = params.toString()
  return request<ReviewData>(`/activity/review${qs ? `?${qs}` : ''}`)
}

/** 同类推荐：按题单标签与难度挑未 AC 过的题（平台须具备题目能力） */
export function fetchReviewRecommend(
  platform: string,
  problemKey: string,
): Promise<RecommendData> {
  const params = new URLSearchParams({ platform, problemKey })
  return request<RecommendData>(`/activity/review/recommend?${params.toString()}`)
}

/** 收录一题：传链接（后端解析平台与题号），或直接传平台 + 题号 */
export function addReviewManual(input: {
  url?: string
  platform?: string
  problemKey?: string
}): Promise<ManualReviewResult> {
  return request<ManualReviewResult>('/activity/review/manual', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(input),
  })
}

/** 删除手动收录项（只作用于手动清单，不影响提交聚合出的记录） */
export function removeReviewManual(platform: string, problemKey: string): Promise<void> {
  const params = new URLSearchParams({ platform, problemKey })
  return request<void>(`/activity/review/manual?${params.toString()}`, {
    method: 'DELETE',
  })
}

/** 算法标签目录（按算法查找的筛选项） */
export function fetchReviewTags(platform: string): Promise<{ items: ProblemTag[] }> {
  const params = new URLSearchParams({ platform })
  return request<{ items: ProblemTag[] }>(`/activity/review/tags?${params.toString()}`)
}

/** 按标签浏览题库一页（每页 50；solved / collected 由后端标注） */
export function searchReviewProblems(
  platform: string,
  tag: string,
  page = 1,
): Promise<SearchProblemsData> {
  const params = new URLSearchParams({ platform, tag, page: String(page) })
  return request<SearchProblemsData>(`/activity/review/search?${params.toString()}`)
}

/* ---------- 精细化同步（REFINE_VERDICT 能力平台） ---------- */
function refinePath(platform: string, handle: string): string {
  return `/activity/accounts/${encodeURIComponent(platform)}/${encodeURIComponent(handle)}/refine`
}

export function fetchRefineStatus(platform: string, handle: string): Promise<RefineStatus> {
  return request<RefineStatus>(refinePath(platform, handle))
}

export function startRefine(platform: string, handle: string): Promise<void> {
  return request<void>(refinePath(platform, handle), { method: 'POST' })
}

export function stopRefine(platform: string, handle: string): Promise<void> {
  return request<void>(refinePath(platform, handle), { method: 'DELETE' })
}

export function setRefineAuto(
  platform: string,
  handle: string,
  refineAuto: boolean,
): Promise<RefineStatus> {
  return request<RefineStatus>(
    `/activity/accounts/${encodeURIComponent(platform)}/${encodeURIComponent(handle)}`,
    {
      method: 'PATCH',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ refineAuto }),
    },
  )
}

/* ---------- 用户组与信息卡 ---------- */

export interface ApiGroup {
  name: string
  current: boolean
}

export interface ApiGroupsResponse {
  groups: ApiGroup[]
}

export interface ApiProfile {
  id: string
  signature: string
  avatar: string | null
}

export function fetchGroups(): Promise<ApiGroupsResponse> {
  return request<ApiGroupsResponse>('/activity/groups')
}

export function createGroup(name: string): Promise<ApiGroup> {
  return request<ApiGroup>('/activity/groups', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ name }),
  })
}

export function renameGroup(name: string, newName: string): Promise<ApiGroupsResponse> {
  return request<ApiGroupsResponse>(
    `/activity/groups/${encodeURIComponent(name)}`,
    {
      method: 'PATCH',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ newName }),
    },
  )
}

export function deleteGroup(name: string): Promise<void> {
  return request<void>(`/activity/groups/${encodeURIComponent(name)}`, {
    method: 'DELETE',
  })
}

export function switchGroup(name: string): Promise<ApiGroup> {
  return request<ApiGroup>('/activity/current-group', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ name }),
  })
}

export function fetchProfile(): Promise<ApiProfile> {
  return request<ApiProfile>('/activity/profile')
}

export function updateProfile(payload: {
  id?: string
  signature?: string
  avatar?: string | null
}): Promise<ApiProfile> {
  return request<ApiProfile>('/activity/profile', {
    method: 'PATCH',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  })
}
