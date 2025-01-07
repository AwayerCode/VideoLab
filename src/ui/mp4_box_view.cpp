#include "mp4_box_view.hpp"
#include <QPainter>
#include <QResizeEvent>
#include <cmath>

MP4BoxView::MP4BoxView(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

MP4BoxView::~MP4BoxView() = default;

void MP4BoxView::setBoxes(const std::vector<BoxInfo>& boxes)
{
    boxes_ = boxes;
    updateLayout();
    update();
}

void MP4BoxView::clear()
{
    boxes_.clear();
    boxRects_.clear();
    update();
}

void MP4BoxView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制所有box
    for (size_t i = 0; i < boxes_.size(); ++i) {
        const auto& box = boxes_[i];
        const auto& rect = boxRects_[i];

        // 绘制box背景
        QColor boxColor = getBoxColor(box.type);
        painter.fillRect(rect, boxColor);

        // 绘制box边框
        painter.setPen(Qt::black);
        painter.drawRect(rect);

        // 绘制box标签
        QString label = getBoxLabel(box);
        painter.drawText(rect, Qt::AlignCenter, label);
    }
}

void MP4BoxView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateLayout();
}

QSize MP4BoxView::sizeHint() const
{
    return QSize(600, 400);
}

void MP4BoxView::updateLayout()
{
    if (boxes_.empty()) {
        return;
    }

    // 计算每行可以放置的box数量
    int availableWidth = width() - spacing_;
    columns_ = std::max(1, availableWidth / (boxSize_ + spacing_));

    // 计算实际需要的行数
    int rows = (boxes_.size() + columns_ - 1) / columns_;

    // 调整box大小以适应窗口
    int adjustedBoxSize = std::min(
        (width() - spacing_ * (columns_ + 1)) / columns_,
        (height() - spacing_ * (rows + 1)) / rows
    );

    // 更新所有box的位置
    boxRects_.clear();
    boxRects_.reserve(boxes_.size());

    for (size_t i = 0; i < boxes_.size(); ++i) {
        int row = i / columns_;
        int col = i % columns_;

        QRectF rect(
            spacing_ + col * (adjustedBoxSize + spacing_),
            spacing_ + row * (adjustedBoxSize + spacing_),
            adjustedBoxSize,
            adjustedBoxSize
        );

        boxRects_.push_back(rect);
    }
}

QColor MP4BoxView::getBoxColor(const std::string& type) const
{
    // 为不同类型的box分配不同的颜色
    static const std::map<std::string, QColor> colorMap = {
        {"ftyp", QColor(255, 200, 200)},  // 浅红色
        {"moov", QColor(200, 255, 200)},  // 浅绿色
        {"mdat", QColor(200, 200, 255)},  // 浅蓝色
        {"free", QColor(240, 240, 240)},  // 浅灰色
        {"mvhd", QColor(255, 220, 180)},  // 浅橙色
        {"trak", QColor(180, 255, 220)},  // 浅青色
        {"udta", QColor(220, 180, 255)},  // 浅紫色
        {"meta", QColor(255, 255, 200)}   // 浅黄色
    };

    auto it = colorMap.find(type);
    if (it != colorMap.end()) {
        return it->second;
    }
    
    // 对于未知类型，返回默认颜色
    return QColor(230, 230, 230);
}

QString MP4BoxView::getBoxLabel(const BoxInfo& box) const
{
    // 返回box的类型和大小信息
    QString sizeStr;
    if (box.size < 1024) {
        sizeStr = QString("%1 B").arg(box.size);
    } else if (box.size < 1024 * 1024) {
        sizeStr = QString("%1 KB").arg(box.size / 1024.0, 0, 'f', 1);
    } else {
        sizeStr = QString("%1 MB").arg(box.size / (1024.0 * 1024.0), 0, 'f', 1);
    }

    return QString::fromStdString(box.type) + "\n" + sizeStr;
} 