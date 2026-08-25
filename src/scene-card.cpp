#include "scene-card.hpp"
#include "obs-preview.hpp"

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QtGlobal>
#include <utility>

namespace {
constexpr auto MimeType = "application/x-vmix-dashboard-scene";
constexpr int MediaRowHeight = 18;

struct MediaSearch {
    obs_source_t *source = nullptr;
};

bool findMediaItem(obs_scene_t *, obs_sceneitem_t *item, void *data)
{
    auto *search = static_cast<MediaSearch *>(data);
    obs_source_t *source = obs_sceneitem_get_source(item);
    if (!source)
        return true;

    if (obs_source_media_get_duration(source) > 0) {
        search->source = obs_source_get_ref(source);
        return false;
    }

    if (obs_scene_t *nested = obs_scene_from_source(source)) {
        obs_scene_enum_items(nested, findMediaItem, data);
        if (search->source)
            return false;
    }
    return true;
}
}

SceneCard::SceneCard(obs_source_t *source, const QString &name, QWidget *parent)
    : QFrame(parent), sceneName_(name)
{
    setObjectName("sceneCard");
    setAcceptDrops(true);
    setCursor(Qt::PointingHandCursor);
    MediaSearch search;
    if (obs_scene_t *scene = obs_scene_from_source(source))
        obs_scene_enum_items(scene, findMediaItem, &search);
    mediaSource_ = search.source;

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);

    preview_ = new ObsPreview(source, this);
    preview_->setFixedSize(166, 93);
    label_ = new QLabel(sceneName_, this);
    label_->setAlignment(Qt::AlignCenter);
    label_->setFixedHeight(21);

    layout->addWidget(preview_);
    layout->addWidget(label_);

    if (mediaSource_) {
        mediaSlider_ = new QSlider(Qt::Horizontal, this);
        mediaSlider_->setRange(0, 1000);
        mediaSlider_->setFixedHeight(MediaRowHeight);
        mediaSlider_->setToolTip(QStringLiteral("영상 재생 위치"));
        layout->addWidget(mediaSlider_);

        connect(mediaSlider_, &QSlider::sliderPressed, this, [this] { seeking_ = true; });
        connect(mediaSlider_, &QSlider::sliderReleased, this, [this] {
            seekMedia();
            seeking_ = false;
        });

        mediaTimer_ = new QTimer(this);
        connect(mediaTimer_, &QTimer::timeout, this, [this] { updateMediaProgress(); });
        mediaTimer_->start(200);
        updateMediaProgress();
    } else {
        // Keep every card the same height, even when no seek bar is needed.
        layout->addSpacing(MediaRowHeight);
    }

    preview_->installEventFilter(this);
    label_->installEventFilter(this);
    setCardWidth(170);
    applyStyle();
}

SceneCard::~SceneCard()
{
    if (mediaSource_)
        obs_source_release(mediaSource_);
}

void SceneCard::setCardWidth(int width)
{
    width = qMax(80, width);
    const int previewWidth = width - 4;
    const int previewHeight = qMax(43, qRound(previewWidth * 9.0 / 16.0));
    preview_->setFixedSize(previewWidth, previewHeight);
    setFixedSize(width, previewHeight + 25 + MediaRowHeight);
}

void SceneCard::updateMediaProgress()
{
    if (!mediaSource_ || !mediaSlider_ || seeking_)
        return;
    const int64_t duration = obs_source_media_get_duration(mediaSource_);
    const int64_t time = obs_source_media_get_time(mediaSource_);
    if (duration <= 0) {
        mediaSlider_->hide();
        return;
    }
    mediaSlider_->show();
    const int value = static_cast<int>(
        qBound<int64_t>(int64_t{0}, time * 1000 / duration, int64_t{1000}));
    mediaSlider_->setValue(value);
    mediaSlider_->setToolTip(QStringLiteral("%1 / %2초")
        .arg(time / 1000.0, 0, 'f', 1)
        .arg(duration / 1000.0, 0, 'f', 1));
}

void SceneCard::seekMedia()
{
    if (!mediaSource_ || !mediaSlider_)
        return;
    const int64_t duration = obs_source_media_get_duration(mediaSource_);
    if (duration > 0)
        obs_source_media_set_time(mediaSource_, duration * mediaSlider_->value() / 1000);
}

void SceneCard::setActivateCallback(ActivateCallback callback)
{
    activate_ = std::move(callback);
}

void SceneCard::setReorderCallback(ReorderCallback callback)
{
    reorder_ = std::move(callback);
}

void SceneCard::setActive(bool active)
{
    if (active_ == active)
        return;
    active_ = active;
    applyStyle();
}

void SceneCard::applyStyle()
{
    setStyleSheet(active_
        ? "QFrame#sceneCard{background:#202020;border:2px solid #ff2020;border-radius:3px;}"
          "QLabel{color:#fff;background:#262626;font-weight:600;border:0;}"
        : "QFrame#sceneCard{background:#202020;border:1px solid #555;border-radius:3px;}"
          "QLabel{color:#ddd;background:#262626;font-weight:600;border:0;}");
}

void SceneCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pressPos_ = event->position().toPoint();
        pressed_ = true;
        dragging_ = false;
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void SceneCard::mouseMoveEvent(QMouseEvent *event)
{
    if (pressed_ && (event->buttons() & Qt::LeftButton) &&
        (event->position().toPoint() - pressPos_).manhattanLength() >= QApplication::startDragDistance()) {
        beginDrag();
        return;
    }
    QFrame::mouseMoveEvent(event);
}

void SceneCard::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && pressed_) {
        const bool activate = !dragging_;
        pressed_ = false;
        if (activate && activate_)
            activate_(sceneName_);
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

bool SceneCard::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == preview_ || watched == label_) {
        auto *mouse = dynamic_cast<QMouseEvent *>(event);
        if (mouse) {
            const QPoint cardPos = mapFromGlobal(mouse->globalPosition().toPoint());
            QMouseEvent forwarded(mouse->type(), QPointF(cardPos), mouse->globalPosition(),
                                  mouse->button(), mouse->buttons(), mouse->modifiers());
            if (event->type() == QEvent::MouseButtonPress) {
                mousePressEvent(&forwarded);
                return forwarded.isAccepted();
            }
            if (event->type() == QEvent::MouseMove) {
                mouseMoveEvent(&forwarded);
                return forwarded.isAccepted();
            }
            if (event->type() == QEvent::MouseButtonRelease) {
                mouseReleaseEvent(&forwarded);
                return forwarded.isAccepted();
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

void SceneCard::beginDrag()
{
    dragging_ = true;
    auto *mime = new QMimeData();
    mime->setData(MimeType, sceneName_.toUtf8());
    auto *drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->exec(Qt::MoveAction);
    pressed_ = false;
}

void SceneCard::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(MimeType))
        event->acceptProposedAction();
}

void SceneCard::dropEvent(QDropEvent *event)
{
    const QString source = QString::fromUtf8(event->mimeData()->data(MimeType));
    if (!source.isEmpty() && source != sceneName_ && reorder_)
        reorder_(source, sceneName_);
    event->acceptProposedAction();
}
