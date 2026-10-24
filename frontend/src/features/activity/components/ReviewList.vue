<script setup lang="ts">
/** 错题本列表：未通过 / 挣扎题，按最近尝试时间倒序（排序由后端决定）。
 * 一行一道题：题号 + 题名（新标签打开原题）+ 平台 + 难度 + 状态 + 失败次数 + 最近尝试。
 * 错题可能上百条（一次渲染全部会拖慢滚动），故分页展示。
 * 具备题单能力的平台可展开"同类推荐"：首次展开才请求，结果按题缓存，收起不丢。 */

import { computed, reactive } from 'vue'
import { ChevronDown, ExternalLink, Inbox, ListPlus, Trash2 } from 'lucide-vue-next'
import { NButton, NPagination, NPopconfirm, NSpin, NTag } from 'naive-ui'
import {
  difficultyLabel,
  failLabel,
  relativeDay,
  statusLabel,
  statusTagType,
} from '@/features/activity/model/review'
import { canAddToTodo, showsTodoAction } from '@/features/activity/model/todo'
import { pageCount, paged } from '@/features/activity/model/pagination'
import { fetchReviewRecommend } from '@/features/activity/api'
import { useActivity } from '@/features/activity/store'
import type { RecommendItem, ReviewItem } from '@/features/activity/types'

const props = defineProps<{
  items: ReviewItem[]
  /** 空态文案（未绑定 / 未同步 / 没有错题） */
  emptyHint: string
  /** 空态下是否展示引导操作 */
  emptyAction?: string
  /** 当前页码（从 1 起） */
  page: number
  /** 正在加入待办的行键（"platform/problemKey"），用于按钮 loading */
  todoPending?: string | null
}>()

const emit = defineEmits<{
  'empty-action': []
  'update:page': [page: number]
  /** 删除手动收录项（父级调接口并本地移除） */
  remove: [item: ReviewItem]
  /** 加入待办（父级调接口，成功后就地标记 inTodo） */
  'add-todo': [item: ReviewItem]
}>()

const { platformName } = useActivity()

/** 错题本每页条数 */
const PAGE_SIZE = 25

const hasItems = computed(() => props.items.length > 0)
const totalPages = computed(() => pageCount(props.items.length, PAGE_SIZE))
const rows = computed(() => paged(props.items, props.page, PAGE_SIZE))

/** 展开的推荐面板：key → 一次请求的状态（加载中 / 结果 / 失败文案） */
interface RecommendState {
  loading: boolean
  error: string | null
  items: RecommendItem[]
  stale: boolean
  reason: 'not_indexed' | 'no_match' | null
}

const panels = reactive<Record<string, RecommendState>>({})
const expanded = reactive<Record<string, boolean>>({})

function rowKey(item: ReviewItem): string {
  return `${item.platform}/${item.problemKey}`
}

function toggle(item: ReviewItem): void {
  const key = rowKey(item)
  if (expanded[key]) {
    // 收起只折叠，不清缓存：再次展开不重复请求
    expanded[key] = false
    return
  }
  expanded[key] = true
  const state = panels[key]
  if (state && (state.loading || state.items.length > 0)) return
  void load(key, item)
}

async function load(key: string, item: ReviewItem): Promise<void> {
  panels[key] = { loading: true, error: null, items: [], stale: false, reason: null }
  try {
    const res = await fetchReviewRecommend(item.platform, item.problemKey)
    panels[key] = {
      loading: false,
      error: null,
      items: res.items,
      stale: res.stale,
      reason: res.reason ?? null,
    }
  } catch (e) {
    // 推荐失败不影响错题列表本身：面板内就地提示
    panels[key] = {
      loading: false,
      error: e instanceof Error ? e.message : '推荐加载失败',
      items: [],
      stale: false,
      reason: null,
    }
  }
}

/** 空结果文案：区分"题库没收录这道题"与"有收录但没有相近题" */
function emptyText(state: RecommendState | undefined): string {
  if (state?.reason === 'not_indexed') {
    return '这道题不在平台的公开题库里（如比赛专属题），无法匹配同类题。'
  }
  return '没有找到标签与难度相近的未做题。'
}
</script>

<template>
  <div class="review-list">
    <div v-if="!hasItems" class="review-empty">
      <Inbox :size="22" class="empty-icon" />
      <p class="empty-text">{{ emptyHint }}</p>
      <NButton
        v-if="emptyAction"
        size="small"
        type="primary"
        secondary
        @click="emit('empty-action')"
      >
        {{ emptyAction }}
      </NButton>
    </div>

    <div v-for="item in rows" :key="rowKey(item)" class="review-block">
      <article class="review-row">
        <span class="row-key">{{ item.problemKey }}</span>
        <a
          class="row-name"
          :href="item.problemUrl"
          target="_blank"
          rel="noopener noreferrer"
        >
          {{ item.problemName }}
          <ExternalLink :size="12" />
        </a>
        <span class="row-platform">{{ platformName(item.platform) }}</span>
        <span class="row-difficulty">{{ difficultyLabel(item.difficulty) }}</span>
        <NTag size="small" :bordered="false" :type="statusTagType(item.status)">
          {{ statusLabel(item.status) }}
        </NTag>
        <span class="row-fails">{{ failLabel(item) }}</span>
        <span class="row-time">{{ relativeDay(item.lastAttemptAt) }}</span>
        <NButton
          v-if="showsTodoAction(item)"
          class="row-todo"
          size="tiny"
          :quaternary="!canAddToTodo(item)"
          :secondary="canAddToTodo(item)"
          :disabled="!canAddToTodo(item)"
          :loading="todoPending === rowKey(item)"
          @click="emit('add-todo', item)"
        >
          <template #icon><ListPlus :size="13" /></template>
          {{ item.inTodo ? '已待办' : '待办' }}
        </NButton>
        <NPopconfirm v-if="item.manual" @positive-click="emit('remove', item)">
          <template #trigger>
            <NButton class="row-delete" size="tiny" quaternary aria-label="删除收录">
              <Trash2 :size="13" />
            </NButton>
          </template>
          从错题本移除这道收录题？
        </NPopconfirm>
        <NButton
          v-if="item.canRecommend"
          class="row-toggle"
          size="tiny"
          quaternary
          :aria-expanded="!!expanded[rowKey(item)]"
          @click="toggle(item)"
        >
          <ChevronDown
            :size="14"
            class="toggle-icon"
            :class="{ 'is-open': expanded[rowKey(item)] }"
          />
          {{ expanded[rowKey(item)] ? '收起' : '同类题' }}
        </NButton>
      </article>

      <div v-if="expanded[rowKey(item)]" class="recommend-panel">
        <NSpin v-if="panels[rowKey(item)]?.loading" size="small" />
        <p v-else-if="panels[rowKey(item)]?.error" class="panel-hint error">
          {{ panels[rowKey(item)]?.error }}
        </p>
        <p
          v-else-if="!panels[rowKey(item)] || panels[rowKey(item)]?.items.length === 0"
          class="panel-hint"
        >
          {{ emptyText(panels[rowKey(item)]) }}
        </p>
        <template v-else>
          <p v-if="panels[rowKey(item)]?.stale" class="panel-hint">
            题单刷新失败，以下为上次缓存结果。
          </p>
          <ul class="recommend-items">
            <li v-for="rec in panels[rowKey(item)]?.items" :key="rec.problemKey">
              <span class="rec-key">{{ rec.problemKey }}</span>
              <a
                class="rec-name"
                :href="rec.url"
                target="_blank"
                rel="noopener noreferrer"
              >
                {{ rec.name }}
                <ExternalLink :size="11" />
              </a>
              <span class="rec-rating">{{ rec.rating ?? '—' }}</span>
              <span class="rec-tags">{{ rec.tags.slice(0, 3).join(' · ') }}</span>
            </li>
          </ul>
        </template>
      </div>
    </div>

    <div v-if="totalPages > 1" class="review-pager">
      <NPagination
        :page="page"
        :page-count="totalPages"
        :page-slot="7"
        size="small"
        @update:page="emit('update:page', $event)"
      />
    </div>
  </div>
</template>

<style scoped>
.review-list {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.review-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
  padding: 48px 16px;
  color: var(--muted);
}

.review-empty .empty-icon {
  opacity: 0.7;
}

.empty-text {
  margin: 0;
  font-size: 13px;
}

.review-block {
  display: flex;
  flex-direction: column;
}

.review-row {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 9px 12px;
  border: 1px solid var(--border);
  border-radius: 10px;
  background: var(--surface);
  font-size: 12.5px;
  transition: border-color 0.16s ease;
}

.review-row:hover {
  border-color: var(--accent-soft);
}

.row-key {
  flex: none;
  min-width: 62px;
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-weight: 600;
  color: var(--text);
}

.row-name {
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  display: inline-flex;
  align-items: center;
  gap: 4px;
  color: var(--text);
  text-decoration: none;
}

.row-name:hover {
  color: var(--accent-strong);
}

.row-platform {
  flex: none;
  color: var(--muted);
}

.row-difficulty {
  flex: none;
  min-width: 34px;
  text-align: right;
  color: var(--muted);
  font-variant-numeric: tabular-nums;
}

.row-fails,
.row-time {
  flex: none;
  color: var(--muted);
  font-variant-numeric: tabular-nums;
}

.row-time {
  min-width: 52px;
  text-align: right;
}

.row-toggle {
  flex: none;
  display: inline-flex;
  align-items: center;
  gap: 2px;
  font-size: 12px;
}

.row-delete {
  flex: none;
  color: var(--muted);
}

.row-delete:hover {
  color: #c63b57;
}

.row-todo {
  flex: none;
  font-size: 12px;
}

.toggle-icon {
  transition: transform 0.16s ease;
}

.toggle-icon.is-open {
  transform: rotate(180deg);
}

.recommend-panel {
  margin: 4px 0 2px 12px;
  padding: 8px 12px;
  border-left: 2px solid var(--accent-soft);
  font-size: 12px;
}

.panel-hint {
  margin: 0;
  color: var(--muted);
}

.panel-hint.error {
  color: var(--danger, #d03050);
}

.recommend-items {
  margin: 0;
  padding: 0;
  list-style: none;
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.recommend-items li {
  display: flex;
  align-items: center;
  gap: 8px;
  min-width: 0;
}

.rec-key {
  flex: none;
  min-width: 58px;
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  color: var(--muted);
}

.rec-name {
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  display: inline-flex;
  align-items: center;
  gap: 4px;
  color: var(--text);
  text-decoration: none;
}

.rec-name:hover {
  color: var(--accent-strong);
}

.rec-rating {
  flex: none;
  min-width: 30px;
  text-align: right;
  color: var(--muted);
  font-variant-numeric: tabular-nums;
}

.rec-tags {
  flex: none;
  max-width: 220px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  color: var(--muted);
}

.review-pager {
  display: flex;
  justify-content: center;
  padding: 10px 0 2px;
}
</style>
