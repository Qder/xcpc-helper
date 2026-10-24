<script setup lang="ts">
/** 错题本页：把训练数据里"没做出来"和"做了很多次才过"的题挑出来。
 * 平台筛选复用后端平台注册表（不硬编码平台清单）；状态筛选本页独立，
 * 不写进网址（错题本不是主视图，刷新回到默认全部）。 */

import { computed, onMounted, ref, watch } from 'vue'
import { ChartNoAxesColumn } from 'lucide-vue-next'
import { NButton, NSkeleton, useMessage } from 'naive-ui'
import PlatformTabs from '@/features/activity/components/PlatformTabs.vue'
import ReviewList from '@/features/activity/components/ReviewList.vue'
import { fetchReview } from '@/features/activity/api'
import { useUserGroups } from '@/features/activity/profile'
import { useActivity, type PlatformScope } from '@/features/activity/store'
import type { ReviewData, ReviewFilter } from '@/features/activity/types'

const { init, activePlatform, setPlatform } = useActivity()
const { currentKey } = useUserGroups()
const message = useMessage()

const status = ref<ReviewFilter>('all')
const data = ref<ReviewData | null>(null)
const loading = ref(false)
const page = ref(1)

const STATUS_OPTIONS: { value: ReviewFilter; label: string }[] = [
  { value: 'all', label: '全部' },
  { value: 'unsolved', label: '未通过' },
  { value: 'struggled', label: '挣扎题' },
]

/** 空态文案三分：未绑定 / 未同步 / 没有错题（筛选后为空也说清是筛选导致） */
const emptyHint = computed(() => {
  if (!data.value) return ''
  if (!data.value.bound) return '还没有绑定账号，绑定并同步后才能挑出错题'
  if (!data.value.synced) return '账号还没有同步数据，先去总览页同步一次'
  if (status.value !== 'all') return '当前筛选下没有错题'
  return '没有错题，继续保持'
})

const emptyAction = computed(() => {
  if (!data.value) return undefined
  if (!data.value.bound) return '去绑定账号'
  if (!data.value.synced) return '去总览页同步'
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
</script>

<template>
  <div class="review-page">
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
        @empty-action="onEmptyAction"
      />
    </section>
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
