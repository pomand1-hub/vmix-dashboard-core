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
#include <QVBoxLayout>
#include <utility>

namespace {
constexpr auto MimeType = "application/x-vmix-dashboard-scene";
}

SceneCard::SceneCard(obs_source_t *source, const QString &name, QWidget *parent)
    : QFrame(parent), sceneName_(name)
{
    setObjectName("sceneCard");
    setAcceptDrops(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(170, 118);

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

    preview_->installEventFilter(this);
    label_->installEventFilter(this);
    applyStyle();
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
