#pragma once

#include <QWidget>
#include <obs.h>

class ObsPreview final : public QWidget {
public:
    explicit ObsPreview(obs_source_t *source, QWidget *parent = nullptr);
    ~ObsPreview() override;

    void setSource(obs_source_t *source);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    QPaintEngine *paintEngine() const override { return nullptr; }

private:
    static void draw(void *data, uint32_t cx, uint32_t cy);
    void createDisplay();
    void destroyDisplay();

    obs_source_t *source_ = nullptr;
    obs_display_t *display_ = nullptr;
    bool showing_ = false;
};
