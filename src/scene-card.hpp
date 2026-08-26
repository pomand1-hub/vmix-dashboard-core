#pragma once

#include <QFrame>
#include <QPoint>
#include <QString>
#include <functional>
#include <obs.h>

class QLabel;
class ObsPreview;
class QSlider;
class QTimer;

class SceneCard final : public QFrame {
public:
    using ActivateCallback = std::function<void(const QString &)>;
    using CutCallback = std::function<void(const QString &)>;
    using ContextCallback = std::function<void(const QString &, const QPoint &)>;
    using ReorderCallback = std::function<void(const QString &, const QString &)>;

    SceneCard(obs_source_t *source, const QString &name, QWidget *parent = nullptr);
    ~SceneCard() override;

    QString sceneName() const { return sceneName_; }
    void setActive(bool active);
    void setPreviewActive(bool active);
    void setActivateCallback(ActivateCallback callback);
    void setCutCallback(CutCallback callback);
    void setContextCallback(ContextCallback callback);
    void setReorderCallback(ReorderCallback callback);
    void setCardWidth(int width);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void beginDrag();
    void applyStyle();
    void prepareMediaThumbnail();
    void updateMediaProgress();
    void seekMedia();

    QString sceneName_;
    ObsPreview *preview_ = nullptr;
    QLabel *label_ = nullptr;
    QSlider *mediaSlider_ = nullptr;
    QTimer *mediaTimer_ = nullptr;
    QWidget *rightIndicator_ = nullptr;
    obs_source_t *mediaSource_ = nullptr;
    QPoint pressPos_;
    bool pressed_ = false;
    bool dragging_ = false;
    bool active_ = false;
    bool previewActive_ = false;
    bool seeking_ = false;
    bool thumbnailPrepared_ = false;
    bool thumbnailPreparing_ = false;
    ActivateCallback activate_;
    CutCallback cut_;
    ContextCallback context_;
    ReorderCallback reorder_;
};
