<script setup lang="ts">
/** 错题本页：把训练数据里"没做出来"和"做了很多次才过"的题挑出来，
 * 另外支持手动收录（粘贴链接 / 按算法查找收录）没有提交数据的题。
 * 平台筛选复用后端平台注册表（不硬编码平台清单）；状态筛选本页独立，
 * 不写进网址（错题本不是主视图，刷新回到默认全部）。 */

import { computed, onMounted, ref, watch } from 'vue'
import { ChartNoAxesColumn, Search } from 'lucide-vue-next'
import { NButton, NSkeleton, useMessage } from 'naive-ui'
import PlatformTabs from '@/features/activity/components/PlatformTabs.vue'
import ReviewList from '@/features/activity/components/ReviewList.vue'
import ReviewManualAdd from '@/features/activity/components/ReviewManualAdd.vue'
import ReviewTagSearch from '@/features/activity/components/ReviewTagSearch.vue'
import { fetchReview, addTodo, removeReviewManual } from '@/features/activity/api'
import { removeReviewItem, upsertReviewItem } from '@/features/activity/model/review'
import { markInTodo } from '@/features/activity/model/todo'
import { useUserGroups } from '@/features/activity/profile'
import { useActivity, type PlatformScope } from '@/features/activity/store'
import type { ReviewData, ReviewFilter, ReviewItem } from '@/features/activity/types'

const { init, activePlatform, setPlatform } = useActivity()
const { currentKey } = useUserGroups()
const message = useMessage()

const status = ref<ReviewFilter>('all')
const data = ref<ReviewData | null>(null)
const loading = ref(false)
const page = ref(1)
const tagSearchOpen = ref(false)
/** 正在加入待办的行键（"platform/problemKey"），用于按钮 loading 防重复点击 */
const todoPending = ref<string | null>(null)

const STATUS_OPTIONS: { value: ReviewFilter; label: string }[] = [
  { value: 'all', label: '全部' },
  { value: 'unsolved', label: '未通过' },
  { value: 'struggled', label: '挣扎题' },
  { value: 'collected', label: '收录' },
]

/** 空态文案三分：未绑定 / 未同步 / 没有错题（筛选后为空也说清是筛选导致）
 * 判断以合并后的列表（含手动收录项）为准 */
const emptyHint = computed(() => {
  if (!data.value) return ''
  if (!data.value.bound) return '还没有绑定账号，绑定并同步后才能挑出错题'
  if (!data.value.synced && data.value.items.length === 0) {
    return '账号还没有同步数据；也可以直接粘贴题目链接收录，或按算法查找题目'
  }
  if (status.value !== 'all') return '当前筛选下没有题目'
  return '没有错题，继续保持'
})

const emptyAction = computed(() => {
  if (!data.value) return undefined
  if (!data.value.bound) return '去绑定账号'
  if (!data.value.synced && data.value.items.length === 0) return '去总览页同步'
  return undefined
})

let requestSeq = 0

async function load(): Promise<void> {
  const seq = ++requestSeq
  loading.value = true
  try {
    const res = await fetchReview({ platform: activePlatform.value, status: status.value })
    if (seq !== requestSeq) return // 丢弃过期响应（快速切筛选时的竞态）
    data.value = res
  } catch (e) {
    if (seq !== requestSeq) return
    message.error(e instanceof Error ? e.message : '错题本加载失败')
  } finally {
    if (seq === requestSeq) loading.value = false
  }
}

onMounted(() => {
  init()
  void load()
})

watch([activePlatform, status, currentKey], () => {
  page.value = 1 // 换平台/状态/用户组后回到第 1 页
  void load()
})

/** 空态引导：未绑定与未同步都需要先回到总览页（绑定弹窗与同步都在那里） */
function onEmptyAction(): void {
  window.location.assign('/activity/overview')
}

function scope(value: PlatformScope): void {
  setPlatform(value)
}

/** 收录成功：并入当前列表（不整页刷新，保留筛选与滚动位置）。
 * 当前筛选是「未通过/挣扎题」时该收录项不该出现（状态不匹配），此时不插入。 */
function onAdded(item: ReviewItem, created: boolean): void {
  if (!data.value) return
  if (created && status.value !== 'all' && status.value !== 'collected') return
  // 当前平台筛选下收录了别的平台的题（粘贴链接可能来自任意平台）：不插入
  if (activePlatform.value !== 'all' && item.platform !== activePlatform.value) return
  data.value = { ...data.value, items: upsertReviewItem(data.value.items, item) }
  page.value = 1 // 新条目按时间倒序在最前
}

/** 删除手动收录：先落盘再改本地列表（失败时列表保持原样，避免假成功） */
async function onRemove(item: ReviewItem): Promise<void> {
  try {
    await removeReviewManual(item.platform, item.problemKey)
    if (data.value) {
      data.value = {
        ...data.value,
        items: removeReviewItem(data.value.items, item.platform, item.problemKey),
      }
    }
    message.success('已从错题本移除')
  } catch (e) {
    message.error(e instanceof Error ? e.message : '删除失败，请稍后重试')
  }
}

/** 加入待办：成功后就地把该行标成「已待办」（不整页刷新，保留筛选与滚动位置）。
 * 已 AC 的题后端会拒绝（400），按 detail 提示，前端不猜原因。 */
async function onAddTodo(item: ReviewItem): Promise<void> {
  const key = `${item.platform}/${item.problemKey}`
  if (todoPending.value) return
  todoPending.value = key
  try {
    const res = await addTodo(item.platform, item.problemKey)
    if (data.value) {
      data.value = {
        ...data.value,
        items: markInTodo(data.value.items, item.platform, item.problemKey),
      }
    }
    message.success(res.created ? '已加入待办' : '该题已在待办里')
  } catch (e) {
    message.error(e instanceof Error ? e.message : '加入待办失败，请稍后重试')
  } finally {
    todoPending.value = null
  }
}
</script>

<template>
  <div class="review-page">
    <div class="review-toolbar review-toolbar--top">
      <ReviewManualAdd @added="onAdded" />
      <NButton size="small" secondary @click="tagSearchOpen = true">
        <template #icon><Search :size="14" /></template>
        按算法查找
      </NButton>
    </div>

    <div class="review-toolbar">
      <PlatformTabs :model-value="activePlatform" @update:model-value="scope" />
      <div class="status-filter" role="group" aria-label="错题状态筛选">
        <button
          v-for="opt in STATUS_OPTIONS"
          :key="opt.value"
          type="button"
          class="filter-chip"
          :class="{ active: status === opt.value }"
          :aria-pressed="status === opt.value"
          @click="status = opt.value"
        >
          {{ opt.label }}
        </button>
      </div>
      <span class="review-count">{{ data ? `${data.items.length} 道` : '' }}</span>
    </div>

    <section class="review-panel">
      <div v-if="loading && !data" class="review-skeleton">
        <NSkeleton v-for="i in 6" :key="i" height="38px" :sharp="false" />
      </div>
      <div v-else-if="!data?.bound" class="review-empty-page">
        <div class="empty-icon">
          <ChartNoAxesColumn :size="26" />
        </div>
        <h2 class="empty-title">还没有绑定账号</h2>
        <p class="empty-hint">绑定一个竞赛平台账号并同步后，这里会列出你的错题。</p>
        <NButton type="primary" @click="onEmptyAction">去绑定账号</NButton>
      </div>
      <ReviewList
        v-else
        v-model:page="page"
        :items="data?.items ?? []"
        :empty-hint="emptyHint"
        :empty-action="emptyAction"
        :todo-pending="todoPending"
        @empty-action="onEmptyAction"
        @remove="onRemove"
        @add-todo="onAddTodo"
      />
    </section>

    <ReviewTagSearch
      v-model:show="tagSearchOpen"
      :initial-platform="activePlatform"
      @added="onAdded"
    />
  </div>
</template>

<style scoped>
.review-page {
  flex: 1;
  min-height: 0;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 12px;
  padding: 14px 20px 16px;
}

.review-toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
  flex: none;
}

.review-toolbar--top {
  gap: 8px;
}

/* 收录输入框占满剩余宽度，按钮靠右 */
.review-toolbar--top > :first-child {
  flex: 1;
  min-width: 0;
}

.status-filter {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px;
  border: 1px solid var(--border);
  border-radius: 99px;
  background: var(--surface);
}

.filter-chip {
  padding: 5px 12px;
  border: 0;
  border-radius: 99px;
  background: transparent;
  color: var(--muted);
  font-size: 12.5px;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
  transition: background 0.16s ease, color 0.16s ease;
}

.filter-chip:hover {
  background: var(--surface-2);
  color: var(--text);
}

.filter-chip.active {
  background: var(--accent-soft);
  color: var(--accent-strong);
}

.review-count {
  margin-left: auto;
  font-size: 12px;
  color: var(--muted);
  font-variant-numeric: tabular-nums;
}

.review-panel {
  flex: 1;
  min-height: 0;
  display: flex;
  flex-direction: column;
}

.review-skeleton {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.review-empty-page {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
  margin: auto;
  padding: 48px 16px;
  text-align: center;
}

.review-empty-page .empty-icon {
  color: var(--accent-strong);
  opacity: 0.8;
}

.empty-title {
  margin: 0;
  font-size: 15px;
  font-weight: 600;
  color: var(--text);
}

.empty-hint {
  margin: 0;
  font-size: 12.5px;
  color: var(--muted);
}
</style>
