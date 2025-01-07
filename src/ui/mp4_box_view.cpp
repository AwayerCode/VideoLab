#include "mp4_box_view.hpp"
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <cmath>

MP4BoxView::MP4BoxView(QWidget* parent)
    : QWidget(parent)
    , scaleFactor_(1.0)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);  // 启用鼠标追踪
    setFocusPolicy(Qt::WheelFocus);  // 允许获取焦点
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
                        std::max<int>(width() - 2 * margin_, minBoxWidth_ * 2),
                        std::max<double>(height() - 2 * margin_, 
                            minBoxHeight_ * static_cast<double>(rootBoxes_.size())));
    
    // 计算根box的布局
    double yOffset = availableRect.top();
    double heightPerBox = std::max<double>(
        (availableRect.height() - (rootBoxes_.size() - 1) * spacing_) / rootBoxes_.size(),
        static_cast<double>(minBoxHeight_)
    );
    
    for (auto* rootBox : rootBoxes_) {
        // 分配固定高度，但宽度仍然保持全宽
        QRectF boxRect(availableRect.left(), yOffset,
                      availableRect.width(),
                      heightPerBox);
        
        // 计算此box及其子box的布局
        layoutBox(rootBox, boxRect);
        
        yOffset += heightPerBox + spacing_;
    }
}

void MP4BoxView::layoutBox(BoxInfo* box, const QRectF& availableRect)
{
    // 确保box有足够的空间显示内容
    QRectF adjustedRect = availableRect;
    if (adjustedRect.width() < minBoxWidth_) {
        adjustedRect.setWidth(minBoxWidth_);
    }
    if (adjustedRect.height() < minBoxHeight_) {
        adjustedRect.setHeight(minBoxHeight_);
    }
    
    // 保存box的布局位置
    box->rect = adjustedRect;
    
    if (box->children.empty()) return;
    
    // 为子box分配空间（考虑内边距）
    QRectF childrenRect = adjustedRect.adjusted(padding_, padding_,
                                              -padding_, -padding_);
    
    // 如果是根box或第一层box，使用水平布局
    bool useHorizontalLayout = (box->level <= 0);
    
    // 计算子box的布局
    int childCount = box->children.size();
    
    if (useHorizontalLayout) {
        // 确保每个子box都有最小宽度
        double totalMinWidth = childCount * minBoxWidth_ + (childCount - 1) * spacing_;
        if (childrenRect.width() < totalMinWidth) {
            childrenRect.setWidth(totalMinWidth);
        }
        
        double widthPerChild = (childrenRect.width() - (childCount - 1) * spacing_) / childCount;
        double xOffset = childrenRect.left();
        
        for (auto* childBox : box->children) {
            QRectF childRect(xOffset, childrenRect.top(),
                           widthPerChild, childrenRect.height());
            layoutBox(childBox, childRect);
            xOffset += widthPerChild + spacing_;
        }
    } else {
        // 确保每个子box都有最小高度
        double totalMinHeight = childCount * minBoxHeight_ + (childCount - 1) * spacing_;
        if (childrenRect.height() < totalMinHeight) {
            childrenRect.setHeight(totalMinHeight);
        }
        
        double heightPerChild = (childrenRect.height() - (childCount - 1) * spacing_) / childCount;
        double yOffset = childrenRect.top();
        
        for (auto* childBox : box->children) {
            QRectF childRect(childrenRect.left(), yOffset,
                           childrenRect.width(), heightPerChild);
            layoutBox(childBox, childRect);
            yOffset += heightPerChild + spacing_;
        }
    }
}

void MP4BoxView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 应用缩放和平移变换
    painter.translate(viewportOffset_);
    painter.scale(zoomFactor_, zoomFactor_);
    
    // 绘制所有box
    for (const auto& box : boxes_) {
        drawBox(painter, &box);
    }
}

void MP4BoxView::drawBox(QPainter& painter, const BoxInfo* box)
{
    // 设置box的颜色
    QColor boxColor = QColor::fromHsv((box->level * 60) % 360, 180, 250);
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(boxColor);
    
    // 绘制box（圆角矩形）
    painter.drawRoundedRect(box->rect, 6, 6);
    
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
    
    // 计算文本区域
    QRectF textRect = box->rect.adjusted(textPadding_, textPadding_,
                                       -textPadding_, -textPadding_);
    
    // 设置字体
    QFont font = painter.font();
    font.setPointSize(10);
    
    // 根据box大小自动调整字体大小
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(label);
    int textHeight = fm.lineSpacing() * 2;  // 假设两行文本
    
    if (textWidth > textRect.width() || textHeight > textRect.height()) {
        // 按比例缩小字体
        double scaleFactor = std::min(textRect.width() / textWidth,
                                    textRect.height() / textHeight);
        font.setPointSizeF(font.pointSizeF() * scaleFactor * 0.9);  // 留一些边距
    }
    
    painter.setFont(font);
    
    // 绘制文本（带阴影）
    painter.setPen(QColor(0, 0, 0, 40));
    painter.drawText(textRect.adjusted(1, 1, 1, 1), Qt::AlignCenter, label);
    
    painter.setPen(Qt::black);
    painter.drawText(textRect, Qt::AlignCenter, label);
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

void MP4BoxView::wheelEvent(QWheelEvent* event)
{
    // 获取鼠标位置
    QPointF mousePos = event->position();
    QPointF scenePos = mapToScene(mousePos.toPoint());
    
    // 计算新的缩放因子
    double delta = event->angleDelta().y();
    double factor = std::pow(zoomStep_, delta / 120.0);
    double newZoom = zoomFactor_ * factor;
    
    // 限制缩放范围
    newZoom = std::max(minZoom_, std::min(maxZoom_, newZoom));
    
    if (newZoom != zoomFactor_) {
        // 调整视口偏移以保持鼠标位置不变
        QPointF newOffset = mousePos - (scenePos * newZoom);
        viewportOffset_ = newOffset;
        zoomFactor_ = newZoom;
        update();
    }
    
    event->accept();
}

void MP4BoxView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        isPanning_ = true;
        lastMousePos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    event->accept();
}

void MP4BoxView::mouseMoveEvent(QMouseEvent* event)
{
    if (isPanning_) {
        QPoint delta = event->pos() - lastMousePos_;
        viewportOffset_ += delta;
        lastMousePos_ = event->pos();
        update();
    }
    event->accept();
}

void MP4BoxView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        isPanning_ = false;
        setCursor(Qt::ArrowCursor);
    }
    event->accept();
}

QPointF MP4BoxView::mapToScene(const QPoint& viewportPos) const
{
    return (viewportPos - viewportOffset_) / zoomFactor_;
}

MP4BoxView::BoxInfo* MP4BoxView::findBoxAt(const QPointF& pos)
{
    for (auto& box : boxes_) {
        if (box.rect.contains(pos)) {
            return &box;
        }
    }
    return nullptr;
}

void MP4BoxView::updateTransform()
{
    update();
} 