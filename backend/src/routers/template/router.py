"""模板库 API 路由。

路由层保持薄：参数解析 + 调用 service。service 为同步实现
（SQLite 同步驱动），通过 asyncio.to_thread 避免阻塞事件循环。
"""

import asyncio
from typing import Annotated

from fastapi import APIRouter, Depends, Query, status
from fastapi.responses import Response

from modules.template.schemas import (
    CategoryInfo,
    DiagnosticItem,
    DiagnosticsResponse,
    ReloadResponse,
    SortMode,
    TemplateBatchDelete,
    TemplateCreate,
    TemplateDetail,
    TemplateRename,
    TemplateSummary,
    VersionUpsert,
    VisualHistoryList,
    VisualUpsert,
)
from services.template.service import TemplateService, get_template_service

router = APIRouter(prefix="/api", tags=["template"])

ServiceDep = Annotated[TemplateService, Depends(get_template_service)]

VISUAL_CSP = (
    "sandbox allow-scripts; default-src 'none'; script-src 'unsafe-inline'; "
    "style-src 'unsafe-inline'; img-src data: blob:; font-src data:; connect-src 'none'"
)


@router.get("/templates", response_model=list[TemplateSummary])
async def list_templates(
    service: ServiceDep,
    category: Annotated[str | None, Query(description="分类目录名")] = None,
    tags: Annotated[list[str] | None, Query(description="标签过滤，多值取交集")] = None,
    keyword: Annotated[str | None, Query(description="关键词，搜标题/标签/说明/代码")] = None,
    sort: Annotated[SortMode, Query(description="排序方式，缺省 difficulty（难度降序）")] = "difficulty",
    has_visual: Annotated[
        bool | None, Query(description="true 时只返回至少一个版本有可视化的模板")
    ] = None,
) -> list[TemplateSummary]:
    return await asyncio.to_thread(
        service.list_templates,
        category=category,
        tags=tags or None,
        keyword=keyword,
        sort=sort,
        has_visual=has_visual,
    )


@router.get("/templates/{category}/{name}/versions/{version}/visual")
async def get_visual(
    category: str, name: str, version: str, service: ServiceDep
) -> Response:
    """GET .../versions/{version}/visual：托管 visual.html（"~" 表示顶层单版本）。"""
    data = await asyncio.to_thread(service.read_visual, category, name, version)
    return Response(
        content=data,
        media_type="text/html; charset=utf-8",
        headers={
            "X-Content-Type-Options": "nosniff",
            "Content-Security-Policy": VISUAL_CSP,
        },
    )


@router.put(
    "/templates/{category}/{name}/versions/{version}/visual",
    status_code=status.HTTP_204_NO_CONTENT,
)
async def put_visual(
    category: str, name: str, version: str, payload: VisualUpsert, service: ServiceDep
) -> None:
    """PUT .../versions/{version}/visual：写入 visual.html（"~" 表示顶层单版本）。"""
    await asyncio.to_thread(service.write_visual, category, name, version, payload)


@router.get(
    "/templates/{category}/{name}/versions/{version}/visual/history",
    response_model=VisualHistoryList,
)
async def list_visual_history(
    category: str, name: str, version: str, service: ServiceDep
) -> VisualHistoryList:
    """GET .../visual/history：该版本历史快照列表（新到旧）。"""
    return await asyncio.to_thread(
        service.list_visual_history, category, name, version
    )


@router.get("/templates/{category}/{name}/versions/{version}/visual/history/{snapshot_id}")
async def get_visual_history(
    category: str, name: str, version: str, snapshot_id: str, service: ServiceDep
) -> Response:
    """GET .../visual/history/{id}：托管一份历史快照 HTML。"""
    data = await asyncio.to_thread(
        service.read_visual_history, category, name, version, snapshot_id
    )
    return Response(
        content=data,
        media_type="text/html; charset=utf-8",
        headers={
            "X-Content-Type-Options": "nosniff",
            "Content-Security-Policy": VISUAL_CSP,
        },
    )


@router.post(
    "/templates/{category}/{name}/versions/{version}/visual/history/{snapshot_id}/restore",
    status_code=status.HTTP_204_NO_CONTENT,
)
async def restore_visual_history(
    category: str, name: str, version: str, snapshot_id: str, service: ServiceDep
) -> None:
    """POST .../visual/history/{id}/restore：把快照写回当前 visual.html。"""
    await asyncio.to_thread(
        service.restore_visual_history, category, name, version, snapshot_id
    )


@router.delete(
    "/templates/{category}/{name}/versions/{version}/visual/history/{snapshot_id}",
    status_code=status.HTTP_204_NO_CONTENT,
)
async def delete_visual_history_item(
    category: str, name: str, version: str, snapshot_id: str, service: ServiceDep
) -> None:
    """DELETE .../visual/history/{id}：删除一份快照。"""
    await asyncio.to_thread(
        service.delete_visual_history_item, category, name, version, snapshot_id
    )


@router.delete(
    "/templates/{category}/{name}/versions/{version}/visual/history",
    status_code=status.HTTP_204_NO_CONTENT,
)
async def clear_visual_history(
    category: str, name: str, version: str, service: ServiceDep
) -> None:
    """DELETE .../visual/history：清空该版本全部历史。"""
    await asyncio.to_thread(service.clear_visual_history, category, name, version)


@router.get("/templates/{template_id:path}", response_model=TemplateDetail)
async def get_template(template_id: str, service: ServiceDep) -> TemplateDetail:
    return await asyncio.to_thread(service.get_detail, template_id)


@router.post("/templates/reload", response_model=ReloadResponse)
async def reload_templates(service: ServiceDep) -> ReloadResponse:
    templates, diagnostics = await asyncio.to_thread(service.rebuild)
    return ReloadResponse(templates=templates, diagnostics=diagnostics)


@router.get("/categories", response_model=list[CategoryInfo])
async def list_categories(service: ServiceDep) -> list[CategoryInfo]:
    return await asyncio.to_thread(service.list_categories)


@router.get("/diagnostics", response_model=DiagnosticsResponse)
async def get_diagnostics(service: ServiceDep) -> DiagnosticsResponse:
    items = await asyncio.to_thread(service.diagnostics)
    return DiagnosticsResponse(
        items=[DiagnosticItem(**item.model_dump()) for item in items]
    )


# ===== 可视化增删改 =====
# 写路由一律用 {category}/{name} 双路径段（名称校验已禁止名称内含 "/"），
# 不使用 :path 转换器，避免与版本子路由产生匹配歧义。
# 顶层单版本（代码直接在模板目录下）在 URL 中用保留字 "~" 寻址。


@router.post(
    "/templates", response_model=TemplateDetail, status_code=status.HTTP_201_CREATED
)
async def create_template(
    payload: TemplateCreate, service: ServiceDep
) -> TemplateDetail:
    """POST /api/templates：新建空主标签（仅分类 + 模板名）。"""
    return await asyncio.to_thread(service.create_template, payload)


@router.put("/templates/{category}/{name}", response_model=TemplateDetail)
async def rename_template(
    category: str, name: str, payload: TemplateRename, service: ServiceDep
) -> TemplateDetail:
    """PUT /api/templates/{category}/{name}：主标签重命名/换分类。"""
    return await asyncio.to_thread(service.rename_template, category, name, payload)


@router.delete("/templates/{category}/{name}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_template(category: str, name: str, service: ServiceDep) -> None:
    """DELETE /api/templates/{category}/{name}：删除空主标签。"""
    await asyncio.to_thread(service.delete_template, category, name)


@router.post(
    "/templates/batch-delete", status_code=status.HTTP_204_NO_CONTENT
)
async def batch_delete_templates(
    payload: TemplateBatchDelete, service: ServiceDep
) -> None:
    """POST /api/templates/batch-delete：批量删除主标签（含全部版本）。"""
    await asyncio.to_thread(service.delete_templates, payload)


@router.post(
    "/templates/{category}/{name}/versions",
    response_model=TemplateDetail,
    status_code=status.HTTP_201_CREATED,
)
async def create_version(
    category: str, name: str, payload: VersionUpsert, service: ServiceDep
) -> TemplateDetail:
    """POST .../versions：在模板下新建副标签版本。"""
    return await asyncio.to_thread(service.create_version, category, name, payload)


@router.put(
    "/templates/{category}/{name}/versions/{version}", response_model=TemplateDetail
)
async def update_version(
    category: str, name: str, version: str, payload: VersionUpsert, service: ServiceDep
) -> TemplateDetail:
    """PUT .../versions/{version}：更新版本（"~" 表示顶层单版本）。"""
    return await asyncio.to_thread(
        service.update_version, category, name, version, payload
    )


@router.delete(
    "/templates/{category}/{name}/versions/{version}",
    status_code=status.HTTP_204_NO_CONTENT,
)
async def delete_version(
    category: str, name: str, version: str, service: ServiceDep
) -> None:
    """DELETE .../versions/{version}：删除版本（"~" 表示顶层单版本）。"""
    await asyncio.to_thread(service.delete_version, category, name, version)
