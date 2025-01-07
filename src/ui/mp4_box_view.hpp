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
    QSize sizeHint() const override;

private:
    void buildHierarchy();
    void calculateLayout();
    void layoutBox(BoxInfo* box, const QRectF& availableRect);
    void drawBox(QPainter& painter, const BoxInfo* box);
    int64_t getMaxBoxSize() const;

    std::vector<BoxInfo> boxes_;
    std::vector<BoxInfo*> rootBoxes_;
    double scaleFactor_;
    const int padding_ = 4;
    const int margin_ = 8;
    const int minBoxSize_ = 40;
}; 