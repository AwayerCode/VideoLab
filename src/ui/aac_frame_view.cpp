#include "aac_frame_view.hpp"
#include <QPainter>
#include <QToolTip>
#include <QMouseEvent>

AACFrameView::AACFrameView(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void AACFrameView::setFrames(const std::vector<AACParser::FrameInfo>& frames)
{
    frameRects_.clear();
    if (frames.empty()) {
        return;
    }

    // 计算统计信息
    maxSampleRate_ = frames[0].sample_rate;
    minSampleRate_ = frames[0].sample_rate;
    maxTimestamp_ = frames[0].timestamp;

    for (const auto& frame : frames) {
        maxSampleRate_ = std::max(maxSampleRate_, frame.sample_rate);
        minSampleRate_ = std::min(minSampleRate_, frame.sample_rate);
        maxTimestamp_ = std::max(maxTimestamp_, frame.timestamp);

        FrameRect frameRect;
        frameRect.info = frame;
        frameRects_.push_back(frameRect);
    }

    updateLayout();
    update();
}

void AACFrameView::clear()
{
    frameRects_.clear();
    maxSampleRate_ = 0;
    minSampleRate_ = 0;
    maxTimestamp_ = 0.0;
    update();
}

void AACFrameView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景网格
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    int gridStep = height() / 10;
    for (int y = 0; y < height(); y += gridStep) {
        painter.drawLine(0, y, width(), y);
    }

    // 绘制帧
    for (const auto& frame : frameRects_) {
        QColor color = getColorForFrame(frame.info);
        painter.fillRect(frame.rect, color);
        painter.setPen(Qt::black);
        painter.drawRect(frame.rect);
    }

    // 绘制时间轴
    painter.setPen(Qt::black);
    int timeStep = width() / 10;
    for (int x = 0; x < width(); x += timeStep) {
        double time = (x * maxTimestamp_) / width();
        painter.drawText(x, height() - 5, QString::number(time, 'f', 1) + "s");
    }
}

void AACFrameView::resizeEvent(QResizeEvent* /*event*/)
{
    updateLayout();
}

QSize AACFrameView::sizeHint() const
{
    return QSize(800, 200);
}

void AACFrameView::mouseMoveEvent(QMouseEvent* event)
{
    lastMousePos_ = event->pos();

    for (const auto& frame : frameRects_) {
        if (frame.rect.contains(event->pos())) {
            showTooltip(event->pos(), getTooltipText(frame));
            return;
        }
    }

    QToolTip::hideText();
}

void AACFrameView::leaveEvent(QEvent* /*event*/)
{
    QToolTip::hideText();
}

void AACFrameView::updateLayout()
{
    if (frameRects_.empty()) return;

    const int minHeight = 2;  // 最小帧高度
    const int spacing = 1;    // 帧间距

    // 计算每个帧的位置
    for (size_t i = 0; i < frameRects_.size(); ++i) {
        auto& frame = frameRects_[i];
        
        // 计算x坐标（基于时间）
        double x = (frame.info.timestamp * width()) / maxTimestamp_;
        
        // 计算高度（基于采样率）
        double heightRatio = static_cast<double>(frame.info.sample_rate - minSampleRate_) /
                           (maxSampleRate_ - minSampleRate_);
        int frameHeight = minHeight + static_cast<int>(heightRatio * (height() - minHeight));
        
        // 设置矩形
        frame.rect = QRect(
            static_cast<int>(x),
            height() - frameHeight,
            std::max(1, width() / static_cast<int>(frameRects_.size()) - spacing),
            frameHeight
        );
    }
}

QColor AACFrameView::getColorForFrame(const AACParser::FrameInfo& frame) const
{
    // 基于通道数和采样率生成颜色
    int hue = (frame.channels * 60) % 360;  // 色调
    int sat = std::min(255, frame.sample_rate / 100);  // 饱和度
    return QColor::fromHsv(hue, sat, 255);
}

QString AACFrameView::getTooltipText(const FrameRect& frame) const
{
    return QString("Time: %1s\nSize: %2 bytes\nSample Rate: %3 Hz\n"
                  "Channels: %4\nProfile: %5\nCRC: %6")
        .arg(frame.info.timestamp, 0, 'f', 3)
        .arg(frame.info.size)
        .arg(frame.info.sample_rate)
        .arg(frame.info.channels)
        .arg(frame.info.profile)
        .arg(frame.info.has_crc ? "Yes" : "No");
}

void AACFrameView::showTooltip(const QPoint& pos, const QString& text)
{
    QToolTip::showText(mapToGlobal(pos), text, this, rect());
} 