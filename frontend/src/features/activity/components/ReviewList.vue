<script setup lang="ts">
/** 错题本列表：未通过 / 挣扎题，按最近尝试时间倒序（排序由后端决定）。
 * 一行一道题：题号 + 题名（新标签打开原题）+ 平台 + 难度 + 状态 + 失败次数 + 最近尝试。
 * 错题可能上百条（一次渲染全部会拖慢滚动），故分页展示。 */

import { computed } from 'vue'
import { ExternalLink, Inbox } from 'lucide-vue-next'
import { NButton, NPagination, NTag } from 'naive-ui'
import {
  difficultyLabel,
  failLabel,
  relativeDay,
  statusLabel,
} from '@/features/activity/model/review'
import { pageCount, paged } from '@/features/activity/model/pagination'
import { useActivity } from '@/features/activity/store'
import type { ReviewItem } from '@/features/activity/types'

const props = defineProps<{
  items: ReviewItem[]
  /** 空态文案（未绑定 / 未同步 / 没有错题） */
  emptyHint: string
  /** 空态下是否展示引导操作 */
  emptyAction?: string
  /** 当前页码（从 1 起） */
  page: number
}>()

const emit = defineEmits<{
  'empty-action': []
  'update:page': [page: number]
}>()

const { platformName } = useActivity()

/** 错题本每页条数 */
const PAGE_SIZE = 25

const hasItems = computed(() => props.items.length > 0)
const totalPages = computed(() => pageCount(props.items.length, PAGE_SIZE))
const rows = computed(() => paged(props.items, props.page, PAGE_SIZE))
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

    <article v-for="item in rows" :key="`${item.platform}/${item.problemKey}`" class="review-row">
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
      <NTag
        size="small"
        :bordered="false"
        :type="item.status === 'unsolved' ? 'error' : 'warning'"
      >
        {{ statusLabel(item.status) }}
      </NTag>
      <span class="row-fails">{{ failLabel(item) }}</span>
      <span class="row-time">{{ relativeDay(item.lastAttemptAt) }}</span>
    </article>

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

.review-pager {
  display: flex;
  justify-content: center;
  padding: 10px 0 2px;
}
</style>
