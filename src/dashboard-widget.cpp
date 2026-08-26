#include "dashboard-widget.hpp"
#include "scene-card.hpp"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QtAlgorithms>
#include <algorithm>
#include <util/platform.h>

DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("vmixDashboardCore");
    setStyleSheet("#vmixDashboardCore{background:#1c1c1c;}");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);

    auto *sizeRow = new QHBoxLayout();
    auto *sizeLabel = new QLabel(QStringLiteral("크기"), this);
    sizeSlider_ = new QSlider(Qt::Horizontal, this);
    sizeSlider_->setRange(80, 360);
    QSettings settings("OpenAI", "vMixDashboardCore");
    previewWidth_ = settings.value("previewWidth", 170).toInt();
    sizeSlider_->setValue(previewWidth_);
    auto *cutButton = new QPushButton(QStringLiteral("CUT"), this);
    cutButton->setFixedWidth(64);
    cutButton->setToolTip(QStringLiteral("미리보기를 프로그램으로 즉시 전환"));
    cutButton->setStyleSheet("QPushButton{background:#b91c1c;color:white;font-weight:700;padding:3px 10px;}"
                             "QPushButton:hover{background:#dc2626;}"
                             "QPushButton:pressed{background:#7f1d1d;}");
    sizeRow->addWidget(sizeLabel);
    sizeRow->addWidget(sizeSlider_);
    sizeRow->addWidget(cutButton);
    root->addLayout(sizeRow);

    connect(sizeSlider_, &QSlider::valueChanged, this, [this](int width) {
        setPreviewWidth(width);
    });
    connect(cutButton, &QPushButton::clicked, this, [this] { cutCurrentPreview(); });
    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    gridHost_ = new QWidget(scroll_);
    grid_ = new QGridLayout(gridHost_);
    grid_->setContentsMargins(0, 0, 0, 0);
    grid_->setSpacing(5);
    grid_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    scroll_->setWidget(gridHost_);
    root->addWidget(scroll_);

    obs_frontend_add_event_callback(&DashboardWidget::frontendEvent, this);
    refreshScenes();

    activeTimer_ = new QTimer(this);
    connect(activeTimer_, &QTimer::timeout, this, [this] { updateActiveScene(); });
    activeTimer_->start(250);
}

DashboardWidget::~DashboardWidget()
{
    obs_frontend_remove_event_callback(&DashboardWidget::frontendEvent, this);
}

void DashboardWidget::frontendEvent(enum obs_frontend_event event, void *data)
{
    auto *self = static_cast<DashboardWidget *>(data);
    switch (event) {
    case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
    case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
    case OBS_FRONTEND_EVENT_FINISHED_LOADING:
        QMetaObject::invokeMethod(self, [self] { self->refreshScenes(); }, Qt::QueuedConnection);
        break;
    case OBS_FRONTEND_EVENT_SCENE_CHANGED:
        QMetaObject::invokeMethod(self, [self] { self->updateActiveScene(); }, Qt::QueuedConnection);
        break;
    default:
        break;
    }
}

void DashboardWidget::refreshScenes()
{
    QStringList available;
    obs_frontend_source_list scenes{};
    obs_frontend_get_scenes(&scenes);
    for (size_t i = 0; i < scenes.sources.num; ++i)
        available << QString::fromUtf8(obs_source_get_name(scenes.sources.array[i]));

    QStringList stored = loadOrder();
    order_.clear();
    for (const QString &name : stored)
        if (available.contains(name) && !order_.contains(name))
            order_ << name;
    for (const QString &name : available)
        if (!order_.contains(name))
            order_ << name;

    qDeleteAll(cards_);
    cards_.clear();

    for (const QString &name : order_) {
        obs_source_t *source = obs_get_source_by_name(name.toUtf8().constData());
        if (!source)
            continue;
        auto *card = new SceneCard(source, name, gridHost_);
        obs_source_release(source);
        card->setActivateCallback([this](const QString &scene) { activateScene(scene); });
        card->setCutCallback([this](const QString &scene) { cutScene(scene); });
        card->setReorderCallback([this](const QString &from, const QString &to) { moveScene(from, to); });
        card->setCardWidth(previewWidth_);
        cards_.insert(name, card);
    }

    obs_frontend_source_list_free(&scenes);
    rebuildGrid();
    updateActiveScene();
    saveOrder();
}

void DashboardWidget::rebuildGrid()
{
    while (QLayoutItem *item = grid_->takeAt(0))
        delete item;

    const int cardWidth = previewWidth_ + 5;
    const int columns = qMax(1, scroll_->viewport()->width() / cardWidth);
    int index = 0;
    for (const QString &name : order_) {
        SceneCard *card = cards_.value(name, nullptr);
        if (!card)
            continue;
        grid_->addWidget(card, index / columns, index % columns);
        ++index;
    }
    gridHost_->adjustSize();
}

void DashboardWidget::setPreviewWidth(int width)
{
    previewWidth_ = width;
    for (SceneCard *card : cards_)
        card->setCardWidth(previewWidth_);
    rebuildGrid();
    QSettings settings("OpenAI", "vMixDashboardCore");
    settings.setValue("previewWidth", previewWidth_);
}

void DashboardWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    rebuildGrid();
}

void DashboardWidget::activateScene(const QString &name)
{
    obs_source_t *source = obs_get_source_by_name(name.toUtf8().constData());
    if (!source)
        return;
    if (obs_frontend_preview_program_mode_active())
        obs_frontend_set_current_preview_scene(source);
    else
        obs_frontend_set_current_scene(source);
    obs_source_release(source);
}

void DashboardWidget::cutScene(const QString &name)
{
    obs_source_t *source = obs_get_source_by_name(name.toUtf8().constData());
    if (!source)
        return;
    obs_frontend_set_current_scene(source);
    obs_source_release(source);
}

void DashboardWidget::cutCurrentPreview()
{
    if (!obs_frontend_preview_program_mode_active())
        return;
    obs_source_t *preview = obs_frontend_get_current_preview_scene();
    if (!preview)
        return;
    obs_frontend_set_current_scene(preview);
    obs_source_release(preview);
}

void DashboardWidget::updateActiveScene()
{
    QString active;
    obs_source_t *source = obs_frontend_get_current_scene();
    if (source) {
        active = QString::fromUtf8(obs_source_get_name(source));
        obs_source_release(source);
    }
    for (auto it = cards_.begin(); it != cards_.end(); ++it)
        it.value()->setActive(it.key() == active);
}

void DashboardWidget::moveScene(const QString &source, const QString &target)
{
    const int from = order_.indexOf(source);
    const int to = order_.indexOf(target);
    if (from < 0 || to < 0 || from == to)
        return;
    order_.move(from, to);
    rebuildGrid();
    saveOrder();
}

QString DashboardWidget::settingsKey() const
{
    char *collection = obs_frontend_get_current_scene_collection();
    const QByteArray raw = collection ? QByteArray(collection) : QByteArray("default");
    if (collection)
        bfree(collection);
    return QStringLiteral("sceneOrder/") + QString::fromLatin1(raw.toBase64(QByteArray::Base64UrlEncoding));
}

QStringList DashboardWidget::loadOrder() const
{
    QSettings settings("OpenAI", "vMixDashboardCore");
    return settings.value(settingsKey()).toStringList();
}

void DashboardWidget::saveOrder() const
{
    QSettings settings("OpenAI", "vMixDashboardCore");
    settings.setValue(settingsKey(), order_);
    settings.sync();
}
