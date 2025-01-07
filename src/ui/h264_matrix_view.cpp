#include "h264_matrix_view.hpp"
#include <QPainter>
#include <QToolTip>
#include <QMouseEvent>

H264MatrixView::H264MatrixView(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void H264MatrixView::setNALUnits(const std::vector<H264Parser::NALUnitInfo>& nalUnits)
{
    cells_.clear();
    cells_.reserve(nalUnits.size());

    for (const auto& nal : nalUnits) {
        Cell cell;
        cell.type = nal.type;
        cell.isKeyframe = nal.is_keyframe;
        cell.size = nal.size;
        cell.timestamp = nal.timestamp;
        cells_.push_back(cell);
    }

    updateLayout();
    update();
}

void H264MatrixView::clear()
{
    cells_.clear();
    update();
}

void H264MatrixView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制所有单元格
    for (const auto& cell : cells_) {
        QColor color = getColorForNALType(cell.type, cell.isKeyframe);
        painter.fillRect(cell.rect, color);
        painter.setPen(Qt::black);
        painter.drawRect(cell.rect);

        // 在单元格中绘制NAL类型的缩写
        QString typeText = QString::number(static_cast<int>(cell.type));
        painter.drawText(cell.rect, Qt::AlignCenter, typeText);
    }
}

void H264MatrixView::resizeEvent(QResizeEvent* /*event*/)
{
    updateLayout();
}

QSize H264MatrixView::sizeHint() const
{
    if (cells_.empty()) {
        return QSize(400, 300);
    }

    int rows = (cells_.size() + columns_ - 1) / columns_;
    return QSize(
        columns_ * (cellSize_ + spacing_) + spacing_,
        rows * (cellSize_ + spacing_) + spacing_
    );
}

void H264MatrixView::mouseMoveEvent(QMouseEvent* event)
{
    lastMousePos_ = event->pos();

    // 查找鼠标所在的单元格
    for (const auto& cell : cells_) {
        if (cell.rect.contains(event->pos())) {
            showTooltip(event->pos(), getTooltipText(cell));
            return;
        }
    }

    QToolTip::hideText();
}

void H264MatrixView::leaveEvent(QEvent* /*event*/)
{
    QToolTip::hideText();
}

void H264MatrixView::updateLayout()
{
    if (cells_.empty()) return;

    int availableWidth = width() - spacing_;
    columns_ = std::max(1, availableWidth / (cellSize_ + spacing_));

    int currentX = spacing_;
    int currentY = spacing_;
    int column = 0;

    for (auto& cell : cells_) {
        cell.rect = QRect(currentX, currentY, cellSize_, cellSize_);

        column++;
        if (column >= columns_) {
            column = 0;
            currentX = spacing_;
            currentY += cellSize_ + spacing_;
        } else {
            currentX += cellSize_ + spacing_;
        }
    }

    // 更新widget的最小高度
    int rows = (cells_.size() + columns_ - 1) / columns_;
    setMinimumHeight(rows * (cellSize_ + spacing_) + spacing_);
}

QColor H264MatrixView::getColorForNALType(H264Parser::NALUnitType type, bool isKeyframe) const
{
    // 为不同的NAL单元类型定义不同的颜色
    if (isKeyframe) {
        return QColor(255, 100, 100);  // 红色 - 关键帧
    }

    switch (type) {
        case H264Parser::NALUnitType::SPS:
            return QColor(100, 255, 100);  // 绿色 - SPS
        case H264Parser::NALUnitType::PPS:
            return QColor(100, 100, 255);  // 蓝色 - PPS
        case H264Parser::NALUnitType::SEI:
            return QColor(255, 255, 100);  // 黄色 - SEI
        case H264Parser::NALUnitType::CODED_SLICE_NON_IDR:
            return QColor(200, 200, 200);  // 灰色 - 非IDR帧
        case H264Parser::NALUnitType::CODED_SLICE_IDR:
            return QColor(255, 150, 150);  // 浅红色 - IDR帧
        default:
            return QColor(240, 240, 240);  // 浅灰色 - 其他类型
    }
}

QString H264MatrixView::getTooltipText(const Cell& cell) const
{
    QString typeStr;
    switch (cell.type) {
        case H264Parser::NALUnitType::CODED_SLICE_NON_IDR:
            typeStr = "Non-IDR Slice";
            break;
        case H264Parser::NALUnitType::CODED_SLICE_IDR:
            typeStr = "IDR Slice";
            break;
        case H264Parser::NALUnitType::SPS:
            typeStr = "Sequence Parameter Set";
            break;
        case H264Parser::NALUnitType::PPS:
            typeStr = "Picture Parameter Set";
            break;
        case H264Parser::NALUnitType::SEI:
            typeStr = "Supplemental Enhancement Information";
            break;
        default:
            typeStr = QString("Type %1").arg(static_cast<int>(cell.type));
    }

    return QString("Type: %1\nSize: %2 bytes\nTimestamp: %3 s\nKeyframe: %4")
        .arg(typeStr)
        .arg(cell.size)
        .arg(cell.timestamp, 0, 'f', 3)
        .arg(cell.isKeyframe ? "Yes" : "No");
}

void H264MatrixView::showTooltip(const QPoint& pos, const QString& text)
{
    QToolTip::showText(mapToGlobal(pos), text, this, rect());
} 