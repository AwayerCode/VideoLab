#pragma once

#include <QWidget>
#include <vector>
#include <string>

class MP4BoxView : public QWidget {
    Q_OBJECT

public:
    struct BoxInfo {
        std::string type;
        int64_t size;
        int64_t offset;
        int level;
        std::vector<BoxInfo*> children;
        BoxInfo* parent;
        QRectF rect;
    };

    explicit MP4BoxView(QWidget* parent = nullptr);
    ~MP4BoxView() override;

    void setBoxes(const std::vector<BoxInfo>& boxes);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    QSize sizeHint() const override;

private:
    void buildHierarchy();
    void calculateLayout();
    void layoutBox(BoxInfo* box, const QRectF& availableRect);
    void drawBox(QPainter& painter, const BoxInfo* box);
    void updateTransform();
    QPointF mapToScene(const QPoint& viewportPos) const;
    BoxInfo* findBoxAt(const QPointF& pos);

    std::vector<BoxInfo> boxes_;
    std::vector<BoxInfo*> rootBoxes_;
    double scaleFactor_;
    
    // 布局相关常量
    const int padding_ = 8;         // box内边距
    const int margin_ = 16;         // 外边距
    const int spacing_ = 12;        // box之间的间距
    const int minBoxWidth_ = 120;   // 最小box宽度
    const int minBoxHeight_ = 60;   // 最小box高度
    const int textPadding_ = 8;     // 文本内边距

    // 缩放和平移相关
    double zoomFactor_ = 1.0;
    QPointF viewportOffset_;
    QPoint lastMousePos_;
    bool isPanning_ = false;
    const double minZoom_ = 0.1;
    const double maxZoom_ = 10.0;
    const double zoomStep_ = 1.2;
}; 