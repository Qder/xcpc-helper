#!/usr/bin/env bash
# XCPC Helper 启动脚本（Linux/macOS）：与 start.bat 等价。
# 优先使用包内自带的 uv 二进制（免安装包）；否则退回 PATH 中的 uv（源码检出）。
set -e
cd "$(dirname "$0")"

if [ -x "./uv" ]; then UV="./uv"; else UV="uv"; fi

# 安装位置可能与 uv 缓存不在同一文件系统（硬链接不可用），
# 使用 copy 模式避免告警。
export UV_LINK_MODE=copy

# 源码检出不含 frontend/dist；缺失时自动构建一次。
# Release 包已带 dist，且不含 package.json。
if [ ! -f frontend/dist/index.html ]; then
  if [ ! -f frontend/package.json ]; then
    echo "缺少前端文件。请使用 Release 压缩包，或克隆完整仓库。"
    exit 1
  fi
  if ! command -v npm >/dev/null 2>&1; then
    echo "未找到 npm。请安装 Node.js，或先运行 scripts/dev.sh。"
    exit 1
  fi
  echo "未找到前端构建产物，正在构建（需要 Node.js，首次可能需要几分钟）..."
  echo ""
  (
    cd frontend
    if [ ! -d node_modules ]; then
      npm ci
    fi
    npm run build
  )
  echo ""
fi

echo "============================================================"
echo "  XCPC Helper"
echo ""
echo "  首次运行会自动下载 Python 与依赖，需要几分钟（需联网）。"
echo "  之后启动即时完成，离线可用。"
echo ""
echo "  服务就绪后会打开浏览器：http://127.0.0.1:8000"
echo "  使用期间请保持本窗口不要关闭。"
echo "============================================================"
echo ""

open_browser() {
  if command -v xdg-open >/dev/null 2>&1; then
    xdg-open "$1" >/dev/null 2>&1 || true
  elif command -v open >/dev/null 2>&1; then
    open "$1" >/dev/null 2>&1 || true
  fi
}

# 等到 API 就绪再打开浏览器（首次下载 Python + 启动同步可能要几分钟）。
(
  for _ in $(seq 1 600); do
    if command -v curl >/dev/null 2>&1; then
      if curl -sf -o /dev/null --max-time 2 "http://127.0.0.1:8000/api/diagnostics"; then
        open_browser "http://127.0.0.1:8000"
        exit 0
      fi
    elif (echo >/dev/tcp/127.0.0.1/8000) >/dev/null 2>&1; then
      open_browser "http://127.0.0.1:8000"
      exit 0
    fi
    sleep 1
  done
) >/dev/null 2>&1 &

"$UV" run --directory backend --frozen uvicorn --app-dir src main:app --host 127.0.0.1 --port 8000
