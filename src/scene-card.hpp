#pragma once

#include <QFrame>
#include <QPoint>
#include <QString>
#include <functional>
#include <obs.h>

class QLabel;
class ObsPreview;

class SceneCard final : public QFrame {
public:
    using ActivateCallback = std::function<void(const QString &)>;
    using ReorderCallback = std::function<void(const QString &, const QString &)>;

    SceneCard(obs_source_t *source, const QString &name, QWidget *parent = nullptr);

    QString sceneName() const { return sceneName_; }
    void setActive(bool active);
    void setActivateCallback(ActivateCallback callback);
    void setReorderCallback(ReorderCallback callback);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void beginDrag();
    void applyStyle();

    QString sceneName_;
    ObsPreview *preview_ = nullptr;
    QLabel *label_ = nullptr;
    QPoint pressPos_;
    bool pressed_ = false;
    bool dragging_ = false;
    bool active_ = false;
    ActivateCallback activate_;
    ReorderCallback reorder_;
};
