#pragma once

#include <QWidget>
#include <vector>
#include "../format/aac_parser.hpp"

class AACFrameView : public QWidget {
    Q_OBJECT

public:
    explicit AACFrameView(QWidget* parent = nullptr);
    ~AACFrameView() override = default;

    void setFrames(const std::vector<AACParser::FrameInfo>& frames);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    QSize sizeHint() const override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct FrameRect {
        AACParser::FrameInfo info;
        QRect rect;
    };

    void updateLayout();
    QColor getColorForFrame(const AACParser::FrameInfo& frame) const;
    QString getTooltipText(const FrameRect& frame) const;
    void showTooltip(const QPoint& pos, const QString& text);

    std::vector<FrameRect> frameRects_;
    int maxSampleRate_{0};
    int minSampleRate_{0};
    double maxTimestamp_{0.0};
    QPoint lastMousePos_;
}; 