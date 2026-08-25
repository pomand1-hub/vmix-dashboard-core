#pragma once

#include <QHash>
#include <QStringList>
#include <QWidget>

#include <obs-frontend-api.h>

class QGridLayout;
class QScrollArea;
class QSlider;
class QTimer;
class SceneCard;

class DashboardWidget final : public QWidget {
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    static void frontendEvent(enum obs_frontend_event event, void *data);
    void refreshScenes();
    void rebuildGrid();
    void updateActiveScene();
    void activateScene(const QString &name);
    void moveScene(const QString &source, const QString &target);
    QString settingsKey() const;
    QStringList loadOrder() const;
    void saveOrder() const;
    void setPreviewWidth(int width);

    QScrollArea *scroll_ = nullptr;
    QWidget *gridHost_ = nullptr;
    QGridLayout *grid_ = nullptr;
    QTimer *activeTimer_ = nullptr;
    QSlider *sizeSlider_ = nullptr;
    int previewWidth_ = 170;
    QStringList order_;
    QHash<QString, SceneCard *> cards_;
};
