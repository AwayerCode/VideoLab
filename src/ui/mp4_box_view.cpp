#include "mp4_box_view.hpp"
#include <QPainter>
#include <QResizeEvent>
#include <cmath>

MP4BoxView::MP4BoxView(QWidget* parent)
    : QWidget(parent)
    , scaleFactor_(1.0)
{
    setMinimumSize(400, 300);
}

MP4BoxView::~MP4BoxView() = default;

void MP4BoxView::setBoxes(const std::vector<BoxInfo>& boxes)
{
    boxes_ = boxes;
    buildHierarchy();
    calculateLayout();
    update();
}

void MP4BoxView::clear()
{
    boxes_.clear();
    rootBoxes_.clear();
    update();
}

void MP4BoxView::buildHierarchy()
{
    rootBoxes_.clear();
    
    // 初始化所有box的parent和children为空
    for (auto& box : boxes_) {
        box.parent = nullptr;
        box.children.clear();
    }
    
    // 构建层级关系
    for (size_t i = 0; i < boxes_.size(); ++i) {
        BoxInfo* currentBox = &boxes_[i];
        
        // 查找当前box的父box
        for (int j = static_cast<int>(i) - 1; j >= 0; --j) {
            BoxInfo* potentialParent = &boxes_[j];
            if (potentialParent->level == currentBox->level - 1) {
                currentBox->parent = potentialParent;
                potentialParent->children.push_back(currentBox);
                break;
            }
        }
        
        // 如果没有父box，则为根box
        if (!currentBox->parent && currentBox->level == 0) {
            rootBoxes_.push_back(currentBox);
        }
    }
}

void MP4BoxView::calculateLayout()
{
    if (boxes_.empty()) return;

    // 设置可用区域（考虑边距）
    QRectF availableRect(margin_, margin_,
                        width() - 2 * margin_,
                        height() - 2 * margin_);
    
    // 计算根box的布局
    double yOffset = availableRect.top();
    double heightPerBox = availableRect.height() / rootBoxes_.size();
    
    for (auto* rootBox : rootBoxes_) {
        // 分配固定高度，但宽度仍然保持全宽
        QRectF boxRect(availableRect.left(), yOffset,
                      availableRect.width(),
                      heightPerBox - padding_);
        
        // 计算此box及其子box的布局
        layoutBox(rootBox, boxRect);
        
        yOffset += heightPerBox;
    }
}

void MP4BoxView::layoutBox(BoxInfo* box, const QRectF& availableRect)
{
    // 保存box的布局位置
    box->rect = availableRect;
    
    if (box->children.empty()) return;
    
    // 为子box分配空间
    QRectF childrenRect = availableRect.adjusted(padding_, padding_,
                                               -padding_, -padding_);
    
    // 如果是根box或第一层box，使用水平布局
    bool useHorizontalLayout = (box->level <= 0);
    
    // 计算子box的布局
    int childCount = box->children.size();
    double spacing = padding_ * 2;  // 增加间距
    
    if (useHorizontalLayout) {
        double widthPerChild = (childrenRect.width() - (childCount - 1) * spacing) / childCount;
        double xOffset = childrenRect.left();
        
        for (auto* childBox : box->children) {
            QRectF childRect(xOffset, childrenRect.top(),
                           widthPerChild, childrenRect.height());
            layoutBox(childBox, childRect);
            xOffset += widthPerChild + spacing;
        }
    } else {
        double heightPerChild = (childrenRect.height() - (childCount - 1) * spacing) / childCount;
        double yOffset = childrenRect.top();
        
        for (auto* childBox : box->children) {
            QRectF childRect(childrenRect.left(), yOffset,
                           childrenRect.width(), heightPerChild);
            layoutBox(childBox, childRect);
            yOffset += heightPerChild + spacing;
        }
    }
}

void MP4BoxView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制所有box
    for (const auto& box : boxes_) {
        drawBox(painter, &box);
    }
}

void MP4BoxView::drawBox(QPainter& painter, const BoxInfo* box)
{
    // 设置box的颜色
    QColor boxColor = QColor::fromHsv((box->level * 60) % 360, 180, 250);
    painter.setPen(QPen(Qt::black, 1));  // 设置边框
    painter.setBrush(boxColor);
    
    // 绘制box（稍微圆角一点）
    painter.drawRoundedRect(box->rect, 4, 4);
    
    // 准备文本信息
    QString sizeStr;
    if (box->size < 1024) {
        sizeStr = QString("%1 B").arg(box->size);
    } else if (box->size < 1024 * 1024) {
        sizeStr = QString("%1 KB").arg(box->size / 1024.0, 0, 'f', 1);
    } else {
        sizeStr = QString("%1 MB").arg(box->size / (1024.0 * 1024.0), 0, 'f', 1);
    }
    
    QString label = QString::fromStdString(box->type) + "\n" + sizeStr;
    
    // 根据box大小调整字体
    QFont font = painter.font();
    font.setPointSize(10);  // 默认字体大小调大一点
    
    if (box->rect.width() < 80 || box->rect.height() < 40) {
        font.setPointSize(8);
    } else if (box->rect.width() < 120 || box->rect.height() < 60) {
        font.setPointSize(9);
    }
    
    painter.setFont(font);
    
    // 绘制文本（添加一点阴影效果）
    painter.setPen(QColor(0, 0, 0, 40));
    QRectF shadowRect = box->rect.adjusted(1, 1, 1, 1);
    painter.drawText(shadowRect, Qt::AlignCenter, label);
    
    painter.setPen(Qt::black);
    painter.drawText(box->rect, Qt::AlignCenter, label);
}

void MP4BoxView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    calculateLayout();
}

QSize MP4BoxView::sizeHint() const
{
    return QSize(800, 600);
} 