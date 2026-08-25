#include "obs-preview.hpp"

#include <QHideEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QtGlobal>

#ifdef _WIN32
#include <Windows.h>
#endif

ObsPreview::ObsPreview(obs_source_t *source, QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setMinimumSize(76, 43);
    setSource(source);
}

ObsPreview::~ObsPreview()
{
    destroyDisplay();
    setSource(nullptr);
}

void ObsPreview::setSource(obs_source_t *source)
{
    if (source == source_)
        return;

    if (source_) {
        if (showing_)
            obs_source_dec_showing(source_);
        obs_source_release(source_);
    }

    source_ = source ? obs_source_get_ref(source) : nullptr;

    if (source_ && showing_)
        obs_source_inc_showing(source_);
}

void ObsPreview::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!showing_ && source_) {
        obs_source_inc_showing(source_);
        showing_ = true;
    }
    createDisplay();
}

void ObsPreview::hideEvent(QHideEvent *event)
{
    destroyDisplay();
    if (showing_ && source_) {
        obs_source_dec_showing(source_);
        showing_ = false;
    }
    QWidget::hideEvent(event);
}

void ObsPreview::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (display_)
        obs_display_resize(display_, static_cast<uint32_t>(width()),
                           static_cast<uint32_t>(height()));
}

void ObsPreview::createDisplay()
{
    if (display_ || !isVisible() || width() < 2 || height() < 2)
        return;

    gs_init_data info{};
    info.cx = static_cast<uint32_t>(width());
    info.cy = static_cast<uint32_t>(height());
    info.format = GS_BGRA;
    info.zsformat = GS_ZS_NONE;

#ifdef _WIN32
    info.window.hwnd = reinterpret_cast<HWND>(winId());
#else
#error This core build currently targets Windows OBS only.
#endif

    display_ = obs_display_create(&info, 0xFF000000);
    if (display_)
        obs_display_add_draw_callback(display_, &ObsPreview::draw, this);
}

void ObsPreview::destroyDisplay()
{
    if (!display_)
        return;
    obs_display_remove_draw_callback(display_, &ObsPreview::draw, this);
    obs_display_destroy(display_);
    display_ = nullptr;
}

void ObsPreview::draw(void *data, uint32_t cx, uint32_t cy)
{
    auto *self = static_cast<ObsPreview *>(data);
    if (!self->source_)
        return;

    const uint32_t sw = obs_source_get_width(self->source_);
    const uint32_t sh = obs_source_get_height(self->source_);
    if (!sw || !sh || !cx || !cy)
        return;

    const float scale = qMin(static_cast<float>(cx) / static_cast<float>(sw),
                             static_cast<float>(cy) / static_cast<float>(sh));
    const float drawW = static_cast<float>(sw) * scale;
    const float drawH = static_cast<float>(sh) * scale;
    const float x = (static_cast<float>(cx) - drawW) * 0.5f;
    const float y = (static_cast<float>(cy) - drawH) * 0.5f;

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(0.0f, static_cast<float>(cx), 0.0f, static_cast<float>(cy),
             -100.0f, 100.0f);
    gs_set_viewport(0, 0, cx, cy);
    gs_matrix_push();
    gs_matrix_translate3f(x, y, 0.0f);
    gs_matrix_scale3f(scale, scale, 1.0f);
    obs_source_video_render(self->source_);
    gs_matrix_pop();
    gs_projection_pop();
    gs_viewport_pop();
}
